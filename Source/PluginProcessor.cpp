/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout ReverberationMachineAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> layout;
    
    float volMinValue = -48.0f;
    float volMaxValue = 12.0f;
    float volDefaultValue = 0.0f;
    
    float volSkewFactor = std::log(0.5f) /
                            std::log((volDefaultValue - volMinValue) / (volMaxValue - volMinValue));
    
    layout.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("VOL", 1),
        "VOL", juce::NormalisableRange<float>(-48.0, 12.0f, 0.1f, volSkewFactor), 0.0f));
    
    layout.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("GAIN", 1),
        "GAIN", juce::NormalisableRange<float>(0.0f, 1.0f), 0.25f));
    
    layout.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("VERB", 1),
        "VERB", juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f, 0.5f), 0.25f));
    
    layout.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("DARK_LIGHT", 1),
        "DARK_LIGHT", juce::NormalisableRange<float>(-1.0f, 1.0f, 0.0001f), 0.0f));
    
    layout.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("BYPASS", 1), "BYPASS", false));
    
    return {layout.begin(), layout.end()};
}

//==============================================================================
ReverberationMachineAudioProcessor::ReverberationMachineAudioProcessor()
    : AudioProcessor (BusesProperties()
                      .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

ReverberationMachineAudioProcessor::~ReverberationMachineAudioProcessor()
{
}

//==============================================================================
const juce::String ReverberationMachineAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ReverberationMachineAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ReverberationMachineAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ReverberationMachineAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ReverberationMachineAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ReverberationMachineAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ReverberationMachineAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ReverberationMachineAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String ReverberationMachineAudioProcessor::getProgramName (int index)
{
    return {};
}

void ReverberationMachineAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void ReverberationMachineAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    
    reverbL.reset();
    reverbR.reset();
    reverbL.setSampleRate(sampleRate);
    reverbR.setSampleRate(sampleRate);
    
    reverbHighCutL.prepare(spec);
    reverbHighCutR.prepare(spec);
    reverbHighCutL.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    reverbHighCutR.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    reverbHighCutL.setCutoffFrequency(15000.0f);
    reverbHighCutR.setCutoffFrequency(15000.0f);
    reverbHighCutL.setResonance(0.3f);
    reverbHighCutR.setResonance(0.3f);
    
    tailFilterL.prepare(spec);
    tailFilterR.prepare(spec);
    tailFilterL.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    tailFilterR.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    tailFilterL.setResonance(0.5f);
    tailFilterR.setResonance(0.5f);
    
    tailCutoffL.reset(sampleRate, 0.05);
    tailCutoffR.reset(sampleRate, 0.05);
    
    tailModDelayL.reset();
    tailModDelayR.reset();
    tailModDelayL.prepare(spec);
    tailModDelayR.prepare(spec);
    tailModDelayL.setMaximumDelayInSamples(static_cast<int>(sampleRate));
    tailModDelayR.setMaximumDelayInSamples(static_cast<int>(sampleRate));
    tailModDelayL.setDelay(10.0f);
    tailModDelayR.setDelay(10.0f);
    
    preDelayL.reset();
    preDelayR.reset();
    preDelayL.prepare(spec);
    preDelayR.prepare(spec);
    preDelayL.setMaximumDelayInSamples(static_cast<int>(sampleRate * 1.0f));
    preDelayR.setMaximumDelayInSamples(static_cast<int>(sampleRate * 1.0f));
    preDelayL.setDelay((sampleRate * preDelayTimeMs) / 1000.0f);
    preDelayR.setDelay((sampleRate * preDelayTimeMs) / 1000.0f);
    
    tiltLowShelfL.reset();
    tiltLowShelfR.reset();
    tiltHighShelfL.reset();
    tiltHighShelfR.reset();
    tiltLowShelfL.prepare(spec);
    tiltLowShelfR.prepare(spec);
    tiltHighShelfL.prepare(spec);
    tiltHighShelfR.prepare(spec);
}


void ReverberationMachineAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ReverberationMachineAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void ReverberationMachineAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    juce::AudioBuffer<float> bypassBuffer;
    bypassBuffer.makeCopyOf(buffer);
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);
    juce::AudioBuffer<float> processedDryBuffer;
    processedDryBuffer.makeCopyOf(dryBuffer);
    
    bool isBypassed = apvts.getRawParameterValue("BYPASS")->load() >= 0.5f;
    if(!isBypassed)
    {
        // === Gain on processedDryBuffer === //
        float gainParam = apvts.getRawParameterValue("GAIN")->load();
        float shaped = std::pow(gainParam, 2.2f);
        float drive1 = juce::jmap(shaped, 1.0f, 8.0f);
        float drive2 = juce::jmap(shaped, 1.0f, 2.5f);

        float gateThreshold = 0.01f;
        float gateReleaseRate = 0.9995f;
        float gateAttackRate = 0.3f;
        static float gateEnvL = 0.0f;
        static float gateEnvR = 0.0f;

        auto softClip = [](float x) { return x / (1.0f + std::abs(x)); };
        auto hardClip = [](float x) { return juce::jlimit(-0.4f, 0.4f, x); };

        if (gainParam > 0.0001f)
        {
            for (int channel = 0; channel < processedDryBuffer.getNumChannels(); ++channel)
            {
                auto* channelData = processedDryBuffer.getWritePointer(channel);
                float& envelope = (channel == 0) ? gateEnvL : gateEnvR;
                
                for (int i = 0; i < processedDryBuffer.getNumSamples(); ++i)
                {
                    float sample = channelData[i];
                    
                    float stage1 = softClip(sample * drive1);
                    float stage2 = hardClip(stage1 * drive2);
                    float mixed = juce::jmap(gainParam, sample, stage2);
                    
                    float absMixed = std::abs(mixed);
                    envelope = (absMixed > envelope)
                    ? gateAttackRate * absMixed + (1.0f - gateAttackRate) * envelope
                    : gateReleaseRate;
                        
                    float gainCurve = juce::jlimit(0.0f, 1.0f, (envelope - gateThreshold) / (0.05f - gateThreshold));
                    gainCurve = std::pow(gainCurve, 2.0f);
                    
                    channelData[i] = mixed * gainCurve;
                }
            }
        }

        juce::AudioBuffer<float> wetBuffer;
        wetBuffer.makeCopyOf(processedDryBuffer);
        
        // === Reverb Predelay === //
        for(int i = 0; i < wetBuffer.getNumSamples(); ++i)
        {
            float sampleL = wetBuffer.getSample(0, i);
            float sampleR = wetBuffer.getNumChannels() > 1 ? wetBuffer.getSample(1, i) : sampleL;
            
            preDelayL.pushSample(0, sampleL);
            preDelayR.pushSample(0, sampleR);
            
            float delayedL = preDelayL.popSample(0);
            float delayedR = preDelayR.popSample(0);
            
            wetBuffer.setSample(0, i, delayedL);
            if (wetBuffer.getNumChannels() > 1)
                wetBuffer.setSample(1, i, delayedR);
        }

        // === Reverb and Filtering === //
        reverbParams.roomSize = 0.95f;
        reverbParams.damping = 0.1f;
        reverbParams.wetLevel = 1.0f;
        reverbParams.dryLevel = 0.0f;
        reverbParams.width = 0.8f;
        reverbParams.freezeMode = 0.0f;
        
        reverbL.setParameters(reverbParams);
        reverbR.setParameters(reverbParams);

        juce::dsp::AudioBlock<float> block(wetBuffer);
        auto blockL = block.getSingleChannelBlock(0);
        auto blockR = block.getSingleChannelBlock(1);
        juce::dsp::ProcessContextReplacing<float> contextL(blockL);
        juce::dsp::ProcessContextReplacing<float> contextR(blockR);
        
        reverbL.processMono(wetBuffer.getWritePointer(0), wetBuffer.getNumSamples());
        reverbHighCutL.process(contextL);

        if (wetBuffer.getNumChannels() > 1)
            {
                reverbR.processMono(wetBuffer.getWritePointer(1), wetBuffer.getNumSamples());
                reverbHighCutR.process(contextR);
            }

        auto mapTailCutoff = [](float level)
        {
            float db = juce::Decibels::gainToDecibels(level + 1e-5f);
            db = juce::jlimit(-60.0f, 0.0f, db);
            
            float norm = juce::jmap(db, -60.0f, 0.0f, 1.0f, 0.0f);
            float shapedNorm = std::pow(norm, 2.5f);
            
            return juce::jmap(shapedNorm, 40.0f, 6000.0f);
        };

        float tailsReleaseRate = 0.9995f;
        
        for(int i = 0; i < wetBuffer.getNumSamples(); ++i)
        {
            float sampleL = std::abs(wetBuffer.getSample(0, i));
            float sampleR = wetBuffer.getNumChannels() > 1 ? std::abs(wetBuffer.getSample(1, i)) : sampleL;
            
            tailEnvelopeL = std::max(sampleL, tailEnvelopeL * tailsReleaseRate);
            tailEnvelopeR = std::max(sampleR, tailEnvelopeR * tailsReleaseRate);
        }
        
        tailCutoffL.setTargetValue(mapTailCutoff(tailEnvelopeL));
        tailCutoffR.setTargetValue(mapTailCutoff(tailEnvelopeR));
        
        for(int i = 0; i < wetBuffer.getNumSamples(); ++i)
        {
            float cutoffL = tailCutoffL.getNextValue();
            float cutoffR = tailCutoffR.getNextValue();
            
            tailFilterL.setCutoffFrequency(cutoffL);
            tailFilterR.setCutoffFrequency(cutoffR);
            
            float wetSampleL = wetBuffer.getSample(0, i);
            float wetSampleR = wetBuffer.getNumChannels() > 1 ? wetBuffer.getSample(1, i) : wetSampleL;
            
            wetSampleL = tailFilterL.processSample(0, wetSampleL);
            if(wetBuffer.getNumChannels() > 1)
            {
                wetSampleR = tailFilterR.processSample(0, wetSampleR);
            }
            
            wetBuffer.setSample(0, i, wetSampleL);
            if (wetBuffer.getNumChannels() > 1)
                wetBuffer.setSample(1, i, wetSampleR);
        }
        
        // === Reverb Modulation === //
        const float sampleRate = getSampleRate();
        const float lfoIncrement = (2.0f * juce::MathConstants<float>::pi * lfoRateHz) / sampleRate;
        
        for(int i = 0; i < wetBuffer.getNumSamples(); ++i)
        {
            float lfoValue = std::sin(lfoPhase);
            float modulatedDelayMs = 10.0f + lfoValue * lfoDepthMs;
            float maxDelayMs = (tailModDelayL.getMaximumDelayInSamples() * 1000.0f) / sampleRate;
            modulatedDelayMs = juce::jlimit(0.0f, maxDelayMs, modulatedDelayMs);
            
            tailModDelayL.setDelay(modulatedDelayMs);
            tailModDelayR.setDelay(modulatedDelayMs);
            
            float sampleL = wetBuffer.getSample(0, i);
            float sampleR = wetBuffer.getNumSamples() > 1 ? wetBuffer.getSample(1, i) : sampleL;
            
            tailModDelayL.pushSample(0, sampleL);
            tailModDelayR.pushSample(0, sampleR);
            
            float modSampleL = tailModDelayL.popSample(0);
            float modSampleR = tailModDelayR.popSample(0);
            
            wetBuffer.setSample(0, i, modSampleL);
            if(wetBuffer.getNumChannels() > 1)
                wetBuffer.setSample(1, i, modSampleR);
            
            lfoPhase += lfoIncrement;
            if(lfoPhase >= 2.0f * juce::MathConstants<float>::pi)
                lfoPhase -= 2.0f * juce::MathConstants<float>::pi;
        }
        
        // Measure tails
        float tailEnvL = 0.0f;
        float tailEnvR = 0.0f;
        const float tailRelease = 0.9995;
        float verbAmount = apvts.getRawParameterValue("VERB")->load();
        
        for(int i = 0; i < wetBuffer.getNumSamples(); ++i)
        {
            float tailSampleL = std::abs(wetBuffer.getSample(0, i));
            float tailSampleR = wetBuffer.getNumChannels() > 1
                ? std::abs(wetBuffer.getSample(1, i))
                : tailSampleL;
            
            tailEnvL = std::max(tailSampleL, tailEnvL * tailRelease);
            tailEnvR = std::max(tailSampleR, tailEnvR * tailRelease);
            
            float scaledTailL = tailEnvL * verbAmount;
            float scaledTailR = tailEnvR * verbAmount;
            
            tailLevelL.store(scaledTailL);
            tailLevelR.store(scaledTailR);
        }

        // === Final Wet/Dry Mix === //
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* wet = wetBuffer.getReadPointer(channel);
            auto* dry = processedDryBuffer.getReadPointer(channel);
            auto* out = buffer.getWritePointer(channel);

            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                out[i] = dry[i] * (1.0f - verbAmount) + wet[i] * verbAmount;
            }
        }
        
        // === Dark / Light Tilt EQ === //
        juce::dsp::AudioBlock<float> finalBlock(buffer);
        auto finalBlockL = finalBlock.getSingleChannelBlock(0);
        auto finalBlockR = finalBlock.getSingleChannelBlock(1);
        juce::dsp::ProcessContextReplacing<float> finalContextL(finalBlockL);
        juce::dsp::ProcessContextReplacing<float> finalContextR(finalBlockR);
        
        updateTiltEQ();
        
        tiltLowShelfL.process(finalContextL);
        tiltLowShelfR.process(finalContextR);
        tiltHighShelfL.process(finalContextL);
        tiltHighShelfR.process(finalContextR);

        // === Output Volume Control === //
        auto volDb = apvts.getRawParameterValue("VOL")->load();
        targetGain = juce::Decibels::decibelsToGain(volDb);

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                channelData[i] *= targetGain;
            }
        }
    }
    
    measureLevels(bypassBuffer, buffer);
}

