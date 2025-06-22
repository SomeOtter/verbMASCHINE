/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>

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
            float lineWidth = radius * 0.06f;
            Rectangle<float> innerBounds(center.x - innerRadius, center.y - innerRadius,
                                         innerRadius * 2, innerRadius * 2);
            g.setColour(Colour::fromRGB(200, 200, 190));
            g.fillEllipse(innerBounds);
            g.setColour(Colour::fromRGB(27, 27, 27));
            g.drawEllipse(innerBounds, lineWidth);
            
            // Pointer
            Path pointer;
            float pointerLength = radius * 0.5f;
            float pointerThickness = lineWidth;
            
            pointer.addRectangle(-pointerThickness * 0.5f, -(radius),
                                 pointerThickness, pointerLength);
            
            g.setColour(Colour::fromRGB(27, 27, 27));
            g.fillPath(pointer, AffineTransform::rotation(angle).translated(center.x, center.y));
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
        
        float gain = juce::jlimit(0.0f, 1.0f, getGain());
        float verb = juce::jlimit(0.0f, 1.0f, getVerb());

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

class StereoMeterComponent : public juce::Component, private juce::Timer
{
public:
    StereoMeterComponent()
    {
        addAndMakeVisible(leftLevelLabel);
        addAndMakeVisible(rightLevelLabel);
        
        auto setupLabel = [](juce::Label& label)
        {
            juce::FontOptions dbFont("Helvetica Neue", 10.0f, juce::Font::bold);
            label.setJustificationType(juce::Justification::centredLeft);
            label.setColour(juce::Label::textColourId, juce::Colour::fromRGB(200, 200, 190));
            label.setFont(dbFont);
            label.setInterceptsMouseClicks(false, false);
        };
        
        setupLabel(leftLevelLabel);
        setupLabel(rightLevelLabel);
        
        startTimerHz(30);
    }
    
    void setRawLevels(float left, float right)
    {
        leftRawLevel = juce::jlimit(0.0f, 1.0f, left);
        rightRawLevel = juce::jlimit(0.0f, 1.0f, right);
        
        if(leftRawLevel >= 0.99f) clipOpacityLeft = 1.0f;
        if(rightRawLevel >= 0.99f) clipOpacityRight = 1.0f;
        
        repaint();
    }
    
    void setChannelHeight(float h) {channelHeight = h;}
    void setChannelSpacing(float s) {channelSpacing = s;}
    
    void resized() override
    {
        auto r = getLocalBounds().toFloat();
        
        const float h = (channelHeight >= 0)
            ? channelHeight
            : (r.getHeight() - channelSpacing) * 0.5f;
        
        const float totalUsed = h * 2.0f + channelSpacing;
        const float startY = (r.getHeight() - totalUsed) * 0.5f;
        
        auto fullLeft = juce::Rectangle<float>(r.getX(), r.getY() + startY, r.getWidth(), h);
        auto fullRight = juce::Rectangle<float>(r.getX(),
                                                       r.getY() + startY + h + channelSpacing,
                                                       r.getWidth(), h);
        
        const float textWidth = 40.0f;
        
        leftTextArea = fullLeft.removeFromLeft(textWidth);
        rightTextArea = fullRight.removeFromLeft(textWidth);
        
        leftMeterBounds = fullLeft.withX(fullLeft.getX()).withWidth(fullLeft.getWidth());
        rightMeterBounds = fullRight.withX(fullRight.getX()).withWidth(fullRight.getWidth());
        
        leftLevelLabel.setBounds(leftTextArea.toNearestInt());
        rightLevelLabel.setBounds(rightTextArea.toNearestInt());
    }
    
private:
    float leftRawLevel = 0.0f, rightRawLevel = 0.0f;
    float leftSmoothedLevel = 0.0f, rightSmoothedLevel = 0.0f;
    float leftTextLevel = 0.0f, rightTextLevel = 0.0f;
    const float smoothFactor = 0.2f;
    const float textSmoothFactor = 0.02;
    
    float clipOpacityLeft = 0.0f, clipOpacityRight = 0.0f;
    const float clipFade = 0.02f;
    
    float channelHeight = 7.0f;
    float channelSpacing = 7.0f;

    juce::Rectangle<float> leftMeterBounds, rightMeterBounds;
    juce::Rectangle<float> leftTextArea, rightTextArea;
    
    juce::Label leftLevelLabel, rightLevelLabel;
    
