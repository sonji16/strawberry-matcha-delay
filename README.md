🍓🍵 Strawberry Matcha Delay🍓🍵 
A custom audio delay plugin built in JUCE / C++, featuring a hand-designed user interface and a real-time signal scope. 
This project focuses on pairing functional audio DSP with a polished, original front-end design.

<img width="633" height="697" alt="Screenshot 2026-05-12 at 1 38 37 PM" src="https://github.com/user-attachments/assets/cae3f87f-a299-4974-bc42-94e347981c26" />

**Overview**
Strawberry Matcha Delay is a stereo delay effect with three core controls and a live waveform display. 
It was built to explore both sides of audio plugin development: the signal processing and the visual/interaction design.

**Features**
Time — adjusts the delay time
Feedback — controls how much of the delayed signal is fed back into the effect
Mix — blends dry (input) and wet (delayed) signal
Lowcut - a lowcut filter 12dB/oct by default. 
Highcut - a highcut filter 12dB/oct by default.
Frequency window - a visualization of the lowcut and highcut filters among the frequency range. Interactive with 
the lowcut and highcut knobs.
Real-time signal scope — live visualization of the audio waveform
Custom UI — implemented with a custom JUCE LookAndFeel

**Built With**
C++
JUCE framework
Projucer (project management / build configuration)

The Source/ folder contains the code. 
The .zip of the VST3 version is provided.  

