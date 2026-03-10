#pragma once

#include <JuceHeader.h>

class EffectsChain
{
public:
    void prepare (double sampleRate, int blockSize);
    void process (juce::AudioBuffer<float>& buffer, int startSample, int numSamples);
    void release();

    // Called from UI thread
    void setRevSize     (float v) { atomicRevSize     = v; }
    void setRevMix      (float v) { atomicRevMix      = v; }
    void setDelTime     (float v) { atomicDelTime      = v; }
    void setDelFeedback (float v) { atomicDelFeedback  = v; }
    void setDelMix      (float v) { atomicDelMix       = v; }

    EffectsChain() = default;
    
private:
    double sampleRate { 44100.0 };

    // Delay
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLineL { 192000 };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLineR { 192000 };

    float lastDelTime { 0.0f };
    
    // Reverb
    juce::dsp::Reverb reverb;

    std::atomic<float> atomicRevSize     { 0.5f };
    std::atomic<float> atomicRevMix      { 0.0f };
    std::atomic<float> atomicDelTime     { 0.3f };
    std::atomic<float> atomicDelFeedback { 0.4f };
    std::atomic<float> atomicDelMix      { 0.0f };

};
