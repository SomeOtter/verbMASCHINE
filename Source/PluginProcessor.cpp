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
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
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
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void ReverberationMachineAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ReverberationMachineAudioProcessor();
}
