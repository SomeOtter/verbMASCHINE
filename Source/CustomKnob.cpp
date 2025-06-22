/*
  ==============================================================================

    CustomKnob.cpp
    Created: 22 Jun 2025 11:28:17pm
    Author:  Otto Knuepfer

  ==============================================================================
*/

#include "CustomKnob.h"
#include "PluginEditor.h"

CustomKnob::CustomKnob() : juce::Slider(juce::Slider::RotaryHorizontalVerticalDrag,
                                        juce::Slider::TextBoxBelow),
                                        lookAndFeel(new CustomKnobLookAndFeel())
{
    setLookAndFeel(lookAndFeel.get());
    setTextBoxStyle(NoTextBox, true, 0, 0);
}

CustomKnob::~CustomKnob()
{
    setLookAndFeel(nullptr);
}

void CustomKnob::setDisplayValues(juce::String minLabel, juce::String maxLabel)
{
    displayMin = std::move(minLabel);
    displayMax = std::move(maxLabel);
}
