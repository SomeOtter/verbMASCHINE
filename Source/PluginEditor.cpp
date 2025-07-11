/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
verbMASCHINEAudioProcessorEditor::verbMASCHINEAudioProcessorEditor (verbMASCHINEAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    
    titleLabel.addMouseListener(this, true);
    
    visualiser = std::make_unique<VisualiserComponent>(
                                                       [this] { return gainKnob.getValue(); },
                                                       [this] { return verbKnob.getValue(); });
    addAndMakeVisible(*visualiser);
    
    startTimerHz(30);
    
    setSize(650, 700);

    addAndMakeVisible(titleLabel);
    titleLabel.setFont(CustomFonts::karasumaGothicBlack.withHeight(90.0f));
    titleLabel.setText("verbMASCHINE", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, CustomColours::aqua);
    titleLabel.setInterceptsMouseClicks(true, false);
    titleLabel.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    
    addAndMakeVisible(volKnob);
    addAndMakeVisible(gainKnob);
    addAndMakeVisible(verbKnob);
    addAndMakeVisible(darkLightKnob);
    
    volKnob.setDisplayValues("-48", "+12");
    gainKnob.setDisplayValues("0%", "100%");
    verbKnob.setDisplayValues("0%", "125%");
    
    addAndMakeVisible(volLabel);
    addAndMakeVisible(gainLabel);
    addAndMakeVisible(verbLabel);
    
    addAndMakeVisible(darkLightLabel);
    darkLightLabel.setFont(CustomFonts::karasumaGothicBold.withHeight(15.0f));
    darkLightLabel.setText("DARK / LIGHT", juce::dontSendNotification);
    darkLightLabel.setJustificationType(juce::Justification::centredTop);
    darkLightLabel.setColour(juce::Label::textColourId, CustomColours::lightGrey);
    
    addAndMakeVisible(inputLabel);
    inputLabel.setFont(CustomFonts::karasumaGothicBlack.withHeight(28.0f));
    inputLabel.setText("INPUT", juce::dontSendNotification);
    inputLabel.setJustificationType(juce::Justification::centredLeft);
    inputLabel.setColour(juce::Label::textColourId, CustomColours::lightGrey);
    
    addAndMakeVisible(outputLabel);
    outputLabel.setFont(CustomFonts::karasumaGothicBlack.withHeight(28.0f));
    outputLabel.setText("OUTPUT", juce::dontSendNotification);
    outputLabel.setJustificationType(juce::Justification::centredLeft);
    outputLabel.setColour(juce::Label::textColourId, CustomColours::lightGrey);
    
    addAndMakeVisible(stereoInputMeter);
    addAndMakeVisible(stereoOutputMeter);
    stereoInputMeter.setChannelSpacing(6.0f);
    stereoOutputMeter.setChannelSpacing(6.0f);
    stereoInputMeter.setChannelHeight(4.0f);
    
    addAndMakeVisible(tailMeter);
    addAndMakeVisible(tailsLabel);
    tailsLabel.setFont(CustomFonts::karasumaGothicBoldItalic.withHeight(26.0f));
    tailsLabel.setText("TAILS", juce::dontSendNotification);
    tailsLabel.setJustificationType(juce::Justification::topLeft);
    tailsLabel.setColour(juce::Label::textColourId, CustomColours::lightGrey);
    
    volAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, "VOL", volKnob);
    gainAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, "GAIN", gainKnob);
    verbAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, "VERB", verbKnob);
    darkLightAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, "DARK_LIGHT",                                                                                darkLightKnob);
}

verbMASCHINEAudioProcessorEditor::~verbMASCHINEAudioProcessorEditor()
{
    volKnob.setLookAndFeel(nullptr);
    gainKnob.setLookAndFeel(nullptr);
    verbKnob.setLookAndFeel(nullptr);
    darkLightKnob.setLookAndFeel(nullptr);
}

//==============================================================================
void verbMASCHINEAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Background
    g.fillAll(CustomColours::offBlack);
    
    // Border
    g.setColour(CustomColours::lightGrey);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(8), 15.0f, 4.0f);
    
    // Meter area
    g.setColour(CustomColours::darkGrey);
    g.fillRoundedRectangle(row1Fill.toFloat(), 10.0f);
    
    g.setColour(CustomColours::offBlack);
    g.fillRoundedRectangle(inputFill.toFloat(), 10.0f);
    g.fillRoundedRectangle(outputFill.toFloat(), 10.0f);
}

