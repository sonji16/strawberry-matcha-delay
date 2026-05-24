/*
  ==============================================================================
    PluginEditor.cpp
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "DelayLookAndFeel.h"
#include "ScopeComponent.h"
#include "EQComponent.h"

//string labels of the delay rate sync setting buttons
const char* _177delayyAudioProcessorEditor::divLabels[9] = {
    "1/4", "1/8", "1/16",
    "d1/4", "d1/8", "d1/16",
    "t1/4", "t1/8", "t1/16"
};

//==============================================================================
_177delayyAudioProcessorEditor::_177delayyAudioProcessorEditor (_177delayyAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      timeSA  (p.parms, "time", timeSlider),
      fdbkSA  (p.parms, "fdbk", fdbkSlider),
      mixSA   (p.parms, "mix",  mixSlider),
      syncAtt (p.parms, "sync", syncButton),
      scope   (p.scopeCollector),
    //label font setting (font, size, letter spacing)
      eqComp  (p.parms, laf.talinaFont.withHeight(10.0f).withExtraKerningFactor(0.1f), laf.talinaFont.withHeight(14.0f).withExtraKerningFactor(0.1f))
{
    setLookAndFeel (&laf);
    
    
    //setting up the top three knob (time, feedback, mix)
    auto setupKnob = [&](juce::Slider& s, const juce::String& name,
                         juce::Label& l,  const juce::String& labelText)
    {
        s.setName (name);
        s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 100, 30);
        addAndMakeVisible (s);

        l.setText (labelText, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setFont (laf.talinaFont.withHeight(18.0f).withExtraKerningFactor(0.1f));
        l.setColour (juce::Label::textColourId,
                     juce::Colour (DelayLookAndFeel::colTextPrimary));
        addAndMakeVisible (l);
    };

    setupKnob (timeSlider, "Time",     timeLabel, "TIME");
    setupKnob (fdbkSlider, "Feedback", fdbkLabel, "FEEDBACK");
    setupKnob (mixSlider,  "Mix",      mixLabel,  "MIX");

    // Sync button setting
    syncButton.setClickingTogglesState (true);
    syncButton.setColour (juce::TextButton::buttonColourId,
                          juce::Colour (DelayLookAndFeel::colSurface));
    syncButton.setColour (juce::TextButton::buttonOnColourId,
                          juce::Colour (DelayLookAndFeel::colPink));
    syncButton.setColour (juce::TextButton::textColourOffId,
                          juce::Colour (DelayLookAndFeel::colTextPrimary));
    syncButton.setColour (juce::TextButton::textColourOnId,
                          juce::Colour (DelayLookAndFeel::colSurface));
    syncButton.onClick = [this] {
        updateDivisionButtons();
        updateTimeKnobAppearance();
    };
    addAndMakeVisible (syncButton);

    // Tap tempo button
    tapButton.setColour (juce::TextButton::buttonColourId,
                         juce::Colour (DelayLookAndFeel::colSurface));
    tapButton.setColour (juce::TextButton::textColourOffId,
                         juce::Colour (DelayLookAndFeel::colTextPrimary));
    tapButton.onClick = [this] { audioProcessor.tapTempo(); };
    addAndMakeVisible (tapButton);

    // Division buttons (delay rate setting)
    for (int i = 0; i < 9; ++i)
    {
        divButtons[i].setButtonText (divLabels[i]);
        divButtons[i].setClickingTogglesState (false);
        //setting colors
        divButtons[i].setColour (juce::TextButton::buttonColourId,
                                 juce::Colour (DelayLookAndFeel::colSurface));
        divButtons[i].setColour (juce::TextButton::buttonOnColourId,
                                 juce::Colour (DelayLookAndFeel::colPink));
        divButtons[i].setColour (juce::TextButton::textColourOffId,
                                 juce::Colour (DelayLookAndFeel::colTextPrimary));
        divButtons[i].setColour (juce::TextButton::textColourOnId,
                                 juce::Colour (DelayLookAndFeel::colSurface));
        const int index = i;
        divButtons[i].onClick = [this, index]
        {
            // Set division parameter
            if (auto* param = audioProcessor.parms.getParameter ("division"))
                param->setValueNotifyingHost (param->convertTo0to1 ((float) index));

            // Update time knob to reflect synced value at current BPM
            const double bpm     = audioProcessor.getCurrentBPM();
            const double beatMs  = 60000.0 / bpm;
            const double delayMs = beatMs * _177delayyAudioProcessor::divisionMultipliers[index];
            const float  clamped = (float) juce::jlimit (1.0, 2000.0, delayMs);

            if (auto* timeParam = audioProcessor.parms.getParameter ("time"))
                timeParam->setValueNotifyingHost (timeParam->convertTo0to1 (clamped));

            updateDivisionButtons();
        };
        addAndMakeVisible (divButtons[i]);
    }

    updateDivisionButtons();
    updateTimeKnobAppearance();

    addAndMakeVisible (eqComp);
    addAndMakeVisible (scope);

    setSize (630, 670);
}

_177delayyAudioProcessorEditor::~_177delayyAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void _177delayyAudioProcessorEditor::updateDivisionButtons()
{
    const bool synced  = syncButton.getToggleState();
    const int  current = (int) std::round (
        *audioProcessor.parms.getRawParameterValue ("division"));

    for (int i = 0; i < 9; ++i)
    {
        divButtons[i].setEnabled (synced);
        divButtons[i].setToggleState (synced && (i == current),
                                      juce::dontSendNotification);
    }
}

void _177delayyAudioProcessorEditor::updateTimeKnobAppearance()
{
    const bool synced = syncButton.getToggleState();

    // Dim the time knob arc and dot when synced
    timeSlider.setColour (juce::Slider::rotarySliderFillColourId,
                          synced ? juce::Colour (0x44F6B0BB)   // dimmed pink
                                 : juce::Colour (DelayLookAndFeel::colTextPrimary));
    timeSlider.setColour (juce::Slider::thumbColourId,
                          synced ? juce::Colour (0x44F6B0BB)
                                 : juce::Colour (DelayLookAndFeel::colPink));
    timeSlider.setColour (juce::Slider::textBoxTextColourId,
                          synced ? juce::Colour (0x44F6B0BB)
                                 : juce::Colour (DelayLookAndFeel::colTextPrimary));
   
    timeSlider.repaint();

    timeLabel.setColour (juce::Label::textColourId,
                         synced ? juce::Colour (0x88fdf6e4)
                                : juce::Colour (DelayLookAndFeel::colTextPrimary));
    timeLabel.repaint();
}

//==============================================================================
void _177delayyAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (DelayLookAndFeel::colBackground));

    g.setColour (juce::Colour (DelayLookAndFeel::colSurface));
    g.fillRect (0, 0, getWidth(), 100);
    
    // name
    g.setFont (laf.talinaFont.withHeight(20.0f).withExtraKerningFactor(0.1f));
    g.setColour (juce::Colour (DelayLookAndFeel::colTextPrimary));
    g.drawText ("JIWOO SON", 14, 8, 200, 20, juce::Justification::left);

    // Matcha cup image — right side of header
    auto matchaImg = juce::ImageCache::getFromMemory (
        BinaryData::matcha_cup_png, BinaryData::matcha_cup_pngSize);

    if (matchaImg.isValid())
    {
        const int imgH = 80;
        const int imgW = (int)(matchaImg.getWidth() * (imgH / (float)matchaImg.getHeight()));
        const int imgX = getWidth() - imgW - 40;
        const int imgY = 10;
        g.drawImage (matchaImg, imgX, imgY, imgW, imgH,
                     0, 0, matchaImg.getWidth(), matchaImg.getHeight());
    }
    //strawberry matcha (title)
    g.setFont (laf.talinaFont.withHeight(22.0f).withExtraKerningFactor(0.1f));
    g.setColour (juce::Colour (DelayLookAndFeel::colPink));
    g.drawText ("STRAWBERRY MATCHA", 0, 18, getWidth(), 30, juce::Justification::centred);
    
    //delay (title)
    g.setFont (laf.talinaFont.withHeight(55.0f).withExtraKerningFactor(0.0f));
    g.setColour (juce::Colour (DelayLookAndFeel::colPink));
    g.drawText ("DELAY", 0, 44, getWidth(), 60, juce::Justification::centred);

    g.setColour (juce::Colour (0xff5a6640));
    g.drawHorizontalLine (100, 1.0f, (float) getWidth());
    
    //eq filter
    g.setFont (laf.talinaFont.withHeight(20.0f).withExtraKerningFactor(0.1f));
    g.setColour (juce::Colour (DelayLookAndFeel::colTextPrimary));
    g.drawText ("EQ FILTER", 12, 360, 120, 16, juce::Justification::left);
}

void _177delayyAudioProcessorEditor::resized()
{
    const int w = getWidth();
    const int h = getHeight(); //unused

    // --- Tempo row ---
    const int tempoRowY = 108;
    const int tempoRowH = 28;
    const int btnW      = 48;
    const int divBtnW   = 52;
    const int btnGap    = 4;

    // button bounds
    syncButton.setBounds (8, tempoRowY, btnW, tempoRowH);
    tapButton .setBounds (8 + btnW + btnGap, tempoRowY, btnW, tempoRowH);

    const int divStartX = 8 + (btnW + btnGap) * 2 + 8;
    for (int i = 0; i < 9; ++i)
        divButtons[i].setBounds (divStartX + i * (divBtnW + btnGap),
                                 tempoRowY, divBtnW, tempoRowH);

    // --- Knobs ---
    const int knobSize = 120;
    const int knobY    = 190;
    const int sectionW = w / 3;
    const int labelH   = 40;
    const int labelY   = knobY - labelH - 4;

    for (int i = 0; i < 3; ++i)
    {
        juce::Slider* s = (i == 0) ? &timeSlider : (i == 1) ? &fdbkSlider : &mixSlider;
        juce::Label*  l = (i == 0) ? &timeLabel  : (i == 1) ? &fdbkLabel  : &mixLabel;
        const int cx = sectionW * i + sectionW / 2;
        s->setBounds (cx - knobSize / 2, knobY, knobSize, knobSize);
        l->setBounds (cx - knobSize / 2, labelY, knobSize, labelH);
    }

    // --- EQ ---
    const int eqY = 330;
    const int eqH = 218;
    eqComp.setBounds (8, eqY, w - 16, eqH);

    // --- Scope ---
    const int scopeY = eqY + eqH + 8;
    scope.setBounds (8, scopeY, w - 16, 80);
}
