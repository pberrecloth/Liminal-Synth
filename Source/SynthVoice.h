#pragma once

#include <JuceHeader.h>

//==============================================================================
class SynthSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote    (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

//==============================================================================
class SynthVoice : public juce::SynthesiserVoice
{
public:
    enum class Waveform { Sine, Square, Saw, Triangle, Noise };

    SynthVoice()
    {
        oscA.initialise ([] (float x) { return std::sin (x); }, 128);
        oscB.initialise ([] (float x) { return std::sin (x); }, 128);
        oscC.initialise ([] (float x) { return std::sin (x); }, 128);
        oscD.initialise ([] (float x) { return std::sin (x); }, 128);
        oscE.initialise ([] (float x) { return std::sin (x); }, 128);
        oscF.initialise ([] (float x) { return std::sin (x); }, 128);
    }

    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<SynthSound*> (sound) != nullptr;
    }

    void prepareVoice (double sampleRate, int blockSize)
    {
        currentSampleRate = sampleRate;

        juce::dsp::ProcessSpec spec;
        spec.sampleRate       = sampleRate;
        spec.maximumBlockSize = (juce::uint32) blockSize;
        spec.numChannels      = 1;

        oscA.prepare (spec); oscB.prepare (spec);
        oscC.prepare (spec); oscD.prepare (spec);
        oscE.prepare (spec); oscF.prepare (spec);

        gain.prepare (spec);
        gain.setGainLinear (0.3f);

        // FILTERS - Safe Cut Off
        float safeCutoff = juce::jmin (20000.0f, (float)(sampleRate * 0.49));

        // Filter 1
        filter1.prepare (spec);
        filter1series.prepare (spec);
        filter1.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        filter1series.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        filter1.setCutoffFrequency (safeCutoff);
        filter1series.setCutoffFrequency (safeCutoff);
        filter1.setResonance (0.7f);
        filter1series.setResonance (0.7f);
        smoothedCutoff1.reset (sampleRate, 0.05); smoothedCutoff1.setCurrentAndTargetValue (safeCutoff);
        smoothedRes1   .reset (sampleRate, 0.05); smoothedRes1   .setCurrentAndTargetValue (0.7f);
        smoothedDrive1 .reset (sampleRate, 0.05); smoothedDrive1 .setCurrentAndTargetValue (1.0f);

        
        // Filter 2
        filter2.prepare (spec);
        filter2series.prepare (spec);
        filter2.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        filter2series.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        filter2.setCutoffFrequency (safeCutoff);
        filter2series.setCutoffFrequency (safeCutoff);
        filter2.setResonance (0.7f);
        filter2series.setResonance (0.7f);
        smoothedCutoff2.reset (sampleRate, 0.05); smoothedCutoff2.setCurrentAndTargetValue (safeCutoff);
        smoothedRes2   .reset (sampleRate, 0.05); smoothedRes2   .setCurrentAndTargetValue (0.7f);
        smoothedDrive2 .reset (sampleRate, 0.05); smoothedDrive2 .setCurrentAndTargetValue (1.0f);

        filterEnv2.setSampleRate (sampleRate);
        juce::ADSR::Parameters f2p;
        f2p.attack = 0.1f; f2p.decay = 0.2f; f2p.sustain = 0.7f; f2p.release = 0.4f;
        filterEnv2.setParameters (f2p);

        // Amp ADSR
        adsr.setSampleRate (sampleRate);
        juce::ADSR::Parameters p;
        p.attack = 0.1f; p.decay = 0.2f; p.sustain = 0.7f; p.release = 0.4f;
        adsr.setParameters (p);

        // OSC 1 smoothing
        smoothedGain    .reset (sampleRate, 0.05);
        smoothedPanLeft .reset (sampleRate, 0.05);
        smoothedPanRight.reset (sampleRate, 0.05);
        smoothedMorph   .reset (sampleRate, 0.05);

        // OSC 2 smoothing
        smoothedGain2    .reset (sampleRate, 0.05); smoothedGain2    .setCurrentAndTargetValue (0.5f);
        smoothedPanLeft2 .reset (sampleRate, 0.05); smoothedPanLeft2 .setCurrentAndTargetValue (1.0f);
        smoothedPanRight2.reset (sampleRate, 0.05); smoothedPanRight2.setCurrentAndTargetValue (1.0f);
        smoothedMorph2   .reset (sampleRate, 0.05);

        // OSC 3 smoothing
        smoothedGain3    .reset (sampleRate, 0.05); smoothedGain3    .setCurrentAndTargetValue (0.5f);
        smoothedPanLeft3 .reset (sampleRate, 0.05); smoothedPanLeft3 .setCurrentAndTargetValue (1.0f);
        smoothedPanRight3.reset (sampleRate, 0.05); smoothedPanRight3.setCurrentAndTargetValue (1.0f);
        smoothedMorph3   .reset (sampleRate, 0.05);

        // Filter env 1
        filterEnv1.setSampleRate (sampleRate);
        juce::ADSR::Parameters fp;
        fp.attack = 0.1f; fp.decay = 0.2f; fp.sustain = 0.7f; fp.release = 0.4f;
        filterEnv1.setParameters (fp);
    }

    //==========================================================================
    // OSC 1
    void setWaveformA (Waveform w) { isNoiseA = (w == Waveform::Noise); if (!isNoiseA) applyWaveform (oscA, w); }
    void setWaveformB (Waveform w) { isNoiseB = (w == Waveform::Noise); if (!isNoiseB) applyWaveform (oscB, w); }
    void setMorph  (float m) { smoothedMorph .setTargetValue (juce::jlimit (0.0f, 1.0f, m)); }
    void setVolume (float v) { smoothedGain  .setTargetValue (v * 0.5f); }
    void setPan (float p)
    {
        smoothedPanLeft .setTargetValue (juce::jmin (1.0f, 1.0f - p));
        smoothedPanRight.setTargetValue (juce::jmin (1.0f, 1.0f + p));
    }

    // OSC 2
    void setWaveformC (Waveform w) { isNoiseC = (w == Waveform::Noise); if (!isNoiseC) applyWaveform (oscC, w); }
    void setWaveformD (Waveform w) { isNoiseD = (w == Waveform::Noise); if (!isNoiseD) applyWaveform (oscD, w); }
    void setMorph2  (float m) { smoothedMorph2.setTargetValue (juce::jlimit (0.0f, 1.0f, m)); }
    void setVolume2 (float v) { smoothedGain2 .setTargetValue (v * 0.5f); }
    void setPan2 (float p)
    {
        smoothedPanLeft2 .setTargetValue (juce::jmin (1.0f, 1.0f - p));
        smoothedPanRight2.setTargetValue (juce::jmin (1.0f, 1.0f + p));
    }

    // OSC 3
    void setWaveformE (Waveform w) { isNoiseE = (w == Waveform::Noise); if (!isNoiseE) applyWaveform (oscE, w); }
    void setWaveformF (Waveform w) { isNoiseF = (w == Waveform::Noise); if (!isNoiseF) applyWaveform (oscF, w); }
    void setMorph3  (float m) { smoothedMorph3.setTargetValue (juce::jlimit (0.0f, 1.0f, m)); }
    void setVolume3 (float v) { smoothedGain3 .setTargetValue (v * 0.5f); }
    void setPan3 (float p)
    {
        smoothedPanLeft3 .setTargetValue (juce::jmin (1.0f, 1.0f - p));
        smoothedPanRight3.setTargetValue (juce::jmin (1.0f, 1.0f + p));
    }

    void setAdsrParams (const juce::ADSR::Parameters& params) { adsr.setParameters (params); }

    //==========================================================================
    // LFO 1
    void setLfo1Rate  (float hz)  { lfo1Rate    = hz; }
    void setLfo1Depth (float d) { lfo1Depth = d; DBG ("voice setLfo1Depth=" << d); }
    void setLfo1Delay (float ms)  { lfo1DelayMs = ms; }
    void setLfo1Shape (int shape) { lfo1Shape   = shape; }

    //==========================================================================
    void setTuning (int oct, int semi, float fine)
    {
        tuningOct = oct; tuningSemi = semi; tuningFine = fine;
        if (isVoiceActive()) retune (oscA, oscB, currentMidiNote, oct, semi, fine);
    }

    void setTuning2 (int oct, int semi, float fine)
    {
        tuningOct2 = oct; tuningSemi2 = semi; tuningFine2 = fine;
        if (isVoiceActive()) retune (oscC, oscD, currentMidiNote, oct, semi, fine);
    }

    void setTuning3 (int oct, int semi, float fine)
    {
        tuningOct3 = oct; tuningSemi3 = semi; tuningFine3 = fine;
        if (isVoiceActive()) retune (oscE, oscF, currentMidiNote, oct, semi, fine);
    }

    //==========================================================================
    void startNote (int midiNoteNumber, float velocity,
                    juce::SynthesiserSound*, int) override
    {
        currentMidiNote    = midiNoteNumber;
        currentVelocityRaw = velocity;
        currentVelocity    = velocity;

        retune (oscA, oscB, midiNoteNumber, tuningOct,  tuningSemi,  tuningFine);
        retune (oscC, oscD, midiNoteNumber, tuningOct2, tuningSemi2, tuningFine2);
        retune (oscE, oscF, midiNoteNumber, tuningOct3, tuningSemi3, tuningFine3);

        adsr.reset();       adsr.noteOn();
        filterEnv1.reset(); filterEnv1.noteOn();
        filterEnv2.reset(); filterEnv2.noteOn();

        lfo1DelayCounter = 0.0f;

        updateFilter1Cutoff();
        updateFilter2Cutoff();
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            adsr.noteOff();
            filterEnv1.noteOff();
            filterEnv2.noteOff();
        }
        else
        {
            adsr.reset();
            filterEnv1.reset();
            filterEnv2.reset();
            clearCurrentNote();
        }
    }

    //==========================================================================
    void renderNextBlock (juce::AudioBuffer<float>& buffer,
                          int startSample, int numSamples) override
    {
        if (! adsr.isActive()) { clearCurrentNote(); return; }

        auto renderOsc = [&] (juce::dsp::Oscillator<float>& osc, bool isNoise)
        {
            juce::AudioBuffer<float> buf (1, numSamples);
            buf.clear();
            if (isNoise)
            {
                auto* s = buf.getWritePointer (0);
                for (int i = 0; i < numSamples; ++i)
                    s[i] = random.nextFloat() * 2.0f - 1.0f;
            }
            else
            {
                juce::dsp::AudioBlock<float>              block (buf);
                juce::dsp::ProcessContextReplacing<float> ctx   (block);
                osc.process (ctx);
            }
            return buf;
        };

        // LFO 1 — one update per block, applied before OSC render
        float lfo1DelaySamples = (lfo1DelayMs / 1000.0f) * (float) currentSampleRate;
        float lfoMod = 0.0f;
        if (lfo1DelayCounter < lfo1DelaySamples)
            lfo1DelayCounter += (float) numSamples;
        else
            lfoMod = getLfo1Sample (numSamples) * lfo1Depth;

        lfoMod = juce::jlimit (-1.0f, 1.0f, lfoMod);
        float lfo1PitchMult = std::pow (2.0f, lfoMod * 100.0f / 1200.0f);

        float freq1 = 440.0f * std::pow (2.0f, ((float)(currentMidiNote + tuningOct  * 12 + tuningSemi)  + tuningFine  / 100.0f - 69.0f) / 12.0f) * lfo1PitchMult;
        float freq2 = 440.0f * std::pow (2.0f, ((float)(currentMidiNote + tuningOct2 * 12 + tuningSemi2) + tuningFine2 / 100.0f - 69.0f) / 12.0f) * lfo1PitchMult;
        float freq3 = 440.0f * std::pow (2.0f, ((float)(currentMidiNote + tuningOct3 * 12 + tuningSemi3) + tuningFine3 / 100.0f - 69.0f) / 12.0f) * lfo1PitchMult;
        oscA.setFrequency (freq1); oscB.setFrequency (freq1);
        oscC.setFrequency (freq2); oscD.setFrequency (freq2);
        oscE.setFrequency (freq3); oscF.setFrequency (freq3);

        auto bufA = renderOsc (oscA, isNoiseA);
        auto bufB = renderOsc (oscB, isNoiseB);
        auto bufC = renderOsc (oscC, isNoiseC);
        auto bufD = renderOsc (oscD, isNoiseD);
        auto bufE = renderOsc (oscE, isNoiseE);
        auto bufF = renderOsc (oscF, isNoiseF);

        // Update filter envelope params from atomics
        juce::ADSR::Parameters f1p;
        f1p.attack  = atomicFilterEnvA.load(); f1p.decay   = atomicFilterEnvD.load();
        f1p.sustain = atomicFilterEnvS.load(); f1p.release = atomicFilterEnvR.load();
        filterEnv1.setParameters (f1p);
        filter1EnvAmt = atomicFilter1Amt.load();

        juce::ADSR::Parameters f2p;
        f2p.attack  = atomicFilterEnv2A.load(); f2p.decay   = atomicFilterEnv2D.load();
        f2p.sustain = atomicFilterEnv2S.load(); f2p.release = atomicFilterEnv2R.load();
        filterEnv2.setParameters (f2p);
        filter2EnvAmt = atomicFilter2Amt.load();

        float blend = atomicFilterBlend.load();

        auto* sA = bufA.getReadPointer (0);
        auto* sB = bufB.getReadPointer (0);
        auto* sC = bufC.getReadPointer (0);
        auto* sD = bufD.getReadPointer (0);
        auto* sE = bufE.getReadPointer (0);
        auto* sF = bufF.getReadPointer (0);

        juce::AudioBuffer<float> tempBuffer (1, numSamples);
        auto* out = tempBuffer.getWritePointer (0);

        for (int i = 0; i < numSamples; ++i)
                {
                    // Morph each OSC pair
                    float m1 = smoothedMorph .getNextValue();
                    float m2 = smoothedMorph2.getNextValue();
                    float m3 = smoothedMorph3.getNextValue();

                    float osc1s = sA[i] * (1.0f - m1) + sB[i] * m1;
                    float osc2s = sC[i] * (1.0f - m2) + sD[i] * m2;
                    float osc3s = sE[i] * (1.0f - m3) + sF[i] * m3;

                    // Mix OSCs by gain
                    float g1 = smoothedGain .getNextValue();
                    float g2 = smoothedGain2.getNextValue();
                    float g3 = smoothedGain3.getNextValue();
                    float mixed = osc1s * g1 + osc2s * g2 + osc3s * g3;

                    // Drive
                    float drive = smoothedDrive1.getNextValue();
                    mixed = std::tanh (mixed * drive) / std::tanh (drive);

                    // Filter cutoffs with per-sample smoothing + envelope
                    float envMod1   = filterEnv1.getNextSample() * filter1EnvAmt * 20000.0f;
                    float envMod2   = filterEnv2.getNextSample() * filter2EnvAmt * 20000.0f;
                    float maxCutoff = (float)(currentSampleRate * 0.49);
                    float cutoff1   = juce::jlimit (80.0f, maxCutoff, smoothedCutoff1.getNextValue() + envMod1);
                    float cutoff2   = juce::jlimit (80.0f, maxCutoff, smoothedCutoff2.getNextValue() + envMod2);
                    float res1      = smoothedRes1.getNextValue();
                    float res2      = smoothedRes2.getNextValue();

                    // Parallel path
                    filter1.setCutoffFrequency (cutoff1);
                    filter1.setResonance (res1);
                    filter2.setCutoffFrequency (cutoff2);
                    filter2.setResonance (res2);
                    float f1out       = filter1.processSample (0, mixed);
                    float f2out       = filter2.processSample (0, mixed);
                    float parallelOut = (f1out + f2out) * 0.5f;

                    // Series path
                    filter1series.setCutoffFrequency (cutoff1);
                    filter1series.setResonance (res1);
                    filter2series.setCutoffFrequency (cutoff2);
                    filter2series.setResonance (res2);
                    float seriesOut = filter1series.processSample (0, mixed);
                    seriesOut       = filter2series.processSample (0, seriesOut);

                    // Amp ADSR per-sample
                    float env = adsr.getNextSample();

                    out[i] = (parallelOut * (1.0f - blend) + seriesOut * blend) * env;
                }

        // Pan and write
        if (buffer.getNumChannels() >= 2)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                float s = out[i] * currentVelocity;
                buffer.getWritePointer (0)[startSample + i] += s * smoothedPanLeft .getNextValue();
                buffer.getWritePointer (1)[startSample + i] += s * smoothedPanRight.getNextValue();
            }
        }
        else
        {
            buffer.addFrom (0, startSample, tempBuffer, 0, 0, numSamples, currentVelocity);
        }
    }

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    //==========================================================================
    void setFilter1Params (float cutoff, float res, float drive, float keytrack, float velocity)
    {
        filter1Cutoff   = cutoff;
        filter1Keytrack = keytrack;
        filter1Velocity = velocity;
        smoothedRes1  .setTargetValue (juce::jlimit (0.1f,  20.0f, res));
        smoothedDrive1.setTargetValue (juce::jlimit (1.0f,  10.0f, drive));
        updateFilter1Cutoff();
    }

    void setFilter1Type (juce::dsp::StateVariableTPTFilterType type)
    {
        filter1.setType (type);
        filter1series.setType (type);
    }

    void setFilter1EnvParams (const juce::ADSR::Parameters& params, float amount)
    {
        atomicFilterEnvA = params.attack;  atomicFilterEnvD = params.decay;
        atomicFilterEnvS = params.sustain; atomicFilterEnvR = params.release;
        atomicFilter1Amt = amount;
    }

    void setFilter2Params (float cutoff, float res, float drive, float keytrack, float velocity)
    {
        filter2Cutoff   = cutoff;
        filter2Keytrack = keytrack;
        filter2Velocity = velocity;
        smoothedRes2  .setTargetValue (juce::jlimit (0.1f,  20.0f, res));
        smoothedDrive2.setTargetValue (juce::jlimit (1.0f,  10.0f, drive));
        updateFilter2Cutoff();
    }

    void setFilter2Type (juce::dsp::StateVariableTPTFilterType type)
    {
        filter2.setType (type);
        filter2series.setType (type);
    }

    void setFilter2EnvParams (const juce::ADSR::Parameters& params, float amount)
    {
        atomicFilterEnv2A = params.attack;  atomicFilterEnv2D = params.decay;
        atomicFilterEnv2S = params.sustain; atomicFilterEnv2R = params.release;
        atomicFilter2Amt  = amount;
    }

    void setFilterBlend (float blend) { atomicFilterBlend = juce::jlimit (0.0f, 1.0f, blend); }

