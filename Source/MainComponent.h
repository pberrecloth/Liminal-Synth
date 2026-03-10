#pragma once

#include <JuceHeader.h>
#include "SynthVoice.h"
#include "AudioEngine.h"

class MainComponent : public juce::AudioAppComponent
{
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override
    {
        engine.prepareToPlay (samplesPerBlockExpected, sampleRate);
    }

    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        engine.getNextAudioBlock (bufferToFill);
    }

    void releaseResources() override
    {
        engine.releaseResources();
    }

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    AudioEngine engine;

    void showWaveformMenu (juce::TextButton& button,
                           std::function<void(SynthVoice::Waveform)> onSelect);

    //==========================================================================
    // Top Bar
    juce::TextButton settingsButton { "Audio Settings" };
    juce::TextButton midiButton     { "MIDI Settings" };
    juce::TextButton initButton     { "Init Patch" };
    juce::TextButton saveButton     { "Save" };
    juce::Label      patchNameLabel;
    static constexpr int topBarHeight = 36;

    void openAudioSettings();
    void openMidiSettings();

    //==========================================================================
    // Master
    juce::Slider masterVolume, masterPan;
    juce::Label  masterVolumeL, masterPanL;

    //==========================================================================
    // Oscillators
    struct OscSection
    {
        juce::TextButton waveA { "SINE" };
        juce::TextButton waveB { "SQUARE" };
        juce::Slider     morph, oct, semi, fine, vol, pan;
        juce::Label      morphL, octL, semiL, fineL, volL, panL;
    };
    OscSection osc1, osc2, osc3;

    //==========================================================================
    // Filters 1 & 2
    struct FilterSection
    {
        juce::TextButton type { "LOW PASS" };
        juce::Slider     cutoff, res, drive;
        juce::Label      cutoffL, resL, driveL;
        juce::Slider     envA, envD, envS, envR, envAmt;
        juce::Label      envAL, envDL, envSL, envRL, envAmtL;
        juce::Slider     keytrack, velocity;
        juce::Label      keytrackL, velocityL;
    };
    FilterSection filter1, filter2;

    juce::Slider filterBlend;
    juce::Label  filterBlendL;

    // Filter 3 HP
    juce::ToggleButton filter3On     { "On" };
    juce::ToggleButton filter3KeyTrk { "Key Track" };
    juce::Slider       filter3Freq, filter3Q;
    juce::Label        filter3FreqL, filter3QL;

    //==========================================================================
    // Amp Env
    juce::Slider ampA, ampD, ampS, ampR;
    juce::Label  ampAL, ampDL, ampSL, ampRL;

    // Env 3 Free
    juce::Slider env3A, env3D, env3S, env3R;
    juce::Label  env3AL, env3DL, env3SL, env3RL;

    //==========================================================================
    // LFOs
    struct LfoSection
    {
        juce::TextButton shape { "SINE" };
        juce::Slider     rate, depth, delay;
        juce::Label      rateL, depthL, delayL;
    };
    LfoSection lfo1, lfo2;

    //==========================================================================
    // Mod Matrix
    struct ModRow
    {
        juce::TextButton source { "SOURCE" };
        juce::TextButton dest   { "DEST" };
        juce::Slider     amount;
        juce::Label      amountL;
    };
    ModRow mod1, mod2, mod3, mod4;

    //==========================================================================
    // Effects
    juce::TextButton fxPreset { "INIT" };
    juce::Label      fxPresetL;
    juce::Slider     revSize, revMix;
    juce::Label      revSizeL, revMixL;
    juce::Slider     delTime, delFeedback, delMix;
    juce::Label      delTimeL, delFeedbackL, delMixL;

    //==========================================================================
    // Layout constants
    static constexpr int kKnob   = 48;
    static constexpr int kLabel  = 16;
    static constexpr int kGap    = 6;
    static constexpr int kPad    = 8;
    static constexpr int kMaster = 70;
    static constexpr int kOscH   = 130;
    static constexpr int kFilH   = 210;
    static constexpr int kFil3H  = 90;
    static constexpr int kEnvH   = 90;
    static constexpr int kLfoH   = 110;
    static constexpr int kFxH    = 100;
    static constexpr int kMatH   = 110;

    //==========================================================================
    // Helpers
    void setupSlider  (juce::Slider& s);
    void setupLabel   (juce::Label& l, const juce::String& text);
    void setupOsc     (OscSection& osc);
    void setupFilter  (FilterSection& f);
    void setupLfo     (LfoSection& lfo);
    void setupModRow  (ModRow& row);

    void layoutOsc    (OscSection& osc,    juce::Rectangle<int> area);
    void layoutFilter (FilterSection& f,   juce::Rectangle<int> area);
    void layoutLfo    (LfoSection& lfo,    juce::Rectangle<int> area);
    void layoutModRow (ModRow& row,        juce::Rectangle<int> area);
    void layoutEnv    (juce::Rectangle<int> area,
                       juce::Slider& a, juce::Label& al,
                       juce::Slider& d, juce::Label& dl,
                       juce::Slider& s, juce::Label& sl,
                       juce::Slider& r, juce::Label& rl);

    void drawPanel (juce::Graphics& g, juce::Rectangle<int> area,
                    const juce::String& title);

    void setupOscWiring();
    void setupFilterWiring();
    void setupLfoWiring();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
