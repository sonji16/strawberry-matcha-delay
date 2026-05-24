#pragma once
#include <JuceHeader.h>

/*
  ==============================================================================

    DelayLookAndFeel.h
    Custom LookAndFeel for the 177 Delay plugin --> just some GUI settings, mainly
     for color, font, knobs, labels
    Called in PluginEditor.
  ==============================================================================
*/

class DelayLookAndFeel : public juce::LookAndFeel_V4
{
public:
    //==========================================================================
    // Colours
    static constexpr uint32_t colBackground  = 0xff838f58;  // olive green  — plugin panel
    static constexpr uint32_t colSurface     = 0xff6F4E37;  // brown        — knob body
    static constexpr uint32_t colBorder      = 0xff4a3020;  // dark brown   — knob rim
    static constexpr uint32_t colPink        = 0xffF6B0BB;  // pink         — arc / dot / text
    static constexpr uint32_t colPinkDim     = 0x88F6B0BB;  // pink 53%     — highlight / selection
    static constexpr uint32_t colTextPrimary = 0xfffdf6e4;  // ivory  — labels
    static constexpr uint32_t colTextMuted   = 0xff2a2e1a;  // dark olive   — secondary labels
    static constexpr uint32_t colHighlight   = 0x18ffffff;  // white glint  — inner knob glint
    
    // uploaded font file from my laptop to the JUCE folder
    juce::Font talinaFont;
    
    DelayLookAndFeel()
    {
        //setting up my custom font
        auto typeface = juce::Typeface::createSystemTypefaceFor(
            BinaryData::Talina_DEMO_otf,
            BinaryData::Talina_DEMO_otfSize);
        talinaFont = juce::Font(typeface);

        // Window background
        setColour (juce::ResizableWindow::backgroundColourId,
                   juce::Colour (colBackground));

        // Slider thumb / track
        setColour (juce::Slider::thumbColourId,
                   juce::Colour (colPink));
        setColour (juce::Slider::rotarySliderFillColourId,
                   juce::Colour (colPink));
        setColour (juce::Slider::rotarySliderOutlineColourId,
                   juce::Colour (0xff5a6640));  // muted olive — track bg arc
        setColour (juce::Slider::textBoxTextColourId,
                   juce::Colour (colPink));
        setColour (juce::Slider::textBoxBackgroundColourId,
                   juce::Colour (0xff3e2b1a));  // dark brown  — value readout bg
        setColour (juce::Slider::textBoxOutlineColourId,
                   juce::Colour (colPink));  // dark brown  — value readout border
        setColour (juce::Slider::textBoxHighlightColourId,
                   juce::Colour (colPinkDim));

        // Labels
        setColour (juce::Label::textColourId,
                   juce::Colour (colTextMuted));
        setColour (juce::Label::backgroundColourId, juce::Colour (0x00000000));
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0x00000000));
    }

    //==========================================================================
    // Rotary knob
    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider& /*slider*/) override
    {
        const float cx     = x + width  * 0.5f;
        const float cy     = y + height * 0.5f;
        const float radius = juce::jmin (width, height) * 0.5f - 4.0f;

        // --- Track arc (muted olive background arc) ---
        {
            juce::Path trackArc;
            trackArc.addCentredArc (cx, cy, radius, radius, 0.0f,
                                    rotaryStartAngle, rotaryEndAngle, true);
            g.setColour (juce::Colour (0xff5a6640));
            g.strokePath (trackArc, juce::PathStrokeType (3.0f,
                          juce::PathStrokeType::curved,
                          juce::PathStrokeType::rounded));
        }

        // --- Fill arc (pink, proportional) ---
        {
            //angle change
            const float angle = rotaryStartAngle
                                + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
            juce::Path fillArc;
            fillArc.addCentredArc (cx, cy, radius, radius, 0.0f,
                                   rotaryStartAngle, angle, true);
            g.setColour (juce::Colour (colPink));
            g.strokePath (fillArc, juce::PathStrokeType (3.0f,
                          juce::PathStrokeType::curved,
                          juce::PathStrokeType::rounded));
        }

        // --- Knob body (brown) ---
        const float bodyRadius = radius * 0.72f;
        g.setColour (juce::Colour (colSurface));
        g.fillEllipse (cx - bodyRadius, cy - bodyRadius,
                       bodyRadius * 2.0f, bodyRadius * 2.0f);

        g.setColour (juce::Colour (colBorder));
        g.drawEllipse (cx - bodyRadius, cy - bodyRadius,
                       bodyRadius * 2.0f, bodyRadius * 2.0f, 1.0f);

        // --- Inner highlight (top-left glint) ---
        {
            juce::Path glint;
            glint.addCentredArc (cx, cy - bodyRadius * 0.25f,
                                 bodyRadius * 0.45f, bodyRadius * 0.28f,
                                 0.0f,
                                 juce::MathConstants<float>::pi * 1.1f,
                                 juce::MathConstants<float>::pi * 1.9f,
                                 true);
            g.setColour (juce::Colour (colHighlight));
            g.strokePath (glint, juce::PathStrokeType (1.0f));
        }

        // --- Indicator dot (pink) ---
        {
            const float angle = rotaryStartAngle
                                + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle)
                                - juce::MathConstants<float>::halfPi;
            const float dotR   = bodyRadius * 0.82f;
            const float dotCx  = cx + dotR * std::cos (angle);
            const float dotCy  = cy + dotR * std::sin (angle);
            const float dotSz  = 4.0f;

            g.setColour (juce::Colour (colPink));
            g.fillEllipse (dotCx - dotSz, dotCy - dotSz, dotSz * 2.0f, dotSz * 2.0f);
        }
    }

    //==========================================================================
    // Text box inside the slider (value readout)
    juce::Label* createSliderTextBox (juce::Slider& slider) override
    {
        auto* label = LookAndFeel_V4::createSliderTextBox (slider);
        label->setFont (talinaFont.withHeight(15.0f));
        label->setJustificationType (juce::Justification::centred);
        return label;
    }

    juce::Font getLabelFont (juce::Label&) override
    {
        return talinaFont.withHeight(20.0f).withExtraKerningFactor(0.1f);
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& button,
                         bool /*isMouseOverButton*/, bool /*isButtonDown*/) override
    {
        g.setFont (talinaFont.withHeight (13.0f).withExtraKerningFactor (0.1f));
        g.setColour (button.findColour (button.getToggleState()
                                        ? juce::TextButton::textColourOnId
                                        : juce::TextButton::textColourOffId));
        g.drawFittedText (button.getButtonText(),
                          button.getLocalBounds(),
                          juce::Justification::centred, 1);
    }

    void drawTextEditorOutline (juce::Graphics& g, int width, int height,
                                juce::TextEditor& /*editor*/) override
    {
        g.setColour (juce::Colour (colPink));
        g.drawRect (0, 0, width, height, 1);
    }

    void drawLabel (juce::Graphics& g, juce::Label& label) override
    {
        g.fillAll (label.findColour (juce::Label::backgroundColourId));
        if (! label.isBeingEdited())
        {
            g.setColour (label.findColour (juce::Label::textColourId));
            g.setFont (label.getFont());
            g.drawFittedText (label.getText(),
                              label.getLocalBounds(),
                              label.getJustificationType(),
                              1, 1.0f);
        }
    }
    //==========================================================================

};
