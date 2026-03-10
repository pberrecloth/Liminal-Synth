#include "EffectsChain.h"

void EffectsChain::prepare (double sr, int blockSize)
{
    sampleRate = sr;

    juce::dsp::ProcessSpec stereoSpec;
    stereoSpec.sampleRate       = sr;
    stereoSpec.maximumBlockSize = (juce::uint32) blockSize;
    stereoSpec.numChannels      = 2;

    juce::dsp::ProcessSpec monoSpec;
    monoSpec.sampleRate       = sr;
    monoSpec.maximumBlockSize = (juce::uint32) blockSize;
    monoSpec.numChannels      = 1;

    delayLineL.prepare (monoSpec);
    delayLineR.prepare (monoSpec);
    delayLineL.setMaximumDelayInSamples (192000);
    delayLineR.setMaximumDelayInSamples (192000);

    reverb.prepare (stereoSpec);

    juce::dsp::Reverb::Parameters reverbParams;
    reverbParams.roomSize   = 0.5f;
    reverbParams.damping    = 0.5f;
    reverbParams.wetLevel   = 0.0f;
    reverbParams.dryLevel   = 1.0f;
    reverbParams.width      = 1.0f;
    reverb.setParameters (reverbParams);
}

void EffectsChain::process (juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    if (buffer.getNumChannels() < 2) return;

    auto* left  = buffer.getWritePointer (0, startSample);
    auto* right = buffer.getWritePointer (1, startSample);

    // --- Delay ---
    float targetDelTime = juce::jlimit (1.0f,
                                        (float) delayLineL.getMaximumDelayInSamples(),
                                        atomicDelTime.load() * (float) sampleRate);
    float delTimeStep = (targetDelTime - lastDelTime) / (float) numSamples;
    float currentDelTime = lastDelTime;
    lastDelTime = targetDelTime;

    float feedback = atomicDelFeedback.load();
    float delMix   = atomicDelMix.load();

    for (int i = 0; i < numSamples; ++i)
    {
        currentDelTime += delTimeStep;
        delayLineL.setDelay (currentDelTime);
        delayLineR.setDelay (currentDelTime);

        float dryL = left[i];
        float dryR = right[i];

        float wetL = delayLineL.popSample (0);
        float wetR = delayLineR.popSample (0);

        delayLineL.pushSample (0, dryL + wetL * feedback);
        delayLineR.pushSample (0, dryR + wetR * feedback);

        left[i]  = dryL + wetL * delMix;
        right[i] = dryR + wetR * delMix;
    }

    // --- Reverb ---
    float revSize = atomicRevSize.load();
    float revMix  = atomicRevMix.load();

    juce::dsp::Reverb::Parameters reverbParams;
    reverbParams.roomSize   = revSize;
    reverbParams.damping    = 0.5f;
    reverbParams.wetLevel   = revMix;
    reverbParams.dryLevel   = 1.0f;
    reverbParams.width      = 1.0f;
    reverb.setParameters (reverbParams);

    juce::dsp::AudioBlock<float> block (buffer, (size_t) startSample);
    block = block.getSubBlock (0, (size_t) numSamples);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    reverb.process (ctx);
}

void EffectsChain::release()
{
    delayLineL.reset();
    delayLineR.reset();
    reverb.reset();
}
