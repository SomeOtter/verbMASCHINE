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
}

ReverberationMachineAudioProcessorEditor::~ReverberationMachineAudioProcessorEditor()
{
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
    auto row2 = bounds.removeFromTop(rowHeight);
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
    int knobHeight = area.getHeight() * 0.75f;
    int labelHeight = area.getHeight() - knobHeight;
    
    knob.setSliderStyle(juce::Slider::Rotary);
    knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    knob.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    knob.setLookAndFeel(&customLookAndFeel);

    knob.setBounds(area.removeFromTop(knobHeight));
    
    juce::FontOptions labelFont("Helvetica Neue", 28.0f, juce::Font::bold);
    label.setFont(labelFont);
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centredTop);
    label.setBounds(area.withHeight(labelHeight));
    label.setColour(juce::Label::textColourId, juce::Colour::fromRGB(200, 200, 190));
}
