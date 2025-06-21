/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ReverberationMachineAudioProcessorEditor::ReverberationMachineAudioProcessorEditor (ReverberationMachineAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    
    titleLabel.addMouseListener(this, true);
    
    visualiser = std::make_unique<VisualiserComponent>(
                                                       [this] { return gainKnob.getValue(); },
                                                       [this] { return verbKnob.getValue(); });
    addAndMakeVisible(*visualiser);
    
    setSize (620, 600);
    
    juce::FontOptions titleFont("Helvetica Neue", 85.0f, juce::Font::bold);
    addAndMakeVisible(titleLabel);
    titleLabel.setText("verbMASCHINE", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setFont(titleFont);
    titleLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(0, 200, 200));
    titleLabel.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    titleLabel.setInterceptsMouseClicks(true, false);
    titleLabel.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    
    addAndMakeVisible(volKnob);
    addAndMakeVisible(gainKnob);
    addAndMakeVisible(verbKnob);
    
    addAndMakeVisible(volLabel);
    addAndMakeVisible(gainLabel);
    addAndMakeVisible(verbLabel);
    
    addAndMakeVisible(darkLightKnob);
    darkLightKnob.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    darkLightKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    darkLightKnob.setLookAndFeel(&customKnobLookAndFeel);
    
    juce::FontOptions labelFont("Helvetica Neue", 15.0f, juce::Font::bold);
    addAndMakeVisible(darkLightLabel);
    darkLightLabel.setFont(labelFont);
    darkLightLabel.setText("DARK / LIGHT", juce::dontSendNotification);
    darkLightLabel.setJustificationType(juce::Justification::centredTop);
    darkLightLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(200, 200, 190));
    
    juce::FontOptions meterFont("Helvetica Neue", 28.0f, juce::Font::bold);
    addAndMakeVisible(inputLabel);
    inputLabel.setFont(meterFont);
    inputLabel.setText("INPUT", juce::dontSendNotification);
    inputLabel.setJustificationType(juce::Justification::centredLeft);
    inputLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(200, 200, 190));
    
    addAndMakeVisible(outputLabel);
    outputLabel.setFont(meterFont);
    outputLabel.setText("OUTPUT", juce::dontSendNotification);
    outputLabel.setJustificationType(juce::Justification::centredLeft);
    outputLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(200, 200, 190));
    
    addAndMakeVisible(inputMeterL);
    addAndMakeVisible(inputMeterR);
    addAndMakeVisible(outputMeterL);
    addAndMakeVisible(outputMeterR);
    startTimerHz(30);
    
    volAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, "VOL", volKnob);
    gainAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, "GAIN", gainKnob);
    verbAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, "VERB", verbKnob);
    darkLightAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, "DARK_LIGHT", darkLightKnob);
}

ReverberationMachineAudioProcessorEditor::~ReverberationMachineAudioProcessorEditor()
{
    volKnob.setLookAndFeel(nullptr);
    gainKnob.setLookAndFeel(nullptr);
    verbKnob.setLookAndFeel(nullptr);
    darkLightKnob.setLookAndFeel(nullptr);
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
    
    // Meter area
    g.setColour(juce::Colour::fromRGB(27, 27, 27));
    g.fillRoundedRectangle(row1Fill.toFloat(), 15.f);
}

void ReverberationMachineAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(15);
    const int totalRows = 4;
    auto rowHeight = bounds.getHeight() / totalRows;

    auto row1 = bounds.removeFromTop(rowHeight);
    row1Fill = row1.reduced(10);

    const int padding = 25;
    const int meterWidth = 140;
    const int meterHeight = 5;
    const int meterSpacing = 8;

    // Calculate text widths using GlyphArrangement for precision
    auto getTextWidth = [](const juce::String& text, const juce::Font& font) -> int {
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(font, text, 0.0f, 0.0f);
        return static_cast<int>(glyphs.getBoundingBox(0, glyphs.getNumGlyphs(), true).getWidth()) + 8; // +gap
    };

    const int inputTextWidth  = getTextWidth("INPUT", inputLabel.getFont());
    const int outputTextWidth = getTextWidth("OUTPUT", outputLabel.getFont());
    const int labelWidth = juce::jmax(inputTextWidth, outputTextWidth + 30);
    const int gapBetweenLabelAndMeter = 10;

    int y = row1.getCentreY() - ((2 * meterHeight + meterSpacing) / 2);

    // Input Half
    {
        auto inputArea = row1.removeFromLeft(row1.getWidth() / 2).reduced(padding, 0);

        juce::Rectangle<int> labelBounds = inputArea.removeFromLeft(labelWidth);
        inputLabel.setBounds(labelBounds.withY(y).withHeight(2 * meterHeight + meterSpacing));
        inputLabel.setJustificationType(juce::Justification::centredLeft);

        // Meter sits immediately to the right of label
        auto meterX = labelBounds.getRight() + gapBetweenLabelAndMeter;
        inputMeterL.setBounds(meterX, y, meterWidth, meterHeight);
        inputMeterR.setBounds(meterX, y + meterHeight + meterSpacing, meterWidth, meterHeight);
    }

    // Output Half
    {
        auto outputArea = row1.reduced(padding, 0);  // Remaining right half

        juce::Rectangle<int> labelBounds = outputArea.removeFromLeft(labelWidth);
        outputLabel.setBounds(labelBounds.withY(y).withHeight(2 * meterHeight + meterSpacing));
        outputLabel.setJustificationType(juce::Justification::centredLeft);

        auto meterX = labelBounds.getRight() + gapBetweenLabelAndMeter;
        outputMeterL.setBounds(meterX, y, meterWidth, meterHeight);
        outputMeterR.setBounds(meterX, y + meterHeight + meterSpacing, meterWidth, meterHeight);
    }

    
    auto row2 = bounds.removeFromTop(rowHeight);
    {
        auto visualArea = row2.removeFromLeft(row2.getWidth() * 2 / 3);
        visualiser->setBounds(visualArea);
        
        auto darkLightArea = row2.reduced(10);

        int totalHeight = darkLightArea.getHeight();
        int knobHeight = totalHeight * 0.5f;

        // Get area for toggle
        auto knobBounds = darkLightArea.removeFromTop(knobHeight);
        darkLightKnob.setBounds(knobBounds);

        // Directly below the toggle, no extra space
        auto labelBounds = darkLightArea.reduced(15);
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

void ReverberationMachineAudioProcessorEditor::timerCallback()
{
    // Meter Updates
    inputMeterL.setLevel(audioProcessor.inputLevelL.load());
    inputMeterR.setLevel(audioProcessor.inputLevelR.load());
    outputMeterL.setLevel(audioProcessor.outputLevelL.load());
    outputMeterR.setLevel(audioProcessor.outputLevelR.load());
    
    // Text Animation
    if (!isAnimating)
        return;

    constexpr float step = 0.1f; // Animation speed
    colourBlend += step;

    if (colourBlend >= 1.0f)
    {
        colourBlend = 1.0f;
        isAnimating = false;
        stopTimer();
    }

    auto blended = startColour.interpolatedWith(targetColour, colourBlend);
    titleLabel.setColour(juce::Label::textColourId, blended);
}

void ReverberationMachineAudioProcessorEditor::mouseUp(const juce::MouseEvent& event)
{
    if (event.eventComponent == &titleLabel)
    {
        auto& processor = static_cast<ReverberationMachineAudioProcessor&>(audioProcessor);
        auto* bypassParam = dynamic_cast<juce::AudioParameterBool*>(processor.apvts.getParameter("BYPASS"));

        if (bypassParam != nullptr)
        {
            bool newState = !bypassParam->get();

            bypassParam->beginChangeGesture();
            bypassParam->setValueNotifyingHost(newState ? 1.0f : 0.0f);
            bypassParam->endChangeGesture();

            startColour = titleLabel.findColour(juce::Label::textColourId);
            targetColour = newState ? juce::Colour::fromRGB(200, 200, 190) : juce::Colour::fromRGB(0, 200, 200);
            colourBlend = 0.0f;
            isAnimating = true;

            startTimerHz(60);
        }
    }
}
