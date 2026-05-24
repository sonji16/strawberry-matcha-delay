#pragma once
#include <JuceHeader.h>

/*
  ==============================================================================
    EQComponent.h
    EQ graph gui settings. the EQ lowcut/highcut on the graph is linked with the
    actual lowcut/highcut knobs so it is interactive.
  ==============================================================================
*/

static constexpr uint32_t EQ_colPink        = 0xffF6B0BB;
static constexpr uint32_t EQ_colSurface     = 0xff6F4E37;
static constexpr uint32_t EQ_colTextPrimary = 0xfffdf6e4;
static constexpr uint32_t EQ_colBackground  = 0xff3e2b1a;
static constexpr uint32_t EQ_colBorder      = 0xff4a3020;

class EQComponent : public juce::Component,
                    private juce::Timer
{
public:

    void setKnobFont (const juce::Font& font)
    {
        lowcutKnob.setTextValueSuffix  ("");
        highcutKnob.setTextValueSuffix ("");
        knobFont = font;
        eqLaf.setFont (font);
        lowcutKnob .setLookAndFeel (&eqLaf);
        highcutKnob.setLookAndFeel (&eqLaf);
    }

    explicit EQComponent (juce::AudioProcessorValueTreeState& apvts,
                          const juce::Font& labelFont = juce::Font (10.0f),
                          const juce::Font& knobLabelFont = juce::Font (12.0f))
        : apvts (apvts), gridFont (labelFont), knobLabelFont (knobLabelFont)
    {
        lowcutSlider.setRange  (20.0, 20000.0, 1.0);
        highcutSlider.setRange (20.0, 20000.0, 1.0);
        lowcutSlider.setSkewFactorFromMidPoint  (1000.0);
        highcutSlider.setSkewFactorFromMidPoint (1000.0);
        lowcutSlider.setVisible  (false);
        highcutSlider.setVisible (false);
        addAndMakeVisible (lowcutSlider);
        addAndMakeVisible (highcutSlider);

        lowcutAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
                        (apvts, "lowcut",  lowcutSlider);
        highcutAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
                        (apvts, "highcut", highcutSlider);

        setupKnob (lowcutKnob, lowcutLabel, "LOWCUT");
        lowcutKnob.setValue (lowcutSlider.getValue(), juce::dontSendNotification);
        lowcutKnob.onValueChange = [this] {
            lowcutSlider.setValue (lowcutKnob.getValue(), juce::sendNotification);
            repaint();
        };
        lowcutSlider.onValueChange = [this] {
            lowcutKnob.setValue (lowcutSlider.getValue(), juce::dontSendNotification);
            repaint();
        };

        setupKnob (highcutKnob, highcutLabel, "HIGHCUT");
        highcutKnob.setValue (highcutSlider.getValue(), juce::dontSendNotification);
        highcutKnob.onValueChange = [this] {
            highcutSlider.setValue (highcutKnob.getValue(), juce::sendNotification);
            repaint();
        };
        highcutSlider.onValueChange = [this] {
            highcutKnob.setValue (highcutSlider.getValue(), juce::dontSendNotification);
            repaint();
        };

        steepButton.setButtonText ("48dB");
        steepButton.setColour (juce::ToggleButton::tickColourId,         juce::Colour (EQ_colSurface));
        steepButton.setColour (juce::ToggleButton::tickDisabledColourId, juce::Colour (0xff5a6640));
        steepButton.setColour (juce::Label::textColourId,                juce::Colour (EQ_colTextPrimary));
        addAndMakeVisible (steepButton);

        steepAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>
                       (apvts, "steep", steepButton);

        setOpaque (false);
        startTimerHz (30);
    }

    ~EQComponent() override {
        lowcutKnob .setLookAndFeel (nullptr);
        highcutKnob.setLookAndFeel (nullptr);
        stopTimer();
    }

    void paint (juce::Graphics& g) override
    {
        const float w      = (float) getWidth();
        const float graphH = (float) graphHeight;
        const float lcHz   = (float) lowcutSlider.getValue();
        const float hcHz   = (float) highcutSlider.getValue();
        const float lcX    = freqToX (lcHz, w);
        const float hcX    = freqToX (hcHz, w);

        g.setColour (juce::Colour (EQ_colBackground));
        g.fillRoundedRectangle (0, 0, w, graphH, 4.0f);
        g.setColour (juce::Colour (EQ_colBorder));
        g.drawRoundedRectangle (0.5f, 0.5f, w - 1.0f, graphH - 1.0f, 4.0f, 0.5f);

        g.setColour (juce::Colour (0xff5a3a28));
        for (float freq : { 100.0f, 1000.0f, 10000.0f })
            g.drawVerticalLine (juce::roundToInt (freqToX (freq, w)), 2.0f, graphH - 2.0f);

        {
            juce::Path curve;
            curve.startNewSubPath (0.0f, graphH - 2.0f);
            for (int px = 0; px < (int) w; ++px)
            {
                const float freq = xToFreq ((float) px, w);
                float gaindB = 0.0f;
                if (freq < lcHz)
                    gaindB = 80.0f * std::log10 (freq / lcHz);
                else if (freq > hcHz)
                    gaindB = 80.0f * std::log10 (hcHz / freq);

                const float gainNorm = juce::jlimit (0.0f, 1.0f, (gaindB + 18.0f) / 18.0f);
                const float py = (graphH - 4.0f) * (1.0f - gainNorm) + 2.0f;
                curve.lineTo ((float) px, py);
            }
            curve.lineTo (w, graphH - 2.0f);
            curve.closeSubPath();

            g.setColour (juce::Colour (0x33F6B0BB));
            g.fillPath (curve);
            g.setColour (juce::Colour (0xccF6B0BB));
            g.strokePath (curve, juce::PathStrokeType (1.5f));
        }

        drawHandle (g, lcX, graphH);
        drawHandle (g, hcX, graphH);

        // Grid freq labels — ivory, talina font
        g.setColour (juce::Colour (EQ_colTextPrimary));
        g.setFont (gridFont);
        for (auto& p : std::initializer_list<std::pair<float, const char*>>
             { {100.0f,"100"}, {1000.0f,"1k"}, {10000.0f,"10k"} })
            g.drawText (p.second,
                        (int) freqToX (p.first, w) + 2, (int) graphH - 14, 30, 12,
                        juce::Justification::left);

        g.setColour (juce::Colour (EQ_colPink));
        g.setFont (9.0f);
        g.drawText (juce::String ((int) lcHz) + " Hz", (int) lcX - 28, 2, 56, 12, juce::Justification::centred);
        g.drawText (juce::String ((int) hcHz) + " Hz", (int) hcX - 28, 2, 56, 12, juce::Justification::centred);
    }

    void resized() override
    {
        const int w        = getWidth();
        const int knobSize = 70;
        const int labelH   = 18;
        const int gap      = 8;
        const int knobTop  = graphHeight + gap+ 15;

        lowcutLabel .setBounds (w / 4 - knobSize / 2, knobTop,              knobSize, labelH);
        lowcutKnob  .setBounds (w / 4 - knobSize / 2, knobTop + labelH + 2, knobSize, knobSize);

        highcutLabel.setBounds (3 * w / 4 - knobSize / 2, knobTop,              knobSize, labelH);
        highcutKnob .setBounds (3 * w / 4 - knobSize / 2, knobTop + labelH + 2, knobSize, knobSize);

        steepButton.setBounds (w / 2 - 30, knobTop + 20, 60, 18);  // between the knobs vertically
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.position.y > (float) graphHeight) return;
        const float w   = (float) getWidth();
        const float lcX = freqToX ((float) lowcutSlider.getValue(),  w);
        const float hcX = freqToX ((float) highcutSlider.getValue(), w);
        draggingLC = (std::abs (e.position.x - lcX) < std::abs (e.position.x - hcX));
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (e.mouseDownPosition.y > (float) graphHeight) return;
        const float freq = xToFreq (e.position.x, (float) getWidth());
        if (draggingLC)
            lowcutSlider .setValue (juce::jlimit (20.0, 20000.0, (double) freq), juce::sendNotification);
        else
            highcutSlider.setValue (juce::jlimit (20.0, 20000.0, (double) freq), juce::sendNotification);
        repaint();
    }

