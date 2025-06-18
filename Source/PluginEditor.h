/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class ReverberationMachineAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    ReverberationMachineAudioProcessorEditor (ReverberationMachineAudioProcessor&);
    ~ReverberationMachineAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ReverberationMachineAudioProcessor& audioProcessor;
    
    juce::FontOptions fontOptions;
    juce::Label titleLabel;
    
    juce::Slider volKnob, gainKnob, verbKnob;
    juce::Label volLabel, gainLabel, verbLabel;
    
    void layoutKnobWithLabel(juce::Slider&, juce::Label&, const juce::String&, juce::Rectangle<int>);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverberationMachineAudioProcessorEditor)
};
