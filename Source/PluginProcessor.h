/*
  ==============================================================================
    PluginProcessor.h
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "ScopeComponent.h"

class _177delayyAudioProcessor  : public juce::AudioProcessor
{
public:
    ScopeDataCollector scopeCollector;

    _177delayyAudioProcessor();
    ~_177delayyAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Tap tempo — called from UI thread
    void tapTempo();
    double getCurrentBPM() const;

    static constexpr double divisionMultipliers[9] = {
        1.0,          // 1/4
        0.5,          // 1/8
        0.25,         // 1/16
        1.5,          // dotted 1/4
        0.75,         // dotted 1/8
        0.375,        // dotted 1/16
        2.0/3.0,      // triplet 1/4
        1.0/3.0,      // triplet 1/8
        1.0/6.0       // triplet 1/16
    };

    juce::AudioProcessorValueTreeState parms;

private:
    std::atomic<float>* time_p     = nullptr;
    std::atomic<float>* fdbk_p     = nullptr;
    std::atomic<float>* mix_p      = nullptr;
    std::atomic<float>* lowcut_p   = nullptr;
    std::atomic<float>* highcut_p  = nullptr;
    std::atomic<float>* steep_p    = nullptr;
    std::atomic<float>* sync_p     = nullptr;
    std::atomic<float>* division_p = nullptr;

    float* del_buf  = nullptr;
    long   del_size = 0;
    long   del_mask = 0;
    long   w_pos    = 0;
    double sample_rate = 48000.0;

    juce::SmoothedValue<float> smoothed_delay_samples;

    // Filters — 4 stages each for 48dB/oct
    //IIRFilter is JUCE's legacy single-channel biquad filter, having an infinite impulse response
    juce::IIRFilter lowcutL[4],  lowcutR[4];
    juce::IIRFilter highcutL[4], highcutR[4];

    void updateFilters();

    // Tap tempo
    juce::int64 lastTapTime  = 0;
    double      tapInterval  = 0.0;  // ms
    int         tapCount     = 0;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (_177delayyAudioProcessor)
};
