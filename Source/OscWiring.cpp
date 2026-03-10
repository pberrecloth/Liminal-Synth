#include "MainComponent.h"

void MainComponent::setupOsc (OscSection& osc)
{
    addAndMakeVisible (osc.waveA);
    addAndMakeVisible (osc.waveB);
    setupSlider (osc.morph); setupLabel (osc.morphL, "Morph");
    setupSlider (osc.oct);   setupLabel (osc.octL,   "Oct");
    setupSlider (osc.semi);  setupLabel (osc.semiL,  "Semi");
    setupSlider (osc.fine);  setupLabel (osc.fineL,  "Fine");
    setupSlider (osc.vol);   setupLabel (osc.volL,   "Vol");
    setupSlider (osc.pan);   setupLabel (osc.panL,   "Pan");
}

void MainComponent::setupOscWiring()
{
    setupOsc (osc1);
    setupOsc (osc2);
    setupOsc (osc3);

    // OSC 1 waveforms
    osc1.waveA.onClick = [this]()
    {
        showWaveformMenu (osc1.waveA, [this] (SynthVoice::Waveform w)
        {
            for (int i = 0; i < engine.synthesiser.getNumVoices(); ++i)
                if (auto* v = dynamic_cast<SynthVoice*> (engine.synthesiser.getVoice (i)))
                    v->setWaveformA (w);
        });
    };

    osc1.waveB.onClick = [this]()
    {
        showWaveformMenu (osc1.waveB, [this] (SynthVoice::Waveform w)
        {
            for (int i = 0; i < engine.synthesiser.getNumVoices(); ++i)
                if (auto* v = dynamic_cast<SynthVoice*> (engine.synthesiser.getVoice (i)))
                    v->setWaveformB (w);
        });
    };

    // OSC 1 morph
    osc1.morph.setRange (0.0, 1.0);
    osc1.morph.setValue (0.0);
    osc1.morph.onValueChange = [this]()
    {
        if (!engine.isReady) return;  // ← Guard
        float morph = (float) osc1.morph.getValue();
        for (int i = 0; i < engine.synthesiser.getNumVoices(); ++i)
            if (auto* v = dynamic_cast<SynthVoice*> (engine.synthesiser.getVoice (i)))
                v->setMorph (morph);
    };

    // OSC 1 vol / pan
    osc1.vol.setRange (0.0,  1.0);
    osc1.pan.setRange (-1.0, 1.0);
    osc1.pan.setValue (0.0);

    osc1.vol.onValueChange = [this]()
    {
        if (!engine.isReady) return;  // ← Guard
        float vol = (float) osc1.vol.getValue();
        for (int i = 0; i < engine.synthesiser.getNumVoices(); ++i)
            if (auto* v = dynamic_cast<SynthVoice*> (engine.synthesiser.getVoice (i)))
                v->setVolume (vol);
    };

    osc1.pan.onValueChange = [this]()
    {
        if (!engine.isReady) return;  // ← Guard
        float pan = (float) osc1.pan.getValue();
        for (int i = 0; i < engine.synthesiser.getNumVoices(); ++i)
            if (auto* v = dynamic_cast<SynthVoice*> (engine.synthesiser.getVoice (i)))
                v->setPan (pan);
    };

    // OSC 1 tuning
    osc1.oct .setRange (-4.0,   4.0,   1.0);
    osc1.semi.setRange (-12.0,  12.0,  1.0);
    osc1.fine.setRange (-100.0, 100.0);
    osc1.oct .setValue (0.0);
    osc1.semi.setValue (0.0);
    osc1.fine.setValue (0.0);

    auto updateOsc1Tuning = [this]()
    {
        if (!engine.isReady) return;  // ← Guard
        int   oct  = (int)   osc1.oct .getValue();
        int   semi = (int)   osc1.semi.getValue();
        float fine = (float) osc1.fine.getValue();
        for (int i = 0; i < engine.synthesiser.getNumVoices(); ++i)
            if (auto* v = dynamic_cast<SynthVoice*> (engine.synthesiser.getVoice (i)))
                v->setTuning (oct, semi, fine);
    };

    osc1.oct .onValueChange = updateOsc1Tuning;
    osc1.semi.onValueChange = updateOsc1Tuning;
    osc1.fine.onValueChange = updateOsc1Tuning;

    // OSC 2 waveforms
        osc2.waveA.onClick = [this]()
        {
            showWaveformMenu (osc2.waveA, [this] (SynthVoice::Waveform w)
            {
                for (int i = 0; i < engine.synthesiser.getNumVoices(); ++i)
                    if (auto* v = dynamic_cast<SynthVoice*> (engine.synthesiser.getVoice (i)))
                        v->setWaveformC (w);
            });
        };

        osc2.waveB.onClick = [this]()
        {
            showWaveformMenu (osc2.waveB, [this] (SynthVoice::Waveform w)
            {
                for (int i = 0; i < engine.synthesiser.getNumVoices(); ++i)
                    if (auto* v = dynamic_cast<SynthVoice*> (engine.synthesiser.getVoice (i)))
                        v->setWaveformD (w);
            });
        };

        osc2.morph.setRange (0.0, 1.0); osc2.morph.setValue (0.0);
        osc2.vol  .setRange (0.0, 1.0);
        osc2.pan  .setRange (-1.0, 1.0); osc2.pan.setValue (0.0);
        osc2.oct  .setRange (-4.0,   4.0,   1.0); osc2.oct .setValue (0.0);
        osc2.semi .setRange (-12.0,  12.0,  1.0); osc2.semi.setValue (0.0);
        osc2.fine .setRange (-100.0, 100.0);      osc2.fine.setValue (0.0);

        osc2.morph.onValueChange = [this]()
        {
            if (!engine.isReady) return;  // ← Guard
            float m = (float) osc2.morph.getValue();
            for (int i = 0; i < engine.synthesiser.getNumVoices(); ++i)
                if (auto* v = dynamic_cast<SynthVoice*> (engine.synthesiser.getVoice (i)))
                    v->setMorph2 (m);
        };
        osc2.vol.onValueChange = [this]()
        {
            if (!engine.isReady) return;  // ← Guard
            float vol = (float) osc2.vol.getValue();
            for (int i = 0; i < engine.synthesiser.getNumVoices(); ++i)
                if (auto* v = dynamic_cast<SynthVoice*> (engine.synthesiser.getVoice (i)))
                    v->setVolume2 (vol);
        };
        osc2.pan.onValueChange = [this]()
        {
            if (!engine.isReady) return;  // ← Guard
            float pan = (float) osc2.pan.getValue();
            for (int i = 0; i < engine.synthesiser.getNumVoices(); ++i)
                if (auto* v = dynamic_cast<SynthVoice*> (engine.synthesiser.getVoice (i)))
                    v->setPan2 (pan);
        };

        auto updateOsc2Tuning = [this]()
        {
            if (!engine.isReady) return;  // ← Guard
            int   oct  = (int)   osc2.oct .getValue();
            int   semi = (int)   osc2.semi.getValue();
            float fine = (float) osc2.fine.getValue();
            for (int i = 0; i < engine.synthesiser.getNumVoices(); ++i)
                if (auto* v = dynamic_cast<SynthVoice*> (engine.synthesiser.getVoice (i)))
                    v->setTuning2 (oct, semi, fine);
        };
        osc2.oct .onValueChange = updateOsc2Tuning;
        osc2.semi.onValueChange = updateOsc2Tuning;
        osc2.fine.onValueChange = updateOsc2Tuning;

        // OSC 3 waveforms
        osc3.waveA.onClick = [this]()
        {
            showWaveformMenu (osc3.waveA, [this] (SynthVoice::Waveform w)
            {
                for (int i = 0; i < engine.synthesiser.getNumVoices(); ++i)
                    if (auto* v = dynamic_cast<SynthVoice*> (engine.synthesiser.getVoice (i)))
                        v->setWaveformE (w);
            });
        };

        osc3.waveB.onClick = [this]()
        {
            showWaveformMenu (osc3.waveB, [this] (SynthVoice::Waveform w)
            {
                for (int i = 0; i < engine.synthesiser.getNumVoices(); ++i)
                    if (auto* v = dynamic_cast<SynthVoice*> (engine.synthesiser.getVoice (i)))
                        v->setWaveformF (w);
            });
        };

        osc3.morph.setRange (0.0, 1.0); osc3.morph.setValue (0.0);
        osc3.vol  .setRange (0.0, 1.0);
        osc3.pan  .setRange (-1.0, 1.0); osc3.pan.setValue (0.0);
        osc3.oct  .setRange (-4.0,   4.0,   1.0); osc3.oct .setValue (0.0);
        osc3.semi .setRange (-12.0,  12.0,  1.0); osc3.semi.setValue (0.0);
        osc3.fine .setRange (-100.0, 100.0);      osc3.fine.setValue (0.0);

        osc3.morph.onValueChange = [this]()
        {
            if (!engine.isReady) return;  // ← Guard
            float m = (float) osc3.morph.getValue();
            for (int i = 0; i < engine.synthesiser.getNumVoices(); ++i)
                if (auto* v = dynamic_cast<SynthVoice*> (engine.synthesiser.getVoice (i)))
                    v->setMorph3 (m);
        };
        osc3.vol.onValueChange = [this]()
        {
            if (!engine.isReady) return;  // ← Guard
            float vol = (float) osc3.vol.getValue();
            for (int i = 0; i < engine.synthesiser.getNumVoices(); ++i)
                if (auto* v = dynamic_cast<SynthVoice*> (engine.synthesiser.getVoice (i)))
                    v->setVolume3 (vol);
        };
        osc3.pan.onValueChange = [this]()
        {
            if (!engine.isReady) return;  // ← Guard
            float pan = (float) osc3.pan.getValue();
            for (int i = 0; i < engine.synthesiser.getNumVoices(); ++i)
                if (auto* v = dynamic_cast<SynthVoice*> (engine.synthesiser.getVoice (i)))
                    v->setPan3 (pan);
        };

        auto updateOsc3Tuning = [this]()
        {
            if (!engine.isReady) return;  // ← Guard
            int   oct  = (int)   osc3.oct .getValue();
            int   semi = (int)   osc3.semi.getValue();
            float fine = (float) osc3.fine.getValue();
            for (int i = 0; i < engine.synthesiser.getNumVoices(); ++i)
                if (auto* v = dynamic_cast<SynthVoice*> (engine.synthesiser.getVoice (i)))
                    v->setTuning3 (oct, semi, fine);
        };
        osc3.oct .onValueChange = updateOsc3Tuning;
        osc3.semi.onValueChange = updateOsc3Tuning;
        osc3.fine.onValueChange = updateOsc3Tuning;}