private:
    //==========================================================================
    void applyWaveform (juce::dsp::Oscillator<float>& osc, Waveform w)
    {
        switch (w)
        {
            case Waveform::Sine:
                osc.initialise ([] (float x) { return std::sin (x); }, 128); break;
            case Waveform::Square:
                osc.initialise ([] (float x) { return x < 0.0f ? -1.0f : 1.0f; }, 128); break;
            case Waveform::Saw:
                osc.initialise ([] (float x) {
                    return juce::jmap (x, -juce::MathConstants<float>::pi,
                                           juce::MathConstants<float>::pi, -1.0f, 1.0f);
                }, 128); break;
            case Waveform::Triangle:
                osc.initialise ([] (float x) {
                    return 2.0f * std::abs (2.0f * (x / juce::MathConstants<float>::twoPi
                                   - std::floor (x / juce::MathConstants<float>::twoPi + 0.5f))) - 1.0f;
                }, 128); break;
            default: break;
        }
    }

    void retune (juce::dsp::Oscillator<float>& oscX, juce::dsp::Oscillator<float>& oscY,
                 int note, int oct, int semi, float fine)
    {
        float totalSemitones = (float)(note + oct * 12 + semi) + fine / 100.0f;
        float freq = 440.0f * std::pow (2.0f, (totalSemitones - 69.0f) / 12.0f);
        oscX.setFrequency (freq);
        oscY.setFrequency (freq);
    }

    void updateFilter1Cutoff()
    {
        float keyOffset = (currentMidiNote - 60) * (filter1Keytrack * 100.0f);
        float velOffset = currentVelocityRaw * filter1Velocity * 10000.0f;
        smoothedCutoff1.setTargetValue (juce::jlimit (20.0f, 20000.0f,
                                                       filter1Cutoff + keyOffset + velOffset));
    }

    void updateFilter2Cutoff()
    {
        float keyOffset = (currentMidiNote - 60) * (filter2Keytrack * 100.0f);
        float velOffset = currentVelocityRaw * filter2Velocity * 10000.0f;
        smoothedCutoff2.setTargetValue (juce::jlimit (20.0f, 20000.0f,
                                                       filter2Cutoff + keyOffset + velOffset));
    }

    //==========================================================================
    // LFO 1 — free-running, pitch destination
    float lfo1Phase        { juce::Random::getSystemRandom().nextFloat()
                             * juce::MathConstants<float>::twoPi };
    float lfo1Rate         { 1.0f };
    float lfo1Depth        { 0.0f };
    float lfo1DelayMs      { 0.0f };
    int   lfo1Shape        { 0 };
    float lfo1DelayCounter { 0.0f };

    float getLfo1Sample (int numSamples) noexcept
    {
        float out = 0.0f;
        switch (lfo1Shape)
        {
            case 0: out = std::sin (lfo1Phase); break;   // Sine
            case 1:                                       // Triangle
                out = (lfo1Phase < juce::MathConstants<float>::pi)
                    ? (lfo1Phase / juce::MathConstants<float>::pi) * 2.0f - 1.0f
                    : 1.0f - ((lfo1Phase - juce::MathConstants<float>::pi)
                              / juce::MathConstants<float>::pi) * 2.0f;
                break;
            case 2: out = (lfo1Phase < juce::MathConstants<float>::pi) ? 1.0f : -1.0f; break; // Square
            case 3: out = (lfo1Phase / juce::MathConstants<float>::twoPi) * 2.0f - 1.0f; break; // Saw up
            case 4: out = 1.0f - (lfo1Phase / juce::MathConstants<float>::twoPi) * 2.0f; break; // Saw down
            default: out = std::sin (lfo1Phase); break;
        }
        lfo1Phase += (juce::MathConstants<float>::twoPi * lfo1Rate * (float) numSamples)
                     / (float) currentSampleRate;
        if (lfo1Phase >= juce::MathConstants<float>::twoPi)
            lfo1Phase -= juce::MathConstants<float>::twoPi;
        return out;
    }

    //==========================================================================
    double currentSampleRate { 44100.0 };

    juce::dsp::Oscillator<float> oscA, oscB;
    juce::dsp::Oscillator<float> oscC, oscD;
    juce::dsp::Oscillator<float> oscE, oscF;
    juce::dsp::Gain<float>       gain;
    juce::ADSR                   adsr;
    juce::Random                 random;

    bool isNoiseA { false }, isNoiseB { false };
    bool isNoiseC { false }, isNoiseD { false };
    bool isNoiseE { false }, isNoiseF { false };

    float currentVelocity    { 0.0f };
    float currentVelocityRaw { 0.0f };
    int   currentMidiNote    { 69 };

    // OSC 1 smoothing
    juce::LinearSmoothedValue<float> smoothedGain, smoothedPanLeft, smoothedPanRight, smoothedMorph;

    // OSC 2 smoothing
    juce::LinearSmoothedValue<float> smoothedGain2, smoothedPanLeft2, smoothedPanRight2, smoothedMorph2;

    // OSC 3 smoothing
    juce::LinearSmoothedValue<float> smoothedGain3, smoothedPanLeft3, smoothedPanRight3, smoothedMorph3;

    // Tuning
    int   tuningOct  { 0 }, tuningSemi  { 0 }; float tuningFine  { 0.0f };
    int   tuningOct2 { 0 }, tuningSemi2 { 0 }; float tuningFine2 { 0.0f };
    int   tuningOct3 { 0 }, tuningSemi3 { 0 }; float tuningFine3 { 0.0f };

    // Filter 1 — parallel instance + series instance
    juce::dsp::StateVariableTPTFilter<float> filter1;
    juce::dsp::StateVariableTPTFilter<float> filter1series;
    juce::LinearSmoothedValue<float>         smoothedCutoff1, smoothedRes1, smoothedDrive1;
    float              filter1Cutoff   { 20000.0f };
    float              filter1Keytrack { 0.0f };
    float              filter1Velocity { 0.0f };
    juce::ADSR         filterEnv1;
    float              filter1EnvAmt   { 0.0f };
    std::atomic<float> atomicFilterEnvA { 0.1f }, atomicFilterEnvD { 0.2f };
    std::atomic<float> atomicFilterEnvS { 0.7f }, atomicFilterEnvR { 0.4f };
    std::atomic<float> atomicFilter1Amt { 0.0f };

    // Filter 2 — parallel instance + series instance
    juce::dsp::StateVariableTPTFilter<float> filter2;
    juce::dsp::StateVariableTPTFilter<float> filter2series;
    juce::LinearSmoothedValue<float>         smoothedCutoff2, smoothedRes2, smoothedDrive2;
    float              filter2Cutoff   { 20000.0f };
    float              filter2Keytrack { 0.0f };
    float              filter2Velocity { 0.0f };
    juce::ADSR         filterEnv2;
    float              filter2EnvAmt   { 0.0f };
    std::atomic<float> atomicFilterEnv2A { 0.1f }, atomicFilterEnv2D { 0.2f };
    std::atomic<float> atomicFilterEnv2S { 0.7f }, atomicFilterEnv2R { 0.4f };
    std::atomic<float> atomicFilter2Amt  { 0.0f };

    // Filter blend
    std::atomic<float> atomicFilterBlend { 1.0f };
};
