#include "MainComponent.h"

void MainComponent::setupFilter (FilterSection& f)
{
    addAndMakeVisible (f.type);
    addAndMakeVisible (f.envOn);
    f.envOn.setToggleState (true, juce::dontSendNotification); // default: env active
    setupSlider (f.cutoff);   setupLabel (f.cutoffL,   "Cutoff");
    setupSlider (f.res);      setupLabel (f.resL,      "Res");
    setupSlider (f.drive);    setupLabel (f.driveL,    "Drive");
    setupSlider (f.keytrack); setupLabel (f.keytrackL, "KeyTrk");
    setupSlider (f.velocity); setupLabel (f.velocityL, "Vel");
    setupSlider (f.envA);     setupLabel (f.envAL,     "A");
    setupSlider (f.envD);     setupLabel (f.envDL,     "D");
    setupSlider (f.envS);     setupLabel (f.envSL,     "S");
    setupSlider (f.envR);     setupLabel (f.envRL,     "R");
    setupSlider (f.envAmt);   setupLabel (f.envAmtL,   "Amt");
}

static void showFilterTypeMenu (juce::TextButton& btn,
                                 std::function<void(juce::dsp::StateVariableTPTFilterType,
                                                    const juce::String&)> onSelect)
{
    juce::PopupMenu menu;
    menu.addItem (1, "Low Pass");
    menu.addItem (2, "High Pass");
    menu.addItem (3, "Band Pass");
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (btn),
        [onSelect] (int result)
        {
            if (result == 0) return;
            juce::String name;
            juce::dsp::StateVariableTPTFilterType type;
            switch (result)
            {
                case 1: name = "Low Pass";  type = juce::dsp::StateVariableTPTFilterType::lowpass;  break;
                case 2: name = "High Pass"; type = juce::dsp::StateVariableTPTFilterType::highpass; break;
                case 3: name = "Band Pass"; type = juce::dsp::StateVariableTPTFilterType::bandpass; break;
                default: return;
            }
            onSelect (type, name);
        });
}

