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
    
    float volSkewFactor =   std::log(0.5f) /
                            std::log((volDefaultValue - volMinValue) / (volMaxValue - volMinValue));
    
    layout.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("VOL", 1),
        "VOL", juce::NormalisableRange<float>(-48.0, 12.0f, 0.1f, volSkewFactor), 0.0f));
    
    layout.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("GAIN", 1),
        "GAIN", juce::NormalisableRange<float>(0.0f, 1.0f), 0.1f));
    
    layout.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("VERB", 1),
        "VERB", juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    
    layout.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("DARK // LIGHT", 1),
        "DARK // LIGHT", false));
    
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
//    reverb.reset();
//    reverb.setSampleRate(sampleRate);
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
    
    // Get input level for meters
    float inL = buffer.getRMSLevel(0, 0, buffer.getNumSamples());
    float inR = buffer.getNumChannels() > 1 ? buffer.getRMSLevel(1, 0, buffer.getNumSamples()) : inL;
    inputLevelL.store(inL);
    inputLevelR.store(inR);
    
    // Gain config
    float gainParam = apvts.getRawParameterValue("GAIN")->load();
    float shaped = std::pow(gainParam, 2.2f);
    float drive1 = juce::jmap(shaped, 1.0f, 10.0f);
    float drive2 = juce::jmap(shaped, 1.0f, 5.0f);
    
    // Gate config
    float threshold = 0.01f;
    float releaseRate = 0.999f;
    static float envelopeL = 0.0f;
    static float envelopeR = 0.0f;
    
    // Clipping functions
    auto softClip = [](float x)
    {
        return x / (1.0f + std::abs(1.0f * x));
    };
    
    auto hardClip = [](float x)
    {
        return juce::jlimit(-0.4f, 0.4f, x);
    };
    
    if(gainParam > 0.0001f)
    {
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            float& envelope = (channel == 0) ? envelopeL : envelopeR;
            
            for(int i = 0; i < buffer.getNumSamples(); ++i)
            {
                float sample = channelData[i];
                
                float stage1 = softClip(sample * drive1);
                float stage2 = hardClip(stage1 * drive2);
                
                float mixed = juce::jmap(gainParam, sample, stage2);
                
                float absMixed = std::abs(mixed);
                if(absMixed > envelope)
                    envelope = absMixed;
                else
                    envelope *= releaseRate;
                
                float gateGain = juce::jlimit(0.0f, 1.0f, juce::jmap(envelope, 0.0f, threshold, 0.0f, 1.0f));
                gateGain = std::pow(gateGain, 6.0f);
                
                channelData[i] = mixed * gateGain;
            }
        }
    }
    
    // Verb config
    float verbAmount = apvts.getRawParameterValue("VERB")->load();
    
    reverbParams.roomSize = 0.9f;
    reverbParams.damping = 0.6f;
    reverbParams.wetLevel = verbAmount;
    reverbParams.dryLevel = 1.0f - verbAmount;
    reverbParams.width = 0.6f;
    reverbParams.freezeMode = 0.0f;
    
    reverb.setParameters(reverbParams);
    
    if(buffer.getNumChannels() >= 2)
    {
        reverb.processStereo(buffer.getWritePointer(0), buffer.getWritePointer(1), buffer.getNumSamples());
    }
    else
    {
        reverb.processMono(buffer.getWritePointer(0), buffer.getNumSamples());
    }

    // Vol config
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
    
    // Get output levels for meter
    float outL = buffer.getRMSLevel(0, 0, buffer.getNumSamples());
    float outR = buffer.getNumChannels() > 1 ? buffer.getRMSLevel(1, 0, buffer.getNumSamples()) : outL;
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
