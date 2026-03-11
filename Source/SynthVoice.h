#pragma once

#include <JuceHeader.h>

//==============================================================================
// Curved ADSR — exponential attack/decay/release, linear sustain
// Replaces juce::ADSR with natural-feeling analogue-style curves
//==============================================================================
class CurvedADSR
{
public:
    struct Parameters
    {
        float attack  { 0.1f };
        float decay   { 0.2f };
        float sustain { 0.7f };
        float release { 0.4f };
    };

    void setSampleRate (double sr) { sampleRate = sr; }

    void setParameters (const Parameters& p) { params = p; }

    // Compatible with juce::ADSR::Parameters
    void setParameters (const juce::ADSR::Parameters& p)
    {
        params.attack  = p.attack;
        params.decay   = p.decay;
        params.sustain = p.sustain;
        params.release = p.release;
    }

    float getCurrentLevel() const { return currentLevel; }

    void noteOn()
    {
        attackTimeCounter = 0.0f;

        // Zero-shortcut: if attack is 0, skip straight to sustain/decay
        if (params.attack == 0.0f)
        {
            currentLevel = 1.0f;
            if (params.decay == 0.0f) { currentLevel = params.sustain; stage = Stage::Sustain; }
            else                      { stage = Stage::Decay; }
            return;
        }

        stage = Stage::Attack;
        // Compute equivalent time position so retrigger starts from current level smoothly
        float attackSamples = juce::jmax (1.0f, params.attack * (float) sampleRate);
        float safeLevel = juce::jlimit (0.0f, 0.9999f, currentLevel);
        attackTimeCounter = -std::log (1.0f - safeLevel) * attackSamples / 6.908f;
    }

    void noteOff()
    {
        if (stage != Stage::Idle)
        {
            stage         = Stage::Release;
            releaseLevel  = currentLevel;
        }
    }

    void reset()
    {
        stage             = Stage::Idle;
        currentLevel      = 0.0f;
        attackTimeCounter = 0.0f;
        quickReleaseActive = false;
    }

    bool isActive() const { return stage != Stage::Idle; }