    void timerCallback() override
    {
        leftSmoothedLevel = smoothFactor * leftRawLevel + (1.0f - smoothFactor) * leftSmoothedLevel;
        rightSmoothedLevel = smoothFactor * rightRawLevel + (1.0f - smoothFactor) * rightSmoothedLevel;
        leftTextLevel = textSmoothFactor * leftRawLevel + (1.0f - textSmoothFactor) * leftTextLevel;
        rightTextLevel = textSmoothFactor * rightRawLevel + (1.0f - textSmoothFactor) * rightTextLevel;
        
        if(clipOpacityLeft > 0.0f)
            clipOpacityLeft = juce::jmax(0.0f, clipOpacityLeft - clipFade);
        if(clipOpacityRight > 0.0f)
            clipOpacityRight = juce::jmax(0.0f, clipOpacityRight - clipFade);
        
        auto toDbString = [](float level)
        {
            if (level <= 0.0001f)
                return juce::String::fromUTF8 (u8"-\u221E"); // '-∞'
            else
                return juce::String(juce::Decibels::gainToDecibels(level, -80.0f), 1);
        };
        
        leftLevelLabel.setText(toDbString(leftTextLevel), juce::dontSendNotification);
        rightLevelLabel.setText(toDbString(rightTextLevel), juce::dontSendNotification);
        
        repaint();
    }
    
    void paint(juce::Graphics& g) override
    {
        drawOneMeter(g, leftMeterBounds, leftSmoothedLevel, clipOpacityLeft);
        drawOneMeter(g, rightMeterBounds, rightSmoothedLevel, clipOpacityRight);
    }
    
    void drawOneMeter(juce::Graphics& g,
                      juce::Rectangle<float> area,
                      float level,
                      float& clipOpacity)
    {
        g.setColour(juce::Colour::fromRGB(200, 200, 190));
        g.fillRect(area);
        
        const float fillWidth = area.getWidth() * level;
        g.setColour(juce::Colour::fromRGB(0, 205, 0).withAlpha(1.0f));
        g.fillRect(area.withWidth(fillWidth));
        
        if(clipOpacity > 0.0f)
        {
            const float clipWidth = juce::jlimit(0.0f, area.getWidth(), level * 20.0f);
            g.setColour(juce::Colour::fromRGB(230, 0, 0));
            g.fillRect(area.withX(area.getRight() - clipWidth).withWidth(clipWidth));
        }
    }
};

class TailMeterComponent : public juce::Component, private juce::Timer
{
public:
    TailMeterComponent()
    {
        startTimerHz(30);
    }
    
    void setRawTailLevels(float left, float right)
    {
        rawL = left;
        rawR = right;
    }
    
    void reset() {
        smoothedL = smoothedR = 0.0f;
    }
    
    void paint(juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat().reduced(4.0f);
        
        g.setColour(juce::Colour::fromRGB(200, 200, 190));
        g.fillRect(area);
        
        auto top = area.removeFromTop(area.getHeight() * 0.5f);
        
        g.setColour(juce::Colour::fromRGB(0, 200, 200));
        g.fillRect(top.withWidth(area.getWidth() * smoothedL));
        
        g.setColour(juce::Colours::purple.withAlpha(0.6f));
        g.fillRect(area.withWidth(area.getWidth() * smoothedR));
    }
    
private:
    float rawL = 0.0f, rawR = 0.0f;
    float smoothedL = 0.0f, smoothedR = 0.0f;
    const float smoothAlpha = 0.2f;
    
    void timerCallback() override
    {
        smoothedL = smoothAlpha * rawL + (1.0f - smoothAlpha) * smoothedL;
        smoothedR = smoothAlpha * rawR + (1.0f - smoothAlpha) * smoothedR;
        repaint();
    }
};

class ReverberationMachineAudioProcessorEditor  :   public juce::AudioProcessorEditor,
                                                    private juce::Timer
{
public:
    ReverberationMachineAudioProcessorEditor (ReverberationMachineAudioProcessor&);
    ~ReverberationMachineAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp(const juce::MouseEvent& event) override;
    void timerCallback() override;

private:
    ReverberationMachineAudioProcessor& audioProcessor;
    
    juce::Label titleLabel;
    
    CustomKnobLookAndFeel customKnobLookAndFeel;
    
    juce::Slider volKnob, gainKnob, verbKnob;
    juce::Label volLabel, gainLabel, verbLabel;
    
    juce::Slider darkLightKnob;
    juce::Label darkLightLabel;
    
    std::unique_ptr<VisualiserComponent> visualiser;
    
    juce::Rectangle<int> row1Fill;
    juce::Rectangle<int> inputFill, outputFill;
    juce::Label inputLabel, outputLabel;
    juce::Label tailsLabel;
    
    StereoMeterComponent stereoInputMeter, stereoOutputMeter;
    TailMeterComponent tailMeter;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> verbAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> darkLightAttachment;
    
    void layoutKnobWithLabel(juce::Slider&, juce::Label&, const juce::String&, juce::Rectangle<int>);
    
    juce::Colour startColour = juce::Colour::fromRGB(0, 200, 200);
    juce::Colour targetColour = juce::Colour::fromRGB(200, 200, 190);
    float colourBlend = 0.0f;
    bool isAnimating = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverberationMachineAudioProcessorEditor)
};
