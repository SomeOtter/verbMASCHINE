/*
  ==============================================================================

    CustomKnob.h
    Created: 22 Jun 2025 11:28:17pm
    Author:  Otto Knuepfer

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"

class CustomKnobLookAndFeel;

class CustomKnob : public juce::Slider
{
public:
    CustomKnob();
    ~CustomKnob() override;
    
    void setDisplayValues(juce::String minLabel, juce::String maxLabel);
    
    juce::String getDisplayMin() const {return displayMin;}
    juce::String getDisplayMax() const {return displayMax;}
    
private:
    juce::String displayMin, displayMax;
    std::unique_ptr<CustomKnobLookAndFeel> lookAndFeel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomKnob)
};