    float getNextSample()
    {
        switch (stage)
        {
            case Stage::Attack:
            {
                float attackSamples = juce::jmax (1.0f, params.attack * (float) sampleRate);
                attackTimeCounter += 1.0f;
                // Convex exponential — capacitor charging: slow start, accelerating bloom
                currentLevel = 1.0f - std::exp (-attackTimeCounter * 6.908f / attackSamples);
                if (currentLevel >= 0.999f)
                {
                    currentLevel      = 1.0f;
                    attackTimeCounter = 0.0f;
                    // Zero-shortcut: if decay is 0, skip straight to sustain
                    if (params.decay == 0.0f) { currentLevel = params.sustain; stage = Stage::Sustain; }
                    else                      { stage = Stage::Decay; }
                }
                break;
            }
            case Stage::Decay:
            {
                float decaySamples = juce::jmax (1.0f, params.decay * (float) sampleRate);
                float target = params.sustain;
                // Exponential decay — fast drop then levels off at sustain
                currentLevel += (target - currentLevel) * (1.0f - std::exp (-3.0f / decaySamples));
                if (std::abs (currentLevel - target) < 0.001f)
                {
                    currentLevel = target;
                    stage = Stage::Sustain;
                }
                break;
            }
            case Stage::Sustain:
                currentLevel = params.sustain;
                break;

            case Stage::Release:
            {
                // Minimum floor: 2ms for quick-release (voice steal), 5ms for user release (prevents click at R=0)
                float minRelease     = (float) sampleRate * (quickReleaseActive ? 0.002f : 0.005f);
                float releaseSamples = quickReleaseActive
                                         ? minRelease
                                         : juce::jmax (minRelease, params.release * (float) sampleRate);
                currentLevel += (0.0f - currentLevel) * (1.0f - std::exp (-3.0f / releaseSamples));
                if (currentLevel < 0.0001f)
                {
                    currentLevel       = 0.0f;
                    quickReleaseActive = false;
                    stage              = Stage::Idle;
                }
                break;
            }
            case Stage::Idle:
                currentLevel = 0.0f;
                break;
        }
        return currentLevel;
    }

private:
    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    Parameters params;
    Stage      stage              { Stage::Idle };
    float      currentLevel       { 0.0f };
    float      releaseLevel       { 0.0f };
    float      attackTimeCounter  { 0.0f };
    bool       quickReleaseActive { false };
    double     sampleRate         { 48000.0 };
};

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

        // Filter envs
        filterEnv1.setSampleRate (sampleRate);
        CurvedADSR::Parameters fp;
        fp.attack = 0.1f; fp.decay = 0.2f; fp.sustain = 0.7f; fp.release = 0.4f;
        filterEnv1.setParameters (fp);

        filterEnv2.setSampleRate (sampleRate);
        filterEnv2.setParameters (fp);

        // Amp ADSR
        adsr.setSampleRate (sampleRate);
        CurvedADSR::Parameters p;
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

    // Amp env — accepts juce::ADSR::Parameters for compatibility with existing wiring
    void setAdsrParams (const juce::ADSR::Parameters& params)
    {
        CurvedADSR::Parameters p;
        p.attack  = params.attack;
        p.decay   = params.decay;
        p.sustain = params.sustain;
        p.release = params.release;
        adsr.setParameters (p);
    }

    // Velocity sensitivity — 0=no velocity, 1=full velocity sensitivity
    void setVelocitySensitivity (float v) { velocitySensitivity = juce::jlimit (0.0f, 1.0f, v); }

    //==========================================================================
    // LFO 1
    void setLfo1Rate  (float hz)   { lfo1Rate    = hz; }
    void setLfo1Depth (float d)    { lfo1Depth   = d; }
    void setLfo1Delay (float ms)   { lfo1DelayMs = ms; }
    void setLfo1Shape (int shape)  { lfo1Shape   = shape; }

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

        // Velocity sensitivity — blend between fixed level and full velocity
        currentVelocity = juce::jlimit (0.0f, 1.0f,
            (1.0f - velocitySensitivity) + velocitySensitivity * velocity);

        retune (oscA, oscB, midiNoteNumber, tuningOct,  tuningSemi,  tuningFine);
        retune (oscC, oscD, midiNoteNumber, tuningOct2, tuningSemi2, tuningFine2);
        retune (oscE, oscF, midiNoteNumber, tuningOct3, tuningSemi3, tuningFine3);

        bool wasStolen = adsr.isActive(); // true when JUCE is stealing a releasing voice

        if (wasStolen)
        {
            // Voice steal: preserve envelope levels and filter state for a click-free handoff.
            // The ADSR continues from its current release level straight into a new attack.
            adsr.noteOn();
            filterEnv1.noteOn();
            filterEnv2.noteOn();
            // Filters keep integrator state — no state discontinuity
            deClickGain = 1.0f;
            deClickInc  = 0.0f;
        }
        else
        {
            // Fresh voice: full reset for a clean start
            adsr.reset();       adsr.noteOn();
            filterEnv1.reset(); filterEnv1.noteOn();
            filterEnv2.reset(); filterEnv2.noteOn();
            filter1.reset();       filter1series.reset();
            filter2.reset();       filter2series.reset();
            // 2ms fade-in ramp to mask oscillator phase discontinuity
            deClickGain = 0.0f;
            deClickInc  = 1.0f / juce::jmax (1.0f, (float) currentSampleRate * 0.002f);
        }

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
            // Voice steal: just release JUCE ownership.
            // Audio state (ADSR level, filter integrators) carries over into the new startNote
            // so the incoming note begins from the outgoing level — no hard cut, no click.
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

        // LFO 1
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
        // Set frequency every block
        oscA.setFrequency (freq1, true); oscB.setFrequency (freq1, true);
        oscC.setFrequency (freq2, true); oscD.setFrequency (freq2, true);
        oscE.setFrequency (freq3, true); oscF.setFrequency (freq3, true);

        auto bufA = renderOsc (oscA, isNoiseA);
        auto bufB = renderOsc (oscB, isNoiseB);
        auto bufC = renderOsc (oscC, isNoiseC);
        auto bufD = renderOsc (oscD, isNoiseD);
        auto bufE = renderOsc (oscE, isNoiseE);
        auto bufF = renderOsc (oscF, isNoiseF);

        // Update filter envelope params from atomics
        CurvedADSR::Parameters f1p;
        f1p.attack  = atomicFilterEnvA.load(); f1p.decay   = atomicFilterEnvD.load();
        f1p.sustain = atomicFilterEnvS.load(); f1p.release = atomicFilterEnvR.load();
        filterEnv1.setParameters (f1p);
        filter1EnvAmt = atomicFilter1Amt.load();

        CurvedADSR::Parameters f2p;
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

        // Resonance — once per block
        float res1 = smoothedRes1.getCurrentValue(); smoothedRes1.skip (numSamples);
        float res2 = smoothedRes2.getCurrentValue(); smoothedRes2.skip (numSamples);
        filter1.setResonance       (res1);
        filter1series.setResonance (res1);
        filter2.setResonance       (res2);
        filter2series.setResonance (res2);

        float maxCutoff = (float)(currentSampleRate * 0.49);

        for (int i = 0; i < numSamples; ++i)
        {
            float m1 = smoothedMorph .getNextValue();
            float m2 = smoothedMorph2.getNextValue();
            float m3 = smoothedMorph3.getNextValue();

            float osc1s = sA[i] * (1.0f - m1) + sB[i] * m1;
            float osc2s = sC[i] * (1.0f - m2) + sD[i] * m2;
            float osc3s = sE[i] * (1.0f - m3) + sF[i] * m3;

            float g1 = smoothedGain .getNextValue();
            float g2 = smoothedGain2.getNextValue();
            float g3 = smoothedGain3.getNextValue();
            float mixed = osc1s * g1 + osc2s * g2 + osc3s * g3;

            float drive1 = smoothedDrive1.getNextValue();
            float drive2 = smoothedDrive2.getNextValue();
            // Per-filter pre-saturation — each filter has its own drive character
            float sat1 = std::tanh (mixed * drive1) / std::tanh (drive1);
            float sat2 = std::tanh (mixed * drive2) / std::tanh (drive2);

            float f1envSample = filterEnv1.getNextSample();
            float f2envSample = filterEnv2.getNextSample();
            float envMod1 = bypassFilter1Env.load() ? 0.0f : f1envSample * filter1EnvAmt * 20000.0f;
            float envMod2 = bypassFilter2Env.load() ? 0.0f : f2envSample * filter2EnvAmt * 20000.0f;
            float cutoff1 = juce::jlimit (80.0f, maxCutoff, smoothedCutoff1.getNextValue() + envMod1);
            float cutoff2 = juce::jlimit (80.0f, maxCutoff, smoothedCutoff2.getNextValue() + envMod2);

            // Parallel path
            filter1.setCutoffFrequency (cutoff1);
            filter2.setCutoffFrequency (cutoff2);
            float f1out       = filter1.processSample (0, sat1);
            float f2out       = filter2.processSample (0, sat2);
            float parallelOut = (f1out + f2out) * 0.5f;

            // Series path: drive2 applied between the two stages
            filter1series.setCutoffFrequency (cutoff1);
            filter2series.setCutoffFrequency (cutoff2);
            float seriesOut = filter1series.processSample (0, sat1);
            seriesOut       = filter2series.processSample (0, seriesOut);

            float env = adsr.getNextSample();  // always advance so voice knows when to stop
            float ampGate = bypassAmpEnv.load() ? 1.0f : env;

            if (deClickGain < 1.0f)
                deClickGain = juce::jmin (1.0f, deClickGain + deClickInc);

            out[i] = (parallelOut * (1.0f - blend) + seriesOut * blend) * ampGate * deClickGain;
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

    //==========================================================================
    // Debug bypasses — isolate which envelope path is misbehaving
    void setAmpEnvBypass     (bool b) { bypassAmpEnv     = b; }
    void setFilter1EnvBypass (bool b) { bypassFilter1Env = b; }
    void setFilter2EnvBypass (bool b) { bypassFilter2Env = b; }

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
        oscX.setFrequency (freq, true);
        oscY.setFrequency (freq, true);
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
    // LFO 1
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
            case 0: out = std::sin (lfo1Phase); break;
            case 1:
                out = (lfo1Phase < juce::MathConstants<float>::pi)
                    ? (lfo1Phase / juce::MathConstants<float>::pi) * 2.0f - 1.0f
                    : 1.0f - ((lfo1Phase - juce::MathConstants<float>::pi)
                              / juce::MathConstants<float>::pi) * 2.0f;
                break;
            case 2: out = (lfo1Phase < juce::MathConstants<float>::pi) ? 1.0f : -1.0f; break;
            case 3: out = (lfo1Phase / juce::MathConstants<float>::twoPi) * 2.0f - 1.0f; break;
            case 4: out = 1.0f - (lfo1Phase / juce::MathConstants<float>::twoPi) * 2.0f; break;
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
    CurvedADSR                   adsr;        // ← was juce::ADSR
    juce::Random                 random;

    bool isNoiseA { false }, isNoiseB { false };
    bool isNoiseC { false }, isNoiseD { false };
    bool isNoiseE { false }, isNoiseF { false };

    float currentVelocity       { 0.7f };
    float currentVelocityRaw    { 0.0f };
    float velocitySensitivity   { 0.7f };  // default — moderate sensitivity
    int   currentMidiNote       { 69 };

    // OSC smoothing
    juce::LinearSmoothedValue<float> smoothedGain, smoothedPanLeft, smoothedPanRight, smoothedMorph;
    juce::LinearSmoothedValue<float> smoothedGain2, smoothedPanLeft2, smoothedPanRight2, smoothedMorph2;
    juce::LinearSmoothedValue<float> smoothedGain3, smoothedPanLeft3, smoothedPanRight3, smoothedMorph3;

    // Tuning
    int   tuningOct  { 0 }, tuningSemi  { 0 }; float tuningFine  { 0.0f };
    int   tuningOct2 { 0 }, tuningSemi2 { 0 }; float tuningFine2 { 0.0f };
    int   tuningOct3 { 0 }, tuningSemi3 { 0 }; float tuningFine3 { 0.0f };

    // Filter 1
    juce::dsp::StateVariableTPTFilter<float> filter1;
    juce::dsp::StateVariableTPTFilter<float> filter1series;
    juce::LinearSmoothedValue<float>         smoothedCutoff1, smoothedRes1, smoothedDrive1;
    float              filter1Cutoff   { 20000.0f };
    float              filter1Keytrack { 0.0f };
    float              filter1Velocity { 0.0f };
    CurvedADSR         filterEnv1;            // ← was juce::ADSR
    float              filter1EnvAmt   { 0.0f };
    std::atomic<float> atomicFilterEnvA { 0.1f }, atomicFilterEnvD { 0.2f };
    std::atomic<float> atomicFilterEnvS { 0.7f }, atomicFilterEnvR { 0.4f };
    std::atomic<float> atomicFilter1Amt { 0.0f };

    // Filter 2
    juce::dsp::StateVariableTPTFilter<float> filter2;
    juce::dsp::StateVariableTPTFilter<float> filter2series;
    juce::LinearSmoothedValue<float>         smoothedCutoff2, smoothedRes2, smoothedDrive2;
    float              filter2Cutoff   { 20000.0f };
    float              filter2Keytrack { 0.0f };
    float              filter2Velocity { 0.0f };
    CurvedADSR         filterEnv2;            // ← was juce::ADSR
    float              filter2EnvAmt   { 0.0f };
    std::atomic<float> atomicFilterEnv2A { 0.1f }, atomicFilterEnv2D { 0.2f };
    std::atomic<float> atomicFilterEnv2S { 0.7f }, atomicFilterEnv2R { 0.4f };
    std::atomic<float> atomicFilter2Amt  { 0.0f };

    // Filter blend
    std::atomic<float> atomicFilterBlend { 1.0f };

    // Debug bypass flags
    std::atomic<bool> bypassAmpEnv     { false };
    std::atomic<bool> bypassFilter1Env { false };
    std::atomic<bool> bypassFilter2Env { false };

    // Note-on de-click: 2ms linear fade-in on fresh starts to mask oscillator phase discontinuity.
    // Set to 1.0 / inc=0.0 for voice-steal starts (amplitude continuity from ADSR carry-over).
    float deClickGain { 1.0f };
    float deClickInc  { 0.0f };
};
