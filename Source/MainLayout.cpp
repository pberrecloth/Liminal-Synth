#include "MainComponent.h"

void MainComponent::setupSlider (juce::Slider& s)
{
    s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    s.setRange (0.0, 1.0);
    s.setValue (0.5);
    addAndMakeVisible (s);
}

void MainComponent::setupLabel (juce::Label& l, const juce::String& text)
{
    l.setText (text, juce::dontSendNotification);
    l.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (l);
}

void MainComponent::setupLfo (LfoSection& lfo)
{
    addAndMakeVisible (lfo.shape);
    setupSlider (lfo.rate);  setupLabel (lfo.rateL,  "Rate");
    setupSlider (lfo.depth); setupLabel (lfo.depthL, "Depth");
    setupSlider (lfo.delay); setupLabel (lfo.delayL, "Delay");

    lfo.rate .setRange (0.01, 20.0);  lfo.rate .setValue (2.0);
    lfo.depth.setRange (0.0,  1.0);   lfo.depth.setValue (0.0);
    lfo.delay.setRange (0.0,  2000.0); lfo.delay.setValue (0.0);
}

void MainComponent::setupModRow (ModRow& row)
{
    addAndMakeVisible (row.source);
    addAndMakeVisible (row.dest);
    setupSlider (row.amount);
    setupLabel  (row.amountL, "Amount");
}

void MainComponent::drawPanel (juce::Graphics& g, juce::Rectangle<int> area,
                                const juce::String& title)
{
    g.setColour (juce::Colours::grey.withAlpha (0.15f));
    g.fillRoundedRectangle (area.toFloat(), 4.0f);
    g.setColour (juce::Colours::grey.withAlpha (0.5f));
    g.drawRoundedRectangle (area.toFloat(), 4.0f, 1.0f);
    g.setColour (juce::Colours::lightgrey);
    g.drawText (title, area.removeFromTop (18), juce::Justification::centred);
}

void MainComponent::layoutOsc (OscSection& osc, juce::Rectangle<int> area)
{
    auto topRow = area.removeFromTop (28);
    osc.waveA.setBounds (topRow.removeFromLeft (90));
    topRow.removeFromLeft (kGap);
    osc.waveB.setBounds (topRow.removeFromLeft (90));
    area.removeFromTop (kGap);

    auto knobRow = area.removeFromTop (kKnob + kLabel);
    auto placeKnob = [&] (juce::Slider& s, juce::Label& l)
    {
        auto cell = knobRow.removeFromLeft (kKnob);
        s.setBounds (cell.removeFromTop (kKnob));
        l.setBounds (cell.removeFromTop (kLabel));
        knobRow.removeFromLeft (kGap);
    };
    placeKnob (osc.morph, osc.morphL);
    placeKnob (osc.oct,   osc.octL);
    placeKnob (osc.semi,  osc.semiL);
    placeKnob (osc.fine,  osc.fineL);
    placeKnob (osc.vol,   osc.volL);
    placeKnob (osc.pan,   osc.panL);
}

void MainComponent::layoutFilter (FilterSection& f, juce::Rectangle<int> area)
{
    f.type.setBounds (area.removeFromTop (24));
    area.removeFromTop (kGap);

    auto row1 = area.removeFromTop (kKnob + kLabel);
    auto placeKnob = [&] (juce::Rectangle<int>& row, juce::Slider& s, juce::Label& l)
    {
        auto cell = row.removeFromLeft (kKnob);
        s.setBounds (cell.removeFromTop (kKnob));
        l.setBounds (cell.removeFromTop (kLabel));
        row.removeFromLeft (kGap);
    };
    placeKnob (row1, f.cutoff,   f.cutoffL);
    placeKnob (row1, f.res,      f.resL);
    placeKnob (row1, f.drive,    f.driveL);
    placeKnob (row1, f.keytrack, f.keytrackL);
    placeKnob (row1, f.velocity, f.velocityL);

    area.removeFromTop (kGap);

    auto row2 = area.removeFromTop (kKnob + kLabel);
    placeKnob (row2, f.envA,   f.envAL);
    placeKnob (row2, f.envD,   f.envDL);
    placeKnob (row2, f.envS,   f.envSL);
    placeKnob (row2, f.envR,   f.envRL);
    placeKnob (row2, f.envAmt, f.envAmtL);
}

