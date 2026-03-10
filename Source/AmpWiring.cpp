#include "MainComponent.h"

void MainComponent::setupAmpWiring()
{
    // ---- Amp Env ----
    setupSlider (ampA); setupLabel (ampAL, "A");
    setupSlider (ampD); setupLabel (ampDL, "D");
    setupSlider (ampS); setupLabel (ampSL, "S");
    setupSlider (ampR); setupLabel (ampRL, "R");

    ampA.setRange (0.005, 5.0); ampA.setSkewFactorFromMidPoint (0.3);
    ampD.setRange (0.001, 5.0); ampD.setSkewFactorFromMidPoint (0.3);
    ampS.setRange (0.0,   1.0);
    ampR.setRange (0.02,  5.0); ampR.setSkewFactorFromMidPoint (0.5);

    ampA.setValue (0.1);
    ampD.setValue (0.2);
    ampS.setValue (0.7);
    ampR.setValue (0.4);

    auto updateAdsr = [this]()
    {
        if (!engine.isReady) return;
        juce::ADSR::Parameters params;
        params.attack  = (float) ampA.getValue();
        params.decay   = (float) ampD.getValue();
        params.sustain = (float) ampS.getValue();
        params.release = (float) ampR.getValue();
        for (int i = 0; i < engine.synthesiser.getNumVoices(); ++i)
            if (auto* v = dynamic_cast<SynthVoice*> (engine.synthesiser.getVoice (i)))
                v->setAdsrParams (params);
    };

    ampA.onValueChange = updateAdsr;
    ampD.onValueChange = updateAdsr;
    ampS.onValueChange = updateAdsr;
    ampR.onValueChange = updateAdsr;

    // ---- Env 3 (free) ----
    setupSlider (env3A); setupLabel (env3AL, "A");
    setupSlider (env3D); setupLabel (env3DL, "D");
    setupSlider (env3S); setupLabel (env3SL, "S");
    setupSlider (env3R); setupLabel (env3RL, "R");

    env3A.setRange (0.005, 5.0); env3A.setSkewFactorFromMidPoint (0.3);
    env3D.setRange (0.001, 5.0); env3D.setSkewFactorFromMidPoint (0.3);
    env3S.setRange (0.0,   1.0);
    env3R.setRange (0.02,  5.0); env3R.setSkewFactorFromMidPoint (0.5);

    env3A.setValue (0.1);
    env3D.setValue (0.2);
    env3S.setValue (0.7);
    env3R.setValue (0.4);
}