private:
    struct EQLaf : public juce::LookAndFeel_V4
    {
        void setFont (const juce::Font& f) { font = f; }
        juce::Label* createSliderTextBox (juce::Slider& s) override
        {
            auto* l = LookAndFeel_V4::createSliderTextBox (s);
            l->setFont (font);
            return l;
        }
        juce::Font font;
    };

    EQLaf eqLaf;
    juce::Font knobFont;
    juce::Font gridFont = juce::Font(15.0f).withExtraKerningFactor(0.1f);
    juce::Font knobLabelFont = juce::Font(15.0f);

    static constexpr int graphHeight = 90;

    juce::AudioProcessorValueTreeState& apvts;

    juce::Slider lowcutSlider, highcutSlider;
    juce::Slider lowcutKnob,   highcutKnob;
    juce::Label  lowcutLabel,  highcutLabel;
    juce::ToggleButton steepButton;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lowcutAtt, highcutAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> steepAtt;

    bool draggingLC = true;

    void setupKnob (juce::Slider& knob, juce::Label& label, const juce::String& labelText)
    {
        knob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        knob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 20);
        knob.setRange (20.0, 20000.0, 1.0);
        knob.setSkewFactorFromMidPoint (1000.0);
        knob.setColour (juce::Slider::rotarySliderFillColourId,    juce::Colour (EQ_colPink));
        knob.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff5a6640));
        knob.setColour (juce::Slider::thumbColourId,               juce::Colour (EQ_colPink));
        knob.setColour (juce::Slider::textBoxTextColourId,         juce::Colour (EQ_colTextPrimary));
        knob.setColour (juce::Slider::textBoxBackgroundColourId,   juce::Colour (0x00000000));
        knob.setColour (juce::Slider::textBoxOutlineColourId,      juce::Colour (0x00000000));
        addAndMakeVisible (knob);

        label.setText (labelText, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setFont (knobLabelFont);
        label.setColour (juce::Label::textColourId, juce::Colour (EQ_colTextPrimary));
        addAndMakeVisible (label);
    }

    void timerCallback() override { repaint(); }

    static float freqToX (float freq, float width)
    {
        const float logMin = std::log10 (20.0f);
        const float logMax = std::log10 (20000.0f);
        return (std::log10 (juce::jlimit (20.0f, 20000.0f, freq)) - logMin)
               / (logMax - logMin) * width;
    }

    static float xToFreq (float x, float width)
    {
        const float logMin = std::log10 (20.0f);
        const float logMax = std::log10 (20000.0f);
        return std::pow (10.0f, logMin + (x / width) * (logMax - logMin));
    }

    void drawHandle (juce::Graphics& g, float x, float h)
    {
        g.setColour (juce::Colour (EQ_colPink));
        g.drawVerticalLine (juce::roundToInt (x), 4.0f, h - 2.0f);
        juce::Path diamond;
        diamond.addStar ({ x, h * 0.5f }, 4, 3.0f, 6.0f,
                         juce::MathConstants<float>::pi * 0.25f);
        g.fillPath (diamond);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQComponent)
};