void MainComponent::layoutLfo (LfoSection& lfo, juce::Rectangle<int> area)
{
    lfo.shape.setBounds (area.removeFromTop (24));
    area.removeFromTop (kGap);

    auto knobRow = area.removeFromTop (kKnob + kLabel);
    auto placeKnob = [&] (juce::Slider& s, juce::Label& l)
    {
        auto cell = knobRow.removeFromLeft (kKnob);
        s.setBounds (cell.removeFromTop (kKnob));
        l.setBounds (cell.removeFromTop (kLabel));
        knobRow.removeFromLeft (kGap);
    };
    placeKnob (lfo.rate,  lfo.rateL);
    placeKnob (lfo.depth, lfo.depthL);
    placeKnob (lfo.delay, lfo.delayL);
}

void MainComponent::layoutModRow (ModRow& row, juce::Rectangle<int> area)
{
    row.source.setBounds (area.removeFromLeft (80));
    area.removeFromLeft (kGap);
    row.dest  .setBounds (area.removeFromLeft (80));
    area.removeFromLeft (kGap);
    auto cell = area.removeFromLeft (kKnob);
    row.amount .setBounds (cell.removeFromTop (kKnob));
    row.amountL.setBounds (cell.removeFromTop (kLabel));
}

void MainComponent::layoutEnv (juce::Rectangle<int> area,
                                juce::Slider& a, juce::Label& al,
                                juce::Slider& d, juce::Label& dl,
                                juce::Slider& s, juce::Label& sl,
                                juce::Slider& r, juce::Label& rl)
{
    auto placeKnob = [&] (juce::Slider& knob, juce::Label& label)
    {
        auto cell = area.removeFromLeft (kKnob);
        knob .setBounds (cell.removeFromTop (kKnob));
        label.setBounds (cell.removeFromTop (kLabel));
        area.removeFromLeft (kGap);
    };
    placeKnob (a, al);
    placeKnob (d, dl);
    placeKnob (s, sl);
    placeKnob (r, rl);
}

