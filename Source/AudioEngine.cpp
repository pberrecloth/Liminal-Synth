#include "AudioEngine.h"

AudioEngine::AudioEngine() {}

void AudioEngine::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    if (sampleRate <= 0) return;  // ← GUARD
    midiCollector.reset (sampleRate);
       
    synthesiser.setCurrentPlaybackSampleRate (sampleRate);
    
    smoothedMasterLeft .reset (sampleRate, 0.2);
    smoothedMasterRight.reset (sampleRate, 0.2);
    smoothedMasterLeft .setCurrentAndTargetValue (0.8f);
    smoothedMasterRight.setCurrentAndTargetValue (0.8f);

    synthesiser.clearVoices();
    synthesiser.clearSounds();

    for (int i = 0; i < 8; ++i)
    {
        auto* voice = new SynthVoice();
        voice->prepareVoice (sampleRate, samplesPerBlockExpected);
        synthesiser.addVoice (voice);
    }

    synthesiser.addSound (new SynthSound());

    // Prepare Filters
    juce::dsp::ProcessSpec filter3Spec;
    filter3Spec.sampleRate       = sampleRate;
    filter3Spec.maximumBlockSize = (juce::uint32) samplesPerBlockExpected;
    filter3Spec.numChannels      = 2;
    filter3.prepare (filter3Spec);
    filter3.setType (juce::dsp::StateVariableTPTFilterType::highpass);
    filter3.setCutoffFrequency (20.0f);
    filter3.setResonance (0.7f);

    effects.prepare (sampleRate, samplesPerBlockExpected);
    
    isReady = true;
}

void AudioEngine::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    if (! isReady)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }
    
    bufferToFill.clearActiveBufferRegion();

    juce::MidiBuffer midiBuffer;
    midiCollector.removeNextBlockOfMessages (midiBuffer, bufferToFill.numSamples);

    synthesiser.renderNextBlock (*bufferToFill.buffer,
                                  midiBuffer,
                                  bufferToFill.startSample,
                                  bufferToFill.numSamples);

    if (atomicFilter3On.load())
    {
        float targetFreq  = atomicFilter3Freq.load();
        float freqStep    = (targetFreq - lastFilter3Freq) / (float) bufferToFill.numSamples;
        float currentFreq = lastFilter3Freq;
        lastFilter3Freq   = targetFreq;

        auto* left  = bufferToFill.buffer->getWritePointer (0, bufferToFill.startSample);
        auto* right = bufferToFill.buffer->getNumChannels() >= 2
                      ? bufferToFill.buffer->getWritePointer (1, bufferToFill.startSample)
                      : nullptr;

        for (int i = 0; i < bufferToFill.numSamples; ++i)
        {
            currentFreq += freqStep;
            filter3.setCutoffFrequency (juce::jlimit (20.0f, 20000.0f, currentFreq));
            left[i] = filter3.processSample (0, left[i]);
            if (right != nullptr)
                right[i] = filter3.processSample (1, right[i]);
        }
    }

    effects.process (*bufferToFill.buffer, bufferToFill.startSample, bufferToFill.numSamples);

    float vol = atomicMasterVol.load();
    float pan = atomicMasterPan.load();

    float leftTarget  = vol * juce::jmin (1.0f, 1.0f - pan);
    float rightTarget = vol * juce::jmin (1.0f, 1.0f + pan);

    smoothedMasterLeft .setTargetValue (leftTarget  < 0.001f ? 0.0f : leftTarget);
    smoothedMasterRight.setTargetValue (rightTarget < 0.001f ? 0.0f : rightTarget);

    if (bufferToFill.buffer->getNumChannels() >= 2)
    {
        auto* left  = bufferToFill.buffer->getWritePointer (0, bufferToFill.startSample);
        auto* right = bufferToFill.buffer->getWritePointer (1, bufferToFill.startSample);
        for (int i = 0; i < bufferToFill.numSamples; ++i)
        {
            left[i]  *= smoothedMasterLeft .getNextValue();
            right[i] *= smoothedMasterRight.getNextValue();
        }
    }
    else
    {
        auto* mono = bufferToFill.buffer->getWritePointer (0, bufferToFill.startSample);
        for (int i = 0; i < bufferToFill.numSamples; ++i)
            mono[i] *= smoothedMasterLeft.getNextValue();
    }
}

void AudioEngine::releaseResources()
{
    synthesiser.clearVoices();
    synthesiser.clearSounds();
    effects.release();
}