void MainComponent::setupFilterWiring()
{
    setupFilter (filter1);
    setupFilter (filter2);

    // ---- Filter 1 ranges ----
    filter1.cutoff  .setRange (20.0, 20000.0); filter1.cutoff.setValue (20000.0);
    filter1.cutoff  .setSkewFactorFromMidPoint (1000.0);
    filter1.res     .setRange (0.1,  10.0);    filter1.res    .setValue (0.7);
    filter1.drive   .setRange (1.0,  10.0);    filter1.drive  .setValue (1.0);
    filter1.drive   .setSkewFactorFromMidPoint (3.0);
    filter1.keytrack.setRange (0.0,  1.0);     filter1.keytrack.setValue (0.0);
    filter1.velocity.setRange (0.0,  1.0);     filter1.velocity.setValue (0.0);

    filter1.envA.setRange (0.0, 5.0); filter1.envA.setSkewFactorFromMidPoint (0.3); filter1.envA.setValue (0.1);
    filter1.envD.setRange (0.0, 5.0); filter1.envD.setSkewFactorFromMidPoint (0.3); filter1.envD.setValue (0.2);
    filter1.envS.setRange (0.0, 1.0);                                                filter1.envS.setValue (0.7);
    filter1.envR.setRange (0.0, 5.0); filter1.envR.setSkewFactorFromMidPoint (0.5); filter1.envR.setValue (0.4);
    filter1.envAmt.setRange (-1.0, 1.0);                                              filter1.envAmt.setValue (0.0);

    auto updateFilter1 = [this]()
    {
        if (!engine.isReady) return;
        float cutoff   = (float) filter1.cutoff  .getValue();
        float res      = (float) filter1.res     .getValue();
        float drive    = (float) filter1.drive   .getValue();
        float keytrack = (float) filter1.keytrack.getValue();
        float velocity = (float) filter1.velocity.getValue();
        for (int i = 0; i < engine.synthesiser.getNumVoices(); ++i)
            if (auto* v = dynamic_cast<SynthVoice*> (engine.synthesiser.getVoice (i)))
                v->setFilter1Params (cutoff, res, drive, keytrack, velocity);
    };

    auto updateFilter1Env = [this]()
    {
        if (!engine.isReady) return;
        juce::ADSR::Parameters p;
        p.attack  = (float) filter1.envA  .getValue();
        p.decay   = (float) filter1.envD  .getValue();
        p.sustain = (float) filter1.envS  .getValue();
        p.release = (float) filter1.envR  .getValue();
        float amt = (float) filter1.envAmt.getValue();
        for (int i = 0; i < engine.synthesiser.getNumVoices(); ++i)
            if (auto* v = dynamic_cast<SynthVoice*> (engine.synthesiser.getVoice (i)))
                v->setFilter1EnvParams (p, amt);
    };

    filter1.cutoff  .onValueChange = updateFilter1;
    filter1.res     .onValueChange = updateFilter1;
    filter1.drive   .onValueChange = updateFilter1;
    filter1.keytrack.onValueChange = updateFilter1;
    filter1.velocity.onValueChange = updateFilter1;
    filter1.envA    .onValueChange = updateFilter1Env;
    filter1.envD    .onValueChange = updateFilter1Env;
    filter1.envS    .onValueChange = updateFilter1Env;
    filter1.envR    .onValueChange = updateFilter1Env;
    filter1.envAmt  .onValueChange = updateFilter1Env;

    filter1.type.onClick = [this]()
    {
        if (!engine.isReady) return;
        showFilterTypeMenu (filter1.type, [this] (auto type, auto name)
        {
            filter1.type.setButtonText (name);
            for (int i = 0; i < engine.synthesiser.getNumVoices(); ++i)
                if (auto* v = dynamic_cast<SynthVoice*> (engine.synthesiser.getVoice (i)))
                    v->setFilter1Type (type);
        });
    };

    // ---- Filter 2 ranges ----
    filter2.cutoff  .setRange (20.0, 20000.0); filter2.cutoff.setValue (20000.0);
    filter2.cutoff  .setSkewFactorFromMidPoint (1000.0);
    filter2.res     .setRange (0.1,  10.0);    filter2.res.setValue (0.7);
    filter2.drive   .setRange (1.0,  10.0);    filter2.drive  .setValue (1.0);
    filter2.drive   .setSkewFactorFromMidPoint (3.0);
    filter2.keytrack.setRange (0.0,  1.0);     filter2.keytrack.setValue (0.0);
    filter2.velocity.setRange (0.0,  1.0);     filter2.velocity.setValue (0.0);

    filter2.envA.setRange (0.0, 5.0); filter2.envA.setSkewFactorFromMidPoint (0.3); filter2.envA.setValue (0.1);
    filter2.envD.setRange (0.0, 5.0); filter2.envD.setSkewFactorFromMidPoint (0.3); filter2.envD.setValue (0.2);
    filter2.envS.setRange (0.0, 1.0);                                                filter2.envS.setValue (0.7);
    filter2.envR.setRange (0.0, 5.0); filter2.envR.setSkewFactorFromMidPoint (0.5); filter2.envR.setValue (0.4);
    filter2.envAmt.setRange (-1.0, 1.0);                                              filter2.envAmt.setValue (0.0);

    auto updateFilter2 = [this]()
    {
        if (!engine.isReady) return;
        float cutoff   = (float) filter2.cutoff  .getValue();
        float res      = (float) filter2.res     .getValue();
        float drive    = (float) filter2.drive   .getValue();
        float keytrack = (float) filter2.keytrack.getValue();
        float velocity = (float) filter2.velocity.getValue();
        for (int i = 0; i < engine.synthesiser.getNumVoices(); ++i)
            if (auto* v = dynamic_cast<SynthVoice*> (engine.synthesiser.getVoice (i)))
                v->setFilter2Params (cutoff, res, drive, keytrack, velocity);
    };

    auto updateFilter2Env = [this]()
    {
        if (!engine.isReady) return;
        juce::ADSR::Parameters p;
        p.attack  = (float) filter2.envA  .getValue();
        p.decay   = (float) filter2.envD  .getValue();
        p.sustain = (float) filter2.envS  .getValue();
        p.release = (float) filter2.envR  .getValue();
        float amt = (float) filter2.envAmt.getValue();
        for (int i = 0; i < engine.synthesiser.getNumVoices(); ++i)
            if (auto* v = dynamic_cast<SynthVoice*> (engine.synthesiser.getVoice (i)))
                v->setFilter2EnvParams (p, amt);
    };

    filter2.cutoff  .onValueChange = updateFilter2;
    filter2.res     .onValueChange = updateFilter2;
    filter2.drive   .onValueChange = updateFilter2;
    filter2.keytrack.onValueChange = updateFilter2;
    filter2.velocity.onValueChange = updateFilter2;
    filter2.envA    .onValueChange = updateFilter2Env;
    filter2.envD    .onValueChange = updateFilter2Env;
    filter2.envS    .onValueChange = updateFilter2Env;
    filter2.envR    .onValueChange = updateFilter2Env;
    filter2.envAmt  .onValueChange = updateFilter2Env;

    filter2.type.onClick = [this]()
    {
        if (!engine.isReady) return;
        showFilterTypeMenu (filter2.type, [this] (auto type, auto name)
        {
            filter2.type.setButtonText (name);
            for (int i = 0; i < engine.synthesiser.getNumVoices(); ++i)
                if (auto* v = dynamic_cast<SynthVoice*> (engine.synthesiser.getVoice (i)))
                    v->setFilter2Type (type);
        });
    };

    // ---- Filter env on/off toggles ----
    filter1.envOn.onStateChange = [this]()
    {
        if (!engine.isReady) return;
        bool bypassed = !filter1.envOn.getToggleState();
        for (int i = 0; i < engine.synthesiser.getNumVoices(); ++i)
            if (auto* v = dynamic_cast<SynthVoice*> (engine.synthesiser.getVoice (i)))
                v->setFilter1EnvBypass (bypassed);
    };
    filter2.envOn.onStateChange = [this]()
    {
        if (!engine.isReady) return;
        bool bypassed = !filter2.envOn.getToggleState();
        for (int i = 0; i < engine.synthesiser.getNumVoices(); ++i)
            if (auto* v = dynamic_cast<SynthVoice*> (engine.synthesiser.getVoice (i)))
                v->setFilter2EnvBypass (bypassed);
    };

    // ---- Filter blend ----
    setupSlider (filterBlend); setupLabel (filterBlendL, "Blend");
    filterBlend.setRange (0.0, 1.0);
    filterBlend.setValue (1.0);
    filterBlend.onValueChange = [this]()
    {
        if (!engine.isReady) return;
        float blend = (float) filterBlend.getValue();
        for (int i = 0; i < engine.synthesiser.getNumVoices(); ++i)
            if (auto* v = dynamic_cast<SynthVoice*> (engine.synthesiser.getVoice (i)))
                v->setFilterBlend (blend);
    };

    // ---- Filter 3 HP master ----
    addAndMakeVisible (filter3On);
    addAndMakeVisible (filter3KeyTrk);
    setupSlider (filter3Freq); setupLabel (filter3FreqL, "Freq");
    setupSlider (filter3Q);    setupLabel (filter3QL,    "Q");
    filter3Freq.setRange (20.0, 20000.0);
    filter3Freq.setValue (20.0);
    filter3Freq.setSkewFactorFromMidPoint (1000.0);
    filter3On  .onClick       = [this]() { engine.setFilter3On   (filter3On.getToggleState()); };
    filter3Freq.onValueChange = [this]() { engine.setFilter3Freq ((float) filter3Freq.getValue()); };
}
