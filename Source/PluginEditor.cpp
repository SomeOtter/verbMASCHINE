/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ReverberationMachineAudioProcessorEditor::ReverberationMachineAudioProcessorEditor (ReverberationMachineAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
fontOptions("Helvetica Neue", 85.0f, juce::Font::bold)
{
    visualiser = std::make_unique<VisualiserComponent>(
                                                       [this] { return gainKnob.getValue(); },
                                                       [this] { return verbKnob.getValue(); });
    addAndMakeVisible(*visualiser);
    
    setSize (620, 600);
    
    addAndMakeVisible(titleLabel);
    titleLabel.setText("GRITTTTTIMER", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setFont(fontOptions);
    titleLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(200, 200, 190));
    titleLabel.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    
    addAndMakeVisible(volKnob);
    addAndMakeVisible(gainKnob);
    addAndMakeVisible(verbKnob);
    
    addAndMakeVisible(volLabel);
    addAndMakeVisible(gainLabel);
    addAndMakeVisible(verbLabel);
    
    addAndMakeVisible(darkLightToggle);
    addAndMakeVisible(darkLightLabel);
    
    darkLightToggle.setClickingTogglesState(true);
    darkLightToggle.setLookAndFeel(&customToggleLookAndFeel);
    
    juce::FontOptions labelFont("Helvetica Neue", 18.0f, juce::Font::bold);
    darkLightLabel.setFont(labelFont);
    darkLightLabel.setText("DARK / LIGHT", juce::dontSendNotification);
    darkLightLabel.setJustificationType(juce::Justification::centredTop);
    darkLightLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(200, 200, 190));
}

ReverberationMachineAudioProcessorEditor::~ReverberationMachineAudioProcessorEditor()
{
    volKnob.setLookAndFeel(nullptr);
    gainKnob.setLookAndFeel(nullptr);
    verbKnob.setLookAndFeel(nullptr);
    darkLightToggle.setLookAndFeel(nullptr);
}

//==============================================================================
void ReverberationMachineAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace juce;
    
    // Background
    g.fillAll(Colour::fromRGB(20, 20, 20));
    
    // Border
    g.setColour(Colour::fromRGB(200, 200, 190));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(8), 15.f, 4.f);
}

void ReverberationMachineAudioProcessorEditor::resized()
{
    // Layout
    auto bounds = getLocalBounds().reduced(10);
    const int TOTALROWS = 4;
    auto rowHeight = bounds.getHeight() / TOTALROWS;
    
    auto row1 = bounds.removeFromTop(rowHeight);
    {
        auto meterArea = row1;
        auto meterWidth = meterArea.getWidth() / 2;
        
        auto inputArea = meterArea.removeFromLeft(meterWidth);
        
        
        
        auto outputArea = meterArea;
    }
    
    auto row2 = bounds.removeFromTop(rowHeight);
    {
        auto visualArea = row2.removeFromLeft(row2.getWidth() * 2 / 3);
        visualiser->setBounds(visualArea);
        
        auto toggleArea = row2;

        int totalHeight = toggleArea.getHeight();
        int toggleHeight = totalHeight * 0.6f;
        int labelHeight = totalHeight - toggleHeight;

        // Get area for toggle
        auto toggleBounds = toggleArea.removeFromTop(toggleHeight);
        darkLightToggle.setBounds(toggleBounds);

        // Directly below the toggle, no extra space
        auto labelBounds = toggleArea.removeFromTop(labelHeight);
        darkLightLabel.setBounds(labelBounds);
        darkLightLabel.setJustificationType(juce::Justification::centredTop);
    }
    
    auto row3 = bounds.removeFromTop(rowHeight);
    {
        auto knobWidth = row3.getWidth() / 3;
        
        auto volArea = row3.removeFromLeft(knobWidth);
        auto gainArea = row3.removeFromLeft(knobWidth);
        auto verbArea = row3;
        
        layoutKnobWithLabel(volKnob, volLabel, "VOL", volArea);
        layoutKnobWithLabel(gainKnob, gainLabel, "GAIN", gainArea);
        layoutKnobWithLabel(verbKnob, verbLabel, "VERB", verbArea);
    }
    
    auto row4 = bounds;
    titleLabel.setBounds(row4);
}

void ReverberationMachineAudioProcessorEditor::layoutKnobWithLabel(juce::Slider& knob,
                                                                   juce::Label& label,
                                                                   const juce::String& text,
                                                                   juce::Rectangle<int> area)
{
    const int labelYOffset = 10;
    const int knobHeight = area.getHeight() * 0.75f;
    const int labelHeight = area.getHeight() - knobHeight - labelYOffset;

    knob.setBounds(area.removeFromTop(knobHeight));
    knob.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    knob.setLookAndFeel(&customKnobLookAndFeel);

    area.removeFromTop(labelYOffset);

    juce::FontOptions labelFont("Helvetica Neue", 24.0f, juce::Font::bold);
    label.setFont(labelFont);
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centredTop);
    label.setColour(juce::Label::textColourId, juce::Colour::fromRGB(200, 200, 190));
    label.setBounds(area.removeFromTop(labelHeight));
}
