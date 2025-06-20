/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class ReverberationMachineAudioProcessor  : public juce::AudioProcessor
{
public:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts {*this, nullptr,
        "Parameters", createParameterLayout()};
    
    float targetGain = 1.0f;
    
    std::atomic<float> inputLevelL {0.0f};
    std::atomic<float> inputLevelR {0.0f};
    std::atomic<float> outputLevelL {0.0f};
    std::atomic<float> outputLevelR {0.0f};
    
    juce::Reverb::Parameters reverbParams;
    juce::Reverb reverbL, reverbR;
    
    juce::dsp::StateVariableTPTFilter<float> reverbHighCutL, reverbHighCutR;
    juce::dsp::StateVariableTPTFilter<float> tailFilterL, tailFilterR;
    juce::SmoothedValue<float> tailCutoffL, tailCutoffR;
    
    float tailEnvelopeL = 0.0f;
    float tailEnvelopeR = 0.0f;
    
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> tailModDelayL, tailModDelayR;
    float lfoPhase = 0.0f;
    float lfoRateHz = 0.6f;
    float lfoDepthMs = 60.0f;
    
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> preDelayL, preDelayR;
    float preDelayTimeMs = 80.0f;
    
    //==============================================================================
    ReverberationMachineAudioProcessor();
    ~ReverberationMachineAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverberationMachineAudioProcessor)
};
