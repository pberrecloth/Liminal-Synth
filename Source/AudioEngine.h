#pragma once

#include <JuceHeader.h>
#include "SynthVoice.h"
#include "EffectsChain.h"

class AudioEngine
{
public:
    AudioEngine();

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate);
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill);
    void releaseResources();

    void setMasterVol   (float v) { atomicMasterVol   = v; }
    void setMasterPan   (float p) { atomicMasterPan   = p; }
    void setFilter3On   (bool b)  { atomicFilter3On   = b; }
    void setFilter3Freq (float f) { atomicFilter3Freq = f; }

    // Direct MIDI — called from message thread, lock-safe
    void addMidiMessage (const juce::MidiMessage& message)
    {
        const juce::ScopedLock sl (midiLock);
        incomingMidi.addEvent (message, 0);
    }

    juce::Synthesiser synthesiser;
    EffectsChain      effects;

    std::atomic<bool> isReady { false };

private:
    juce::CriticalSection midiLock;
    juce::MidiBuffer      incomingMidi;

    juce::LinearSmoothedValue<float> smoothedMasterLeft;
    juce::LinearSmoothedValue<float> smoothedMasterRight;
    std::atomic<float>               atomicMasterVol  { 0.8f };
    std::atomic<float>               atomicMasterPan  { 0.0f };

    juce::dsp::StateVariableTPTFilter<float> filter3;
    std::atomic<float>                       atomicFilter3Freq { 20.0f };
    std::atomic<bool>                        atomicFilter3On   { false };
    float                                    lastFilter3Freq   { 20.0f };

    int callCount { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioEngine)
};
