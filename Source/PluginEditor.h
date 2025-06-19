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
            using namespace juce;
                    
            auto rawBounds = Rectangle<float>(x, y, width, height).toFloat();
            auto size = jmin(rawBounds.getWidth(), rawBounds.getHeight());
            auto bounds = Rectangle<float>(size, size).withCentre(rawBounds.getCentre());
            
            auto radius = size / 2.0f;
            auto center = bounds.getCentre();
            auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

            // Main knob
            g.setColour(Colour::fromRGB(250, 255, 220));
            g.fillEllipse(bounds);

            // Inner circle
            float innerRadius = radius * 0.5f;
            Rectangle<float> innerBounds(center.x - innerRadius, center.y - innerRadius,
                                         innerRadius * 2, innerRadius * 2);
            g.setColour(Colour::fromRGB(200, 200, 190));
            g.fillEllipse(innerBounds);

            // Pointer
            Path pointer;
            float pointerLength = radius * 0.5f;
            float pointerThickness = 3.0f;
            
            pointer.addRectangle(-pointerThickness * 0.5f, -(radius),
                                 pointerThickness, pointerLength);
            
            g.setColour(Colours::black);
            g.fillPath(pointer, AffineTransform::rotation(angle).translated(center.x, center.y));
        }
};

class CustomToggleLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool isMouseOverButton,
                          bool isButtonDown) override
    {
        using namespace juce;
        
        auto bounds = button.getLocalBounds().toFloat();
        
        auto trackWidth = bounds.getWidth() * 0.4f;
        auto trackHeight = bounds.getHeight() * 0.4f;
        auto trackX = bounds.getCentreX() - trackWidth * 0.5f;
        auto trackY = bounds.getCentreY() - trackHeight * 0.5f;
        
        auto trackBounds = Rectangle<float>(trackX, trackY, trackWidth, trackHeight);
        
        auto trackColour = Colour::fromRGB(200, 200, 190);
        auto thumbColour = Colour::fromRGB(250, 255, 220);
        
        g.setColour(trackColour);
        g.fillRoundedRectangle(trackBounds, trackHeight * 0.15f);
        
        float thumbSize = trackHeight;
        float thumbRadius = thumbSize * 0.15f;
        
        float thumbX = button.getToggleState() ? trackBounds.getRight() - thumbSize : trackBounds.getX();
        
        auto thumbBounds = Rectangle<float>(thumbX, trackY, thumbSize, thumbSize);
        
        g.setColour(thumbColour);
        g.fillRoundedRectangle(thumbBounds, thumbRadius);
    }
};

class VisualiserComponent : public juce::Component, private juce::Timer
{
public:
    VisualiserComponent(std::function<float()> getGainFn, std::function<float()> getVerbFn)
        : getGain(std::move(getGainFn)), getVerb(std::move(getVerbFn))
    {
        startTimerHz(30);
    }
    
    void paint(juce::Graphics& g) override
    {
        using namespace juce;
        
        float padLeft = 50.0f;
        float padRight = 5.0f;

        auto localBounds = getLocalBounds().toFloat();
        auto bounds = Rectangle<float>(localBounds.getX() + padLeft,
                                       localBounds.getY(),
                                       localBounds.getWidth() - (padLeft + padRight),
                                       localBounds.getHeight());
        
        float gain = juce::jlimit(0.0f, 1.0f, getGain() / 10);
        float verb = juce::jlimit(0.0f, 1.0f, getVerb() / 10);

        float baseWidth = bounds.getWidth();
        float maxHeight = bounds.getHeight() * 0.5f;
        float fillHeight = gain * maxHeight;

        float centerX = bounds.getCentreX();
        float baseLeft = centerX - baseWidth * 0.5f;
        float baseRight = centerX + baseWidth * 0.5f;
        
        float baseY = bounds.getBottom() - 45.0f;
        float topY = baseY - maxHeight;
        float currentTopY = baseY - fillHeight;
        
        float topX = jmap(verb, centerX, baseRight);
        
        if(gain > 0.0f)
        {
            Path fillTriangle;
            fillTriangle.startNewSubPath(topX, currentTopY);
            fillTriangle.lineTo(baseLeft, baseY);
            fillTriangle.lineTo(baseRight, baseY);
            fillTriangle.closeSubPath();
            
            g.setColour(Colour::fromRGB(200, 200, 190));
            g.fillPath(fillTriangle);
        }

        Path outlineTriangle;
        outlineTriangle.startNewSubPath(topX, topY);
        outlineTriangle.lineTo(baseLeft, baseY);
        outlineTriangle.lineTo(baseRight, baseY);
        outlineTriangle.closeSubPath();

        g.setColour(Colour::fromRGB(200, 200, 190));
        g.strokePath(outlineTriangle, PathStrokeType(3.0f));
    }
    
private:
    std::function<float()> getGain;
    std::function<float()> getVerb;
    
    void timerCallback() override
    {
        repaint();
    }
};

class MeterComponent : public juce::Component
{
public:
    MeterComponent(const juce::String& labelText)
    {
        juce::FontOptions labelFont("Helvetica Neue", 24.0f, juce::Font::bold);
        label.setText(labelText, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centredLeft);
        label.setColour(juce::Label::textColourId, juce::Colour::fromRGB(200, 200, 190));
        label.setFont(labelFont);
        addAndMakeVisible(label);
        addAndMakeVisible(meterBar);
    }
    
    void setLevel(float newLevel)
    {
        level = juce::jlimit(0.0f, 1.0f, newLevel);
        repaint();
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds();
        auto labelWidth = 60;
        
        label.setBounds(bounds.removeFromLeft(labelWidth));
        meterBar.setBounds(bounds);
    }
    
    void paintOverChildren(juce::Graphics& g) override
    {
        auto bounds = meterBar.getBounds().toFloat().reduced(2.0f);
        auto meterWidth = bounds.getWidth() * level;
        
        g.setColour(juce::Colour::fromRGB(200, 200, 190));
        g.fillRect(bounds.removeFromLeft(meterWidth));
        
        g.setColour(juce::Colour::fromRGB(200, 200, 190)); // grey border
        g.drawRect(bounds.toNearestInt(), 1);
    }
    
private:
    juce::Label label;
    juce::Component meterBar;
    float level = 0.0f;
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
    
    juce::ToggleButton darkLightToggle;
    juce::Label darkLightLabel;
    
    CustomKnobLookAndFeel customKnobLookAndFeel;
    CustomToggleLookAndFeel customToggleLookAndFeel;
    
    std::unique_ptr<VisualiserComponent> visualiser;
    
    MeterComponent inputMeter {"INPUT"};
    MeterComponent outputMeter {"OUTPUT"};
    
    void layoutKnobWithLabel(juce::Slider&, juce::Label&, const juce::String&, juce::Rectangle<int>);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverberationMachineAudioProcessorEditor)
};
