#pragma once
#include <JuceHeader.h>
#include <deque>
#include "PluginProcessor.h"

//==============================================================================
class GritLookAndFeel : public juce::LookAndFeel_V4
{
public:
    GritLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
};

//==============================================================================
class KnobControl : public juce::Component
{
public:
    KnobControl (juce::AudioProcessorValueTreeState&, const juce::String& id, const juce::String& text);
    void resized() override;
private:
    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
};

class ChoiceControl : public juce::Component
{
public:
    ChoiceControl (juce::AudioProcessorValueTreeState&, const juce::String& id, const juce::String& text);
    void resized() override;
private:
    juce::ComboBox box;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attach;
};

class ToggleControl : public juce::Component
{
public:
    ToggleControl (juce::AudioProcessorValueTreeState&, const juce::String& id, const juce::String& text);
    void resized() override;
private:
    juce::ToggleButton button;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attach;
};

//==============================================================================
class GritwaveEditor : public juce::AudioProcessorEditor
{
public:
    explicit GritwaveEditor (GritwaveProcessor&);
    ~GritwaveEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Panel
    {
        juce::String title;
        int cols = 1;
        std::vector<juce::Component*> comps;
        juce::Rectangle<int> area;
    };

    int addPanel (const juce::String& title, int cols);
    void knob   (int panel, const juce::String& id, const juce::String& text);
    void choice (int panel, const juce::String& id, const juce::String& text);
    void toggle (int panel, const juce::String& id, const juce::String& text);
    void layoutPanel (Panel& p);

    GritwaveProcessor& proc;
    GritLookAndFeel lnf;

    std::vector<std::unique_ptr<juce::Component>> owned;
    std::deque<Panel> panels;

    juce::ComboBox presetBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GritwaveEditor)
};
