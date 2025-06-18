/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class CustomKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                               float sliderPosProportional, float rotaryStartAngle,
                               float rotaryEndAngle, juce::Slider& slider) override
        {
            auto radius = juce::jmin(width / 2.0f, height / 2.0f) - 4.0f;
            auto centreX = x + width  * 0.5f;
            auto centreY = y + height * 0.5f;
            auto rx = centreX - radius;
            auto ry = centreY - radius;
            auto rw = radius * 2.0f;
            auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

            // Background knob (light beige)
            g.setColour(juce::Colour::fromRGB(240, 230, 200)); // light beige
            g.fillEllipse(rx, ry, rw, rw);

            // Inner circle (light grey)
            g.setColour(juce::Colour::fromRGB(200, 200, 200)); // light grey
            g.fillEllipse(centreX - 4, centreY - 4, 8, 8); // Small center circle

            // Pointer (black)
            juce::Path pointer;
            float pointerLength = radius * 0.75f;
            float pointerThickness = 2.0f;
            pointer.addRectangle(-pointerThickness * 0.5f, -pointerLength, pointerThickness, pointerLength);
            g.setColour(juce::Colours::black);
            g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(centreX, centreY));
        }
};

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
    
    CustomKnobLookAndFeel customLookAndFeel;
    
    void layoutKnobWithLabel(juce::Slider&, juce::Label&, const juce::String&, juce::Rectangle<int>);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverberationMachineAudioProcessorEditor)
};
