#include "MainComponent.h"

void MainComponent::setupLfoWiring()
{
    // ---- LFO 1 ----
    lfo1.rate .setRange (0.01, 20.0); lfo1.rate .setValue (1.0,  juce::sendNotificationSync);
    lfo1.depth.setRange (0.0,  1.0);  lfo1.depth.setValue (0.5,  juce::sendNotificationSync); // 0.5 so LFO is audible
    lfo1.delay.setRange (0.0,  2000.0); lfo1.delay.setValue (0.0, juce::sendNotificationSync);

    addAndMakeVisible (lfo1.rate);
    addAndMakeVisible (lfo1.depth);
    addAndMakeVisible (lfo1.delay);

    setupLabel (lfo1.delayL, "Delay");
    addAndMakeVisible (lfo1.delay);
    
    lfo1.rate.onValueChange = [this]()
    {
        if (!engine.isReady) return;  // ← guard
        float rate = (float) lfo1.rate.getValue();
        for (int i = 0; i < engine.synthesiser.getNumVoices(); ++i)
            if (auto* v = dynamic_cast<SynthVoice*> (engine.synthesiser.getVoice (i)))
                v->setLfo1Rate (rate);
    };

    lfo1.depth.onValueChange = [this]()
    {
        if (!engine.isReady) return;  // ← guard
        float depth = (float) lfo1.depth.getValue();
        DBG ("depth callback fired, depth=" << depth);
        for (int i = 0; i < engine.synthesiser.getNumVoices(); ++i)
            if (auto* v = dynamic_cast<SynthVoice*> (engine.synthesiser.getVoice (i)))
                v->setLfo1Depth (depth);
    };

    lfo1.delay.onValueChange = [this]()
    {
        if (!engine.isReady) return;  // ← guard
        float ms = (float) lfo1.delay.getValue();
        for (int i = 0; i < engine.synthesiser.getNumVoices(); ++i)
            if (auto* v = dynamic_cast<SynthVoice*> (engine.synthesiser.getVoice (i)))
                v->setLfo1Delay (ms);
    };

    lfo1.shape.onClick = [this]()
    {
        juce::PopupMenu menu;
        menu.addItem (1, "Sine");
        menu.addItem (2, "Triangle");
        menu.addItem (3, "Square");
        menu.addItem (4, "Saw Up");
        menu.addItem (5, "Saw Down");

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (lfo1.shape),
            [this] (int result)
            {
                if (result == 0) return;
                const juce::String names[] = { "Sine", "Triangle", "Square", "Saw Up", "Saw Down" };
                lfo1.shape.setButtonText (names[result - 1]);
                int shape = result - 1;
                for (int i = 0; i < engine.synthesiser.getNumVoices(); ++i)
                    if (auto* v = dynamic_cast<SynthVoice*> (engine.synthesiser.getVoice (i)))
                        v->setLfo1Shape (shape);
            });
    };    
    // ---- LFO 1 ----
    lfo2.rate .setRange (0.01, 20.0); lfo2.rate .setValue (1.0,  juce::sendNotificationSync);
    lfo2.depth.setRange (0.0,  1.0);  lfo2.depth.setValue (0.5,  juce::sendNotificationSync); // 0.5 so LFO is audible
    lfo2.delay.setRange (0.0,  2000.0); lfo2.delay.setValue (0.0, juce::sendNotificationSync);

    addAndMakeVisible (lfo1.rate);
    addAndMakeVisible (lfo1.depth);
    addAndMakeVisible (lfo1.delay);
    
    setupLabel (lfo2.delayL, "Delay");
    addAndMakeVisible (lfo2.delay);
}
