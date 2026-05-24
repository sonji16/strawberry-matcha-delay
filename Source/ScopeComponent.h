#pragma once
#include <JuceHeader.h>

/*
  ==============================================================================

    ScopeComponent.h
    A lightweight waveform scope strip for the 177 Delay plugin.
    (Claude AI). this is just an additional visual GUI to make it cool
    Usage

  ==============================================================================
*/

//==============================================================================
// Lock-free ring buffer — processor writes, timer reads on the message thread.
class ScopeDataCollector
{
public:
    static constexpr int kBufferSize = 1024;

    void process (const float* samples, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
        {
            buffer[writePos.load (std::memory_order_relaxed)] = samples[i];
            writePos.store ((writePos.load (std::memory_order_relaxed) + 1) % kBufferSize,
                            std::memory_order_release);
        }
    }

    // Copy a snapshot of the ring buffer in time order into 'dest'.
    // 'dest' must have at least kBufferSize elements.
    void copySnapshot (float* dest) const noexcept
    {
        const int wp = writePos.load (std::memory_order_acquire);
        for (int i = 0; i < kBufferSize; ++i)
            dest[i] = buffer[(wp + i) % kBufferSize];
    }

private:
    float buffer[kBufferSize] = {};
    std::atomic<int> writePos { 0 };
};

//==============================================================================
class ScopeComponent : public juce::Component,
                       private juce::Timer
{
public:
    explicit ScopeComponent (ScopeDataCollector& collectorToUse)
        : collector (collectorToUse)
    {
        setOpaque (false);
        startTimerHz (30);
    }

    ~ScopeComponent() override { stopTimer(); }

    //==========================================================================
    void paint (juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();
        const float w = bounds.getWidth();
        const float h = bounds.getHeight();
        const float cy = h * 0.5f;

        // Background panel — dark brown
        g.setColour (juce::Colour (0xff3e2b1a));
        g.fillRoundedRectangle (bounds, 4.0f);

        // Border — dark brown rim
        g.setColour (juce::Colour (0xff4a3020));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 0.5f);

        // Centre line — muted brown
        g.setColour (juce::Colour (0xff5a3a28));
        g.drawHorizontalLine (juce::roundToInt (cy), 8.0f, w - 8.0f);

        // Waveform
        if (hasData)
        {
            const int numPoints = ScopeDataCollector::kBufferSize;
            const float xStep = (w - 16.0f) / float (numPoints - 1);

            juce::Path wave;
            bool started = false;

            for (int i = 0; i < numPoints; ++i)
            {
                const float px = 8.0f + i * xStep;
                const float py = cy - snapshot[i] * (cy - 4.0f);

                if (!started) { wave.startNewSubPath (px, py); started = true; }
                else           wave.lineTo (px, py);
            }

            // Glow layer (wider, dimmer) — pink
            g.setColour (juce::Colour (0x22F6B0BB));
            g.strokePath (wave, juce::PathStrokeType (3.5f,
                          juce::PathStrokeType::curved,
                          juce::PathStrokeType::rounded));

            // Core line — pink
            g.setColour (juce::Colour (0xffF6B0BB));
            g.strokePath (wave, juce::PathStrokeType (1.0f,
                          juce::PathStrokeType::curved,
                          juce::PathStrokeType::rounded));
        }
    }

    void resized() override {}

private:
    //==========================================================================
    void timerCallback() override
    {
        collector.copySnapshot (snapshot.data());
        hasData = true;
        repaint();
    }

    ScopeDataCollector& collector;
    std::array<float, ScopeDataCollector::kBufferSize> snapshot {};
    bool hasData = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScopeComponent)
};
