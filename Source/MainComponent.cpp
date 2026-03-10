#include "MainComponent.h"

MainComponent::MainComponent()
{
    setSize (1200, 900);

    // Set up device FIRST before starting audio
    /*
    auto setup = deviceManager.getAudioDeviceSetup();
    setup.bufferSize = 128;
    setup.sampleRate = 48000.0;
    deviceManager.setAudioDeviceSetup (setup, true);*/
    
    // Audio FIRST
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
            [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
    }
    else
    {
        setAudioChannels (2, 2);
    }

    
    // Direct MIDI — register this class as the MIDI callback
    deviceManager.addMidiInputDeviceCallback ({}, this);
    
    // Enable all available MIDI inputs
    auto midiInputs = juce::MidiInput::getAvailableDevices();
    for (auto& input : midiInputs)
    {
        if (!deviceManager.isMidiInputDeviceEnabled (input.identifier))
            deviceManager.setMidiInputDeviceEnabled (input.identifier, true);
        deviceManager.addMidiInputDeviceCallback (input.identifier, this);
    }

    // Top bar
    patchNameLabel.setText ("Init Patch", juce::dontSendNotification);
    patchNameLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (settingsButton);
    addAndMakeVisible (midiButton);
    addAndMakeVisible (initButton);
    addAndMakeVisible (saveButton);
    addAndMakeVisible (patchNameLabel);
    settingsButton.onClick = [this] { openAudioSettings(); };
    midiButton.onClick     = [this] { openMidiSettings(); };

    // Master
    setupSlider (masterVolume); setupLabel (masterVolumeL, "Volume");
    setupSlider (masterPan);    setupLabel (masterPanL,    "Pan");
    masterVolume.setRange (0.0, 1.0, 0.001);
    masterVolume.setValue (0.8);
    masterPan   .setRange (-1.0, 1.0);
    masterPan   .setValue (0.0);
    masterVolume.onValueChange = [this]() { engine.setMasterVol ((float) masterVolume.getValue()); };
    masterPan   .onValueChange = [this]() { engine.setMasterPan ((float) masterPan   .getValue()); };

    setupOscWiring();
    setupFilterWiring();
    setupAmpWiring();
    setupLfoWiring();

    // LFOs
    setupLfo (lfo1);
    setupLfo (lfo2);

    // Mod Matrix
    setupModRow (mod1);
    setupModRow (mod2);
    setupModRow (mod3);
    setupModRow (mod4);

    // Effects
    fxPresetL.setText ("Preset:", juce::dontSendNotification);
    fxPresetL.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (fxPresetL);
    addAndMakeVisible (fxPreset);

    setupSlider (revSize);     setupLabel (revSizeL,     "Size");
    setupSlider (revMix);      setupLabel (revMixL,      "Mix");
    setupSlider (delTime);     setupLabel (delTimeL,     "Time");
    setupSlider (delFeedback); setupLabel (delFeedbackL, "Fdbk");
    setupSlider (delMix);      setupLabel (delMixL,      "Mix");

    revSize    .setRange (0.0, 1.0);  revSize    .setValue (0.5);
    revMix     .setRange (0.0, 1.0);  revMix     .setValue (0.0);
    delTime    .setRange (0.01, 2.0); delTime    .setValue (0.3);
    delTime    .setSkewFactorFromMidPoint (0.5);
    delFeedback.setRange (0.0, 0.95); delFeedback.setValue (0.4);
    delMix     .setRange (0.0, 1.0);  delMix     .setValue (0.0);

    revSize    .onValueChange = [this]() { engine.effects.setRevSize     ((float) revSize    .getValue()); };
    revMix     .onValueChange = [this]() { engine.effects.setRevMix      ((float) revMix     .getValue()); };
    delTime    .onValueChange = [this]() { engine.effects.setDelTime     ((float) delTime    .getValue()); };
    delFeedback.onValueChange = [this]() { engine.effects.setDelFeedback ((float) delFeedback.getValue()); };
    delMix     .onValueChange = [this]() { engine.effects.setDelMix      ((float) delMix     .getValue()); };
}

MainComponent::~MainComponent()
{
    auto midiInputs = juce::MidiInput::getAvailableDevices();
       for (auto& input : midiInputs)
           deviceManager.removeMidiInputDeviceCallback (input.identifier, this);
       shutdownAudio();
}

void MainComponent::initialiseVoiceParams()
{
    // AMP
    juce::ADSR::Parameters params;
    params.attack  = (float) ampA.getValue();
    params.decay   = (float) ampD.getValue();
    params.sustain = (float) ampS.getValue();
    params.release = (float) ampR.getValue();
    for (int i = 0; i < engine.synthesiser.getNumVoices(); ++i)
        if (auto* v = dynamic_cast<SynthVoice*> (engine.synthesiser.getVoice (i)))
            v->setAdsrParams (params);

    // LFO
    for (int i = 0; i < engine.synthesiser.getNumVoices(); ++i)
        if (auto* v = dynamic_cast<SynthVoice*> (engine.synthesiser.getVoice (i)))
        {
            v->setLfo1Rate  ((float) lfo1.rate .getValue());
            v->setLfo1Depth ((float) lfo1.depth.getValue());
            v->setLfo1Delay ((float) lfo1.delay.getValue());
        }
    
    // Filter 1 env
        for (int i = 0; i < engine.synthesiser.getNumVoices(); ++i)
            if (auto* v = dynamic_cast<SynthVoice*> (engine.synthesiser.getVoice (i)))
            {
                juce::ADSR::Parameters f1p;
                f1p.attack  = (float) filter1.envA.getValue();
                f1p.decay   = (float) filter1.envD.getValue();
                f1p.sustain = (float) filter1.envS.getValue();
                f1p.release = (float) filter1.envR.getValue();
                v->setFilter1EnvParams (f1p, (float) filter1.envAmt.getValue());

                juce::ADSR::Parameters f2p;
                f2p.attack  = (float) filter2.envA.getValue();
                f2p.decay   = (float) filter2.envD.getValue();
                f2p.sustain = (float) filter2.envS.getValue();
                f2p.release = (float) filter2.envR.getValue();
                v->setFilter2EnvParams (f2p, (float) filter2.envAmt.getValue());
            }
}