void MainComponent::resized()
{
    const int col1W = 480;
    const int col2W = getWidth() - col1W - kPad * 3;
    int y           = topBarHeight + kPad;

    auto bar = getLocalBounds().removeFromTop (topBarHeight).reduced (4, 4);
    settingsButton.setBounds (bar.removeFromLeft (120));
    bar.removeFromLeft (4);
    midiButton    .setBounds (bar.removeFromLeft (120));
    bar.removeFromLeft (4);
    initButton    .setBounds (bar.removeFromLeft (100));
    saveButton    .setBounds (bar.removeFromRight (80));
    patchNameLabel.setBounds (bar);

    {
        auto area = juce::Rectangle<int> (kPad + 8, y + 20, 200, kKnob + kLabel);
        auto placeKnob = [&] (juce::Slider& s, juce::Label& l)
        {
            s.setBounds (area.removeFromLeft (kKnob));
            area.removeFromLeft (4);
            l.setBounds (area.removeFromLeft (kKnob));
            area.removeFromLeft (kGap);
        };
        placeKnob (masterVolume, masterVolumeL);
        placeKnob (masterPan,    masterPanL);
    }
    y += kMaster + kPad;

    const int col1X = kPad;
    const int col2X = kPad * 2 + col1W;
    int col1Y = y;
    int col2Y = y;

    auto layoutOscPanel = [&] (OscSection& osc)
    {
        layoutOsc (osc, juce::Rectangle<int> (col1X + kPad, col1Y + 20,
                                               col1W - kPad * 2, kOscH - 24));
        col1Y += kOscH + kPad;
    };
    layoutOscPanel (osc1);
    layoutOscPanel (osc2);
    layoutOscPanel (osc3);

    int matH     = col2Y + kFilH + kPad + kFil3H + kPad + kEnvH + kPad + kLfoH - col1Y;
    matH         = juce::jmax (kMatH, matH);
    auto matArea = juce::Rectangle<int> (col1X + kPad, col1Y + 20,
                                          col1W - kPad * 2, matH - 24);
    int rowH = matArea.getHeight() / 4;
    layoutModRow (mod1, matArea.removeFromTop (rowH));
    layoutModRow (mod2, matArea.removeFromTop (rowH));
    layoutModRow (mod3, matArea.removeFromTop (rowH));
    layoutModRow (mod4, matArea);

    int filterW = (col2W - kPad) / 2;
    layoutFilter (filter1, juce::Rectangle<int> (col2X + kPad,               col2Y + 20,
                                                  filterW - kPad * 2,         kFilH - 24));
    layoutFilter (filter2, juce::Rectangle<int> (col2X + filterW + kPad * 2, col2Y + 20,
                                                  filterW - kPad * 2,         kFilH - 24));
    col2Y += kFilH + kPad;

    {
        auto area = juce::Rectangle<int> (col2X + kPad, col2Y + 20,
                                           col2W - kPad * 2, kFil3H - 24);
        filter3On    .setBounds (area.removeFromLeft (80));
        area.removeFromLeft (kGap);
        filter3KeyTrk.setBounds (area.removeFromLeft (100));
        area.removeFromLeft (kGap * 3);
        auto placeKnob = [&] (juce::Slider& s, juce::Label& l)
        {
            auto cell = area.removeFromLeft (kKnob);
            s.setBounds (cell.removeFromTop (kKnob));
            l.setBounds (cell.removeFromTop (kLabel));
            area.removeFromLeft (kGap);
        };
        placeKnob (filter3Freq, filter3FreqL);
        placeKnob (filter3Q,    filter3QL);
        placeKnob (filterBlend, filterBlendL);
    }
    col2Y += kFil3H + kPad;

    int envW = (col2W - kPad) / 2;
    layoutEnv (juce::Rectangle<int> (col2X + kPad,            col2Y + 20,
                                      envW - kPad * 2,         kEnvH - 24),
               ampA, ampAL, ampD, ampDL, ampS, ampSL, ampR, ampRL);
    layoutEnv (juce::Rectangle<int> (col2X + envW + kPad * 2, col2Y + 20,
                                      envW - kPad * 2,         kEnvH - 24),
               env3A, env3AL, env3D, env3DL, env3S, env3SL, env3R, env3RL);
    col2Y += kEnvH + kPad;

    int lfoW = (col2W - kPad) / 2;
    layoutLfo (lfo1, juce::Rectangle<int> (col2X + kPad,              col2Y + 20,
                                            lfoW - kPad * 2,           kLfoH - 24));
    layoutLfo (lfo2, juce::Rectangle<int> (col2X + lfoW + kPad * 2,   col2Y + 20,
                                            lfoW - kPad * 2,           kLfoH - 24));
    col2Y += kLfoH + kPad;

    int bottomY = juce::jmax (col1Y + matH + kPad, col2Y);
    {
        auto area = juce::Rectangle<int> (kPad + 8, bottomY + 20,
                                           getWidth() - kPad * 2 - 16, kFxH - 24);
        fxPresetL.setBounds (area.removeFromLeft (50));
        area.removeFromLeft (4);
        fxPreset .setBounds (area.removeFromLeft (100));
        area.removeFromLeft (kGap * 4);

        auto placeKnob = [&] (juce::Slider& s, juce::Label& l)
        {
            auto cell = area.removeFromLeft (kKnob);
            s.setBounds (cell.removeFromTop (kKnob));
            l.setBounds (cell.removeFromTop (kLabel));
            area.removeFromLeft (kGap);
        };
        placeKnob (revSize,     revSizeL);
        placeKnob (revMix,      revMixL);
        area.removeFromLeft (kGap * 4);
        placeKnob (delTime,     delTimeL);
        placeKnob (delFeedback, delFeedbackL);
        placeKnob (delMix,      delMixL);
    }
}
