/*
  ==============================================================================
    PluginProcessor.cpp
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

constexpr double _177delayyAudioProcessor::divisionMultipliers[9];

_177delayyAudioProcessor::_177delayyAudioProcessor()
    : AudioProcessor (
        BusesProperties()
       #if ! JucePlugin_IsMidiEffect
        #if ! JucePlugin_IsSynth
         .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        #endif
         .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
       #endif
      ),
      parms (*this, nullptr, juce::Identifier ("APVTS20SC"),
      {
          //knobs
          std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("time",     1), "Time",     1.0f,  2000.0f, 150.0f),
          std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("fdbk",     1), "Feedback", 0.0f,  100.0f,  0.0f),
          std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("mix",      1), "Mix",      0.0f,  100.0f,  50.0f),
          std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("lowcut",   1), "Lowcut",   20.0f, 20000.0f,20.0f),
          std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("highcut",  1), "Highcut",  20.0f, 20000.0f,20000.0f),
          std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("steep",    1), "Steep",    0.0f,  1.0f,    0.0f),
          std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("sync",     1), "Sync",     0.0f,  1.0f,    0.0f),
          std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("division", 1), "Division", 0.0f,  8.0f,    0.0f),
      })
{
    //setting parameters
    mix_p      = parms.getRawParameterValue ("mix");
    time_p     = parms.getRawParameterValue ("time");
    fdbk_p     = parms.getRawParameterValue ("fdbk");
    lowcut_p   = parms.getRawParameterValue ("lowcut");
    highcut_p  = parms.getRawParameterValue ("highcut");
    steep_p    = parms.getRawParameterValue ("steep");
    sync_p     = parms.getRawParameterValue ("sync");
    division_p = parms.getRawParameterValue ("division");

    del_size = 524288;
    del_mask = del_size - 1;
    del_buf  = new float[del_size * 2]();
    w_pos    = 0;
}

_177delayyAudioProcessor::~_177delayyAudioProcessor()
{
    if (del_buf != nullptr)
        delete[] del_buf;
}

//==============================================================================
const juce::String _177delayyAudioProcessor::getName() const { return JucePlugin_Name; }
bool _177delayyAudioProcessor::acceptsMidi()  const { return false; }
bool _177delayyAudioProcessor::producesMidi() const { return false; }
bool _177delayyAudioProcessor::isMidiEffect() const { return false; }
double _177delayyAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int  _177delayyAudioProcessor::getNumPrograms()    { return 1; }
int  _177delayyAudioProcessor::getCurrentProgram() { return 0; }
void _177delayyAudioProcessor::setCurrentProgram (int) {}
const juce::String _177delayyAudioProcessor::getProgramName (int) { return {}; }
void _177delayyAudioProcessor::changeProgramName (int, const juce::String&) {}
bool _177delayyAudioProcessor::hasEditor() const { return true; }

//==============================================================================
// method for the tapping (tempo setting) button
void _177delayyAudioProcessor::tapTempo()
{
    //getting the value of the system clock
    const juce::int64 now = juce::Time::currentTimeMillis();

    if (lastTapTime > 0)
    {
        const double interval = (double)(now - lastTapTime);

        // Reset if gap is too long (> 3 seconds)
        if (interval > 3000.0)
        {
            tapCount    = 0;
            tapInterval = 0.0;
        }
        else
        {
            // Running average
            tapInterval = (tapInterval * tapCount + interval) / (tapCount + 1);
            tapCount++;

            // Clamp to valid range and set the time parameter
            const float newTime = (float) juce::jlimit (1.0, 2000.0, tapInterval);
            if (auto* param = parms.getParameter ("time"))
                param->setValueNotifyingHost (
                    param->convertTo0to1 (newTime));
        }
    }

    lastTapTime = now;
}
//method of the getting current bpm (setting tempo) from the DAW using API
double _177delayyAudioProcessor::getCurrentBPM() const
{
    if (auto* ph = getPlayHead())
        {
            if (auto pos = ph->getPosition())
                if (auto bpm = pos->getBpm())
                    if (*bpm > 0.0)
                        return *bpm;
        }
        return 120.0; // default if no DAW BPM available (e.g. standalone)
}

//==============================================================================
//changing the coefficients of the lowccut or highcut filters when any value update happens
void _177delayyAudioProcessor::updateFilters()
{
    //get the current lowcut and highcut value
    const float lc = *lowcut_p;
    const float hc = *highcut_p;
    //calculating the coefficients
    auto lcCoeffs = juce::IIRCoefficients::makeHighPass (sample_rate, lc);
    auto hcCoeffs = juce::IIRCoefficients::makeLowPass  (sample_rate, hc);

    //calculating the 4 stages for (12, 24, 36, 48 dB/oct) filtering
    for (int s = 0; s < 4; ++s)
    {
        lowcutL[s].setCoefficients  (lcCoeffs);
        lowcutR[s].setCoefficients  (lcCoeffs);
        highcutL[s].setCoefficients (hcCoeffs);
        highcutR[s].setCoefficients (hcCoeffs);
    }
}

void _177delayyAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    sample_rate = sampleRate;
    smoothed_delay_samples.reset (sampleRate, 0.05);

    for (int i = 0; i < del_size * 2; ++i)
        del_buf[i] = 0.0f;

    for (int s = 0; s < 4; ++s)
    {
        lowcutL[s].reset();  lowcutR[s].reset();
        highcutL[s].reset(); highcutR[s].reset();
    }

    updateFilters();
}

void _177delayyAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool _177delayyAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif
    return true;
}
#endif

void _177delayyAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    updateFilters();

    // --- Tempo sync ---
    const bool synced = (*sync_p > 0.5f);
    if (synced)
    {
        if (auto* ph = getPlayHead())
        {
            juce::AudioPlayHead::CurrentPositionInfo pos;
            if (ph->getCurrentPosition (pos) && pos.bpm > 0.0)
            {
                const double bpm        = pos.bpm;
                const double beatMs     = 60000.0 / bpm;  // ms per beat (1/4 note)
                const int    divIndex   = juce::jlimit (0, 8, (int) std::round (*division_p));
                const double delayMs    = beatMs * divisionMultipliers[divIndex];
                const float  delaySamps = (float)(delayMs * 0.001 * sample_rate);
                //setting up the gliding instead of drastic change to prevent clicking
                smoothed_delay_samples.setTargetValue (delaySamps);
            }
        }
    }
    else
    {
        // Free mode — use time parameter directly
        const float delaySamps = (float)(*time_p * 0.001 * sample_rate);
        smoothed_delay_samples.setTargetValue (delaySamps);
    }

    const int totalIn  = getTotalNumInputChannels();
    const int totalOut = getTotalNumOutputChannels();
    const bool steep   = (*steep_p > 0.5f);

    for (int i = totalIn; i < totalOut; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    auto* in_l  = buffer.getReadPointer (0);
    auto* out_l = buffer.getWritePointer (0);
    auto* in_r  = (totalIn  > 1) ? buffer.getReadPointer  (1) : buffer.getReadPointer  (0);
    auto* out_r = (totalOut > 1) ? buffer.getWritePointer (1) : buffer.getWritePointer (0);

    for (int n = 0; n < buffer.getNumSamples(); ++n)
    {
        long delay_samples = (long) smoothed_delay_samples.getNextValue();

        float in_l_s = in_l[n];
        float in_r_s = in_r[n];

        long r_pos = del_mask & (w_pos + delay_samples);

        float del_out_l = del_buf[r_pos];
        float del_out_r = del_buf[del_size + r_pos];

        // Stage 0 always (12dB/oct)
        del_out_l = lowcutL[0].processSingleSampleRaw (del_out_l);
        del_out_r = lowcutR[0].processSingleSampleRaw (del_out_r);
        del_out_l = highcutL[0].processSingleSampleRaw (del_out_l);
        del_out_r = highcutR[0].processSingleSampleRaw (del_out_r);

        // Stages 1-3 for 48dB/oct
        if (steep)
        {
            del_out_l = lowcutL[1].processSingleSampleRaw (del_out_l);
            del_out_r = lowcutR[1].processSingleSampleRaw (del_out_r);
            del_out_l = highcutL[1].processSingleSampleRaw (del_out_l);
            del_out_r = highcutR[1].processSingleSampleRaw (del_out_r);
            del_out_l = lowcutL[2].processSingleSampleRaw (del_out_l);
            del_out_r = lowcutR[2].processSingleSampleRaw (del_out_r);
            del_out_l = highcutL[2].processSingleSampleRaw (del_out_l);
            del_out_r = highcutR[2].processSingleSampleRaw (del_out_r);
            del_out_l = lowcutL[3].processSingleSampleRaw (del_out_l);
            del_out_r = lowcutR[3].processSingleSampleRaw (del_out_r);
            del_out_l = highcutL[3].processSingleSampleRaw (del_out_l);
            del_out_r = highcutR[3].processSingleSampleRaw (del_out_r);
        }
        // crossfading dry vs wet depending on the 'mix' knob value
        out_l[n] = in_l_s + (*mix_p * 0.01f * (del_out_l - in_l_s));
        out_r[n] = in_r_s + (*mix_p * 0.01f * (del_out_r - in_r_s));

        //adding feedback to the delay buffer 
        del_buf[w_pos]            = in_l_s + (del_out_l * *fdbk_p * 0.01f);
        del_buf[del_size + w_pos] = in_r_s + (del_out_r * *fdbk_p * 0.01f);

        w_pos--;
        w_pos &= del_mask;
    }

    scopeCollector.process (buffer.getReadPointer (0), buffer.getNumSamples());
}

juce::AudioProcessorEditor* _177delayyAudioProcessor::createEditor()
{
    return new _177delayyAudioProcessorEditor (*this);
}

void _177delayyAudioProcessor::getStateInformation (juce::MemoryBlock&) {}
void _177delayyAudioProcessor::setStateInformation (const void*, int) {}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new _177delayyAudioProcessor();
}