void MainComponent::openAudioSettings()
{
    auto* selector = new juce::AudioDeviceSelectorComponent (
        deviceManager, 0, 2, 0, 2, false, false, false, false);
    selector->setSize (500, 400);
    juce::DialogWindow::LaunchOptions o;
    o.content.setOwned (selector);
    o.dialogTitle                  = "Audio Settings";
    o.dialogBackgroundColour       = juce::Colours::darkgrey;
    o.escapeKeyTriggersCloseButton = true;
    o.useNativeTitleBar            = true;
    o.resizable                    = false;
    o.launchAsync();
}

void MainComponent::openMidiSettings()
{
    auto* selector = new juce::AudioDeviceSelectorComponent (
        deviceManager, 0, 0, 0, 0, true, true, false, false);
    selector->setSize (500, 300);
    juce::DialogWindow::LaunchOptions o;
    o.content.setOwned (selector);
    o.dialogTitle                  = "MIDI Settings";
    o.dialogBackgroundColour       = juce::Colours::darkgrey;
    o.escapeKeyTriggersCloseButton = true;
    o.useNativeTitleBar            = true;
    o.resizable                    = false;
    o.launchAsync();
}

void MainComponent::showWaveformMenu (juce::TextButton& button,
                                       std::function<void(SynthVoice::Waveform)> onSelect)
{
    juce::PopupMenu menu;
    menu.addItem (1, "Sine");
    menu.addItem (2, "Square");
    menu.addItem (3, "Saw");
    menu.addItem (4, "Triangle");
    menu.addItem (5, "Noise");

    auto* buttonPtr = &button;
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (button),
        [buttonPtr, onSelect] (int result)
        {
            if (result == 0) return;
            juce::String name;
            SynthVoice::Waveform waveform;
            switch (result)
            {
                case 1: name = "Sine";     waveform = SynthVoice::Waveform::Sine;     break;
                case 2: name = "Square";   waveform = SynthVoice::Waveform::Square;   break;
                case 3: name = "Saw";      waveform = SynthVoice::Waveform::Saw;      break;
                case 4: name = "Triangle"; waveform = SynthVoice::Waveform::Triangle; break;
                case 5: name = "Noise";    waveform = SynthVoice::Waveform::Noise;    break;
                default: return;
            }
            buttonPtr->setButtonText (name);
            onSelect (waveform);
        });
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    const int col1W = 480;
    const int col2W = getWidth() - col1W - kPad * 3;
    int y           = topBarHeight + kPad;

    drawPanel (g, { kPad, y, getWidth() - kPad * 2, kMaster }, "MASTER");
    y += kMaster + kPad;

    const int col1X = kPad;
    const int col2X = kPad * 2 + col1W;
    int col1Y = y;
    int col2Y = y;

    drawPanel (g, { col1X, col1Y, col1W, kOscH }, "OSC 1");  col1Y += kOscH + kPad;
    drawPanel (g, { col1X, col1Y, col1W, kOscH }, "OSC 2");  col1Y += kOscH + kPad;
    drawPanel (g, { col1X, col1Y, col1W, kOscH }, "OSC 3");  col1Y += kOscH + kPad;

    int matH = col2Y + kFilH + kPad + kFil3H + kPad + kEnvH + kPad + kLfoH - col1Y;
    matH = juce::jmax (kMatH, matH);
    drawPanel (g, { col1X, col1Y, col1W, matH }, "MOD MATRIX");

    int filterW = (col2W - kPad) / 2;
    drawPanel (g, { col2X,                  col2Y, filterW, kFilH }, "FILTER 1");
    drawPanel (g, { col2X + filterW + kPad, col2Y, filterW, kFilH }, "FILTER 2");
    col2Y += kFilH + kPad;

    drawPanel (g, { col2X, col2Y, col2W, kFil3H }, "FILTER 3:  HP MASTER");
    col2Y += kFil3H + kPad;

    int envW = (col2W - kPad) / 2;
    drawPanel (g, { col2X,               col2Y, envW, kEnvH }, "AMP ENV");
    drawPanel (g, { col2X + envW + kPad, col2Y, envW, kEnvH }, "ENV 3 - FREE");
    col2Y += kEnvH + kPad;

    int lfoW = (col2W - kPad) / 2;
    drawPanel (g, { col2X,               col2Y, lfoW, kLfoH }, "LFO 1");
    drawPanel (g, { col2X + lfoW + kPad, col2Y, lfoW, kLfoH }, "LFO 2");
    col2Y += kLfoH + kPad;

    int bottomY = juce::jmax (col1Y + matH + kPad, col2Y);
    drawPanel (g, { kPad, bottomY, getWidth() - kPad * 2, kFxH }, "EFFECTS");
}
