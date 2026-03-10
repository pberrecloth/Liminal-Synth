#pragma once

#include <JuceHeader.h>

class Styles : public juce::LookAndFeel_V4
{
public:
    Styles()
    {
        // Knob
        setColour (juce::Slider::thumbColourId,                juce::Colour (0xff00c8ff));
        setColour (juce::Slider::rotarySliderFillColourId,     juce::Colour (0xff00c8ff));
        setColour (juce::Slider::rotarySliderOutlineColourId,  juce::Colour (0xff333333));

        // Buttons
        setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff2a2a2a));
        setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff0077aa));
        setColour (juce::TextButton::textColourOffId,  juce::Colour (0xffdddddd));
        setColour (juce::TextButton::textColourOnId,   juce::Colour (0xff00c8ff));

        // Labels
        setColour (juce::Label::textColourId, juce::Colour (0xff888888));

        // Toggle
        setColour (juce::ToggleButton::textColourId,         juce::Colour (0xffdddddd));
        setColour (juce::ToggleButton::tickColourId,         juce::Colour (0xff00c8ff));
        setColour (juce::ToggleButton::tickDisabledColourId, juce::Colour (0xff888888));

        // Background
        setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (0xff111111));
    }

    //==========================================================================
    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPos,
                           const float rotaryStartAngle,
                           const float rotaryEndAngle,
                           juce::Slider&) override
    {
        auto radius  = (float) juce::jmin (width / 2, height / 2) - 4.0f;
        auto centreX = (float) x + (float) width  * 0.5f;
        auto centreY = (float) y + (float) height * 0.5f;
        auto rx      = centreX - radius;
        auto ry      = centreY - radius;
        auto rw      = radius * 2.0f;
        auto angle   = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // Background circle
        g.setColour (juce::Colour (0xff333333));
        g.fillEllipse (rx, ry, rw, rw);

        // Outline
        g.setColour (juce::Colour (0xff444444));
        g.drawEllipse (rx, ry, rw, rw, 1.0f);

        // Filled arc
        juce::Path arc;
        arc.addArc (rx, ry, rw, rw, rotaryStartAngle, angle, true);
        g.setColour (juce::Colour (0xff00c8ff));
        g.strokePath (arc, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));

        // Pointer
        juce::Path pointer;
        auto pointerLength    = radius * 0.4f;
        auto pointerThickness = 2.0f;
        pointer.addRectangle (-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
        pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centreX, centreY));
        g.setColour (juce::Colour (0xff00c8ff));
        g.fillPath (pointer);
    }

    //==========================================================================
    void drawButtonBackground (juce::Graphics& g,
                               juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool /*isMouseOver*/,
                               bool isButtonDown) override
    {
        auto buttonArea = button.getLocalBounds().toFloat().reduced (0.5f);
        auto colour     = isButtonDown ? backgroundColour.brighter (0.2f) : backgroundColour;

        g.setColour (colour);
        g.fillRoundedRectangle (buttonArea, 3.0f);

        g.setColour (juce::Colour (0xff444444));
        g.drawRoundedRectangle (buttonArea, 3.0f, 1.0f);
    }
};