void ReverberationMachineAudioProcessor::measureLevels(const juce::AudioBuffer<float>& inputBuffer,
                                                       const juce::AudioBuffer<float>& outputBuffer)
{
    const int numSamples = inputBuffer.getNumSamples();
    const int numChannels = inputBuffer.getNumChannels();
    
    float inL = inputBuffer.getRMSLevel(0, 0, numSamples);
    float inR = numChannels > 1 ? inputBuffer.getRMSLevel(1, 0, numSamples) : inL;
    
    inputLevelL.store(inL);
    inputLevelR.store(inR);
    
    float outL = outputBuffer.getRMSLevel(0, 0, numSamples);
    float outR = numChannels > 1 ? outputBuffer.getRMSLevel(1, 0, numSamples) : outL;
    
    outputLevelL.store(outL);
    outputLevelR.store(outR);
}

//==============================================================================
bool ReverberationMachineAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ReverberationMachineAudioProcessor::createEditor()
{
    return new ReverberationMachineAudioProcessorEditor (*this);
}

//==============================================================================
void ReverberationMachineAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    copyXmlToBinary(*apvts.copyState().createXml(), destData);
}

void ReverberationMachineAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    
    if(xmlState && xmlState->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ReverberationMachineAudioProcessor();
}

void ReverberationMachineAudioProcessor::updateTiltEQ()
{
    float tilt = apvts.getRawParameterValue("DARK_LIGHT")->load();
    tilt = juce::jlimit(-1.0f, 1.0f, tilt);
    tilt = std::tanh(tilt * 2.0f);
    tilt = -tilt;
    
    float lowGainDb = tilt * 2.0f;
    float highGainDb = tilt * 6.0f;
    float freq = 800.0f;
    float q = 0.707f;
    
    auto lowShelf = juce::dsp::IIR::Coefficients<float>::makeLowShelf(getSampleRate(), freq, q, juce::Decibels::decibelsToGain(lowGainDb));
    auto highShelf = juce::dsp::IIR::Coefficients<float>::makeHighShelf(getSampleRate(), freq, q, juce::Decibels::gainToDecibels(highGainDb));
    
    tiltLowShelfL.coefficients = lowShelf;
    tiltLowShelfR.coefficients = lowShelf;
    tiltHighShelfL.coefficients = lowShelf;
    tiltHighShelfR.coefficients = lowShelf;
}