void verbMASCHINEAudioProcessorEditor::resized()
{
    // Main split into 4 rows
    auto bounds = getLocalBounds().reduced(15);
    const int totalRows = 4;
    auto rowHeight = bounds.getHeight() / totalRows;

    // === Row 1 === //
    auto row1 = bounds.removeFromTop(rowHeight).reduced(5);
    row1Fill = row1;
    
    const float inoutHeight = row1.getHeight() * 0.75f;
    auto inoutBounds = row1.removeFromTop(inoutHeight).reduced(5);
    inoutBounds = inoutBounds.withSizeKeepingCentre(inoutBounds.getWidth() * 0.95,
                                                    inoutBounds.getHeight());
    
    const float channelHeight = inoutBounds.getHeight() * 0.45f;
    const float gap = inoutBounds.getHeight() * 0.06f;
    juce::Rectangle<int> gapBounds;
    
    auto inputBounds = inoutBounds.removeFromTop(channelHeight);
    gapBounds = inoutBounds.removeFromTop(gap);
    auto outputBounds = inoutBounds.removeFromTop(channelHeight);
    
    inputFill = inputBounds;
    outputFill = outputBounds;
    
    // Labels
    auto textBoundsWidth = [](juce::Rectangle<int> bounds)
    {
        return bounds.getWidth() * 0.25f;
    };
    
    auto inputTextBounds = inputBounds.removeFromLeft(textBoundsWidth(inputBounds));
    inputTextBounds = inputTextBounds.reduced(5);
    auto outputTextBounds = outputBounds.removeFromLeft(textBoundsWidth(outputBounds));
    outputTextBounds = outputTextBounds.reduced(5);
    
    inputLabel.setBounds(inputTextBounds);
    inputLabel.setJustificationType(juce::Justification::centredLeft);
    outputLabel.setBounds(outputTextBounds);
    outputLabel.setJustificationType(juce::Justification::centredLeft);
    
    // Level Meters
    auto inputMeterBounds = inputBounds;
    auto outputMeterBounds = outputBounds;
    inputMeterBounds = inputMeterBounds.withSizeKeepingCentre(
        inputMeterBounds.getWidth() * 0.95, inputMeterBounds.getHeight());
    outputMeterBounds = outputMeterBounds.withSizeKeepingCentre(
        outputMeterBounds.getWidth() * 0.95, outputMeterBounds.getHeight());
    
    stereoInputMeter.setBounds(inputMeterBounds);
    stereoOutputMeter.setBounds(outputMeterBounds);
    
    // Tails Meter
    auto tailBounds = row1.withSizeKeepingCentre(row1.getWidth() * 0.91f, row1.getHeight());
    auto tailsLabelBounds = tailBounds.removeFromLeft(tailBounds.getWidth() * 0.15f);
    auto tailMeterBounds = tailBounds.withSizeKeepingCentre(tailBounds.getWidth(),
                                                            tailBounds.getHeight() * 0.65f);
    tailMeterBounds.setY(tailBounds.getY() + 0.5f);
    
    tailsLabel.setBounds(tailsLabelBounds);
    tailMeter.setBounds(tailMeterBounds);
    
    // === Row 2 === //
    auto row2 = bounds.removeFromTop(rowHeight);
    {
        auto visualArea = row2.removeFromLeft(row2.getWidth() * 2 / 3);
        visualiser->setBounds(visualArea);
        
        auto darkLightArea = row2.reduced(10);

        int totalHeight = darkLightArea.getHeight();
        int knobHeight = totalHeight * 0.58f;

        auto knobBounds = darkLightArea.removeFromTop(knobHeight);
        darkLightKnob.setBounds(knobBounds);

        auto labelBounds = darkLightArea.reduced(15);
        darkLightLabel.setBounds(labelBounds);
        darkLightLabel.setJustificationType(juce::Justification::centredTop);
    }
    
    // === Row 3 === //
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
    
    // === Row 4 === //
    auto row4 = bounds;
    titleLabel.setBounds(row4);
}

void verbMASCHINEAudioProcessorEditor::layoutKnobWithLabel(juce::Slider& knob,
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

    label.setFont(CustomFonts::karasumaGothicBold.withHeight(24.0f));
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centredTop);
    label.setColour(juce::Label::textColourId, CustomColours::lightGrey);
    label.setBounds(area.removeFromTop(labelHeight));
}

void verbMASCHINEAudioProcessorEditor::timerCallback()
{
    // Meter Updates
    stereoInputMeter.setRawLevels(
                               audioProcessor.inputLevelL.load(),
                               audioProcessor.inputLevelR.load());
    stereoOutputMeter.setRawLevels(
                               audioProcessor.outputLevelL.load(),
                               audioProcessor.outputLevelR.load());
    
    tailMeter.setRawTailLevels(audioProcessor.tailLevelL.load(),
                               audioProcessor.tailLevelR.load());
    
    // Text Animation
    if (!isAnimating)
        return;

    constexpr float step = 0.1f; // Animation speed
    colourBlend += step;

    if (colourBlend >= 1.0f)
    {
        colourBlend = 1.0f;
        isAnimating = false;
    }

    auto blended = startColour.interpolatedWith(targetColour, colourBlend);
    titleLabel.setColour(juce::Label::textColourId, blended);
}

void verbMASCHINEAudioProcessorEditor::mouseUp(const juce::MouseEvent& event)
{
    if (event.eventComponent == &titleLabel)
    {
        auto& processor = static_cast<verbMASCHINEAudioProcessor&>(audioProcessor);
        auto* bypassParam = dynamic_cast<juce::AudioParameterBool*>(processor.apvts.getParameter("BYPASS"));

        if (bypassParam != nullptr)
        {
            bool newState = !bypassParam->get();

            bypassParam->beginChangeGesture();
            bypassParam->setValueNotifyingHost(newState ? 1.0f : 0.0f);
            bypassParam->endChangeGesture();

            startColour = titleLabel.findColour(juce::Label::textColourId);
            targetColour = newState ? CustomColours::lightGrey : CustomColours::aqua;
            colourBlend = 0.0f;
            isAnimating = true;

            startTimerHz(60);
        }
    }
}
