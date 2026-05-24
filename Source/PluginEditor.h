/*
  ==============================================================================
    PluginEditor.h
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "DelayLookAndFeel.h"
#include "ScopeComponent.h"
#include "EQComponent.h"

class _177delayyAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    _177delayyAudioProcessorEditor (_177delayyAudioProcessor&);
    ~_177delayyAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    _177delayyAudioProcessor& audioProcessor;
    DelayLookAndFeel laf;

    // Main knobs
    juce::Slider timeSlider, fdbkSlider, mixSlider;
    juce::Label  timeLabel,  fdbkLabel,  mixLabel;

    juce::AudioProcessorValueTreeState::SliderAttachment timeSA, fdbkSA, mixSA;

    // Tempo sync row
    juce::TextButton syncButton  { "SYNC" };
    juce::TextButton tapButton   { "TAP" };

    // Division buttons: 1/4 1/8 1/16 d1/4 d1/8 d1/16 t1/4 t1/8 t1/16
    juce::TextButton divButtons[9];
    static const char* divLabels[9];

    juce::AudioProcessorValueTreeState::ButtonAttachment syncAtt;

    ScopeComponent scope;
    EQComponent    eqComp;

    void updateDivisionButtons();
    void updateTimeKnobAppearance();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (_177delayyAudioProcessorEditor)
};
