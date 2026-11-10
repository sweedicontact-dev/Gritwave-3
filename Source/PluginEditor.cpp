#include "PluginEditor.h"

namespace
{
const juce::Colour bgDark   { 0xff0d0d12 };
const juce::Colour panelCol { 0xff1a1a23 };
const juce::Colour accentG  { 0xff7cff4f };
const juce::Colour accentM  { 0xffd946ef };
const juce::Colour textCol  { 0xffd8d8e0 };
}

//==============================================================================
GritLookAndFeel::GritLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, textCol);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, textCol);
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff262633));
    setColour (juce::ComboBox::textColourId, textCol);
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff3a3a4a));
    setColour (juce::ComboBox::arrowColourId, accentG);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff1e1e28));
    setColour (juce::PopupMenu::textColourId, textCol);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, accentM.darker (0.4f));
    setColour (juce::TextButton::buttonColourId, juce::Colour (0xff262633));
    setColour (juce::TextButton::textColourOffId, accentG);
    setColour (juce::ToggleButton::textColourId, textCol);
    setColour (juce::ToggleButton::tickColourId, accentG);
}

void GritLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float startAngle, float endAngle, juce::Slider&)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.f);
    const float r = std::min (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);

    g.setColour (juce::Colour (0xff23232f));
    g.fillEllipse (c.x - r, c.y - r, r * 2.f, r * 2.f);

    const float ar = r - 2.f;
    juce::Path track;
    track.addCentredArc (c.x, c.y, ar, ar, 0.f, startAngle, endAngle, true);
    g.setColour (juce::Colour (0xff3a3a4c));
    g.strokePath (track, juce::PathStrokeType (3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path val;
    val.addCentredArc (c.x, c.y, ar, ar, 0.f, startAngle, angle, true);
    g.setColour (accentG);
    g.strokePath (val, juce::PathStrokeType (3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path ptr;
    ptr.addRoundedRectangle (-1.5f, -r + 5.f, 3.f, r * 0.4f, 1.5f);
    g.setColour (accentM);
    g.fillPath (ptr, juce::AffineTransform::rotation (angle).translated (c.x, c.y));
}

//==============================================================================
KnobControl::KnobControl (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& text)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 62, 14);
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::Font (juce::FontOptions (11.f)));
    addAndMakeVisible (slider);
    addAndMakeVisible (label);
    attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, id, slider);
}

void KnobControl::resized()
{
    auto b = getLocalBounds();
    label.setBounds (b.removeFromTop (14));
    slider.setBounds (b);
}

ChoiceControl::ChoiceControl (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& text)
{
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (s.getParameter (id)))
        box.addItemList (p->choices, 1);
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::Font (juce::FontOptions (11.f)));
    addAndMakeVisible (box);
    addAndMakeVisible (label);
    attach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, id, box);
}

void ChoiceControl::resized()
{
    auto b = getLocalBounds().reduced (3, 0);
    label.setBounds (b.removeFromTop (14));
    box.setBounds (b.withSizeKeepingCentre (b.getWidth(), std::min (24, b.getHeight())));
}

ToggleControl::ToggleControl (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& text)
{
    button.setButtonText (text);
    addAndMakeVisible (button);
    attach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, id, button);
}

void ToggleControl::resized()
{
    auto b = getLocalBounds();
    button.setBounds (b.withSizeKeepingCentre (b.getWidth(), std::min (24, b.getHeight())));
}

//==============================================================================
int GritwaveEditor::addPanel (const juce::String& title, int cols)
{
    Panel p;
    p.title = title;
    p.cols = cols;
    panels.push_back (p);
    return (int) panels.size() - 1;
}

void GritwaveEditor::knob (int panel, const juce::String& id, const juce::String& text)
{
    auto c = std::make_unique<KnobControl> (proc.apvts, id, text);
    addAndMakeVisible (*c);
    panels[(size_t) panel].comps.push_back (c.get());
    owned.push_back (std::move (c));
}

void GritwaveEditor::choice (int panel, const juce::String& id, const juce::String& text)
{
    auto c = std::make_unique<ChoiceControl> (proc.apvts, id, text);
    addAndMakeVisible (*c);
    panels[(size_t) panel].comps.push_back (c.get());
    owned.push_back (std::move (c));
}

void GritwaveEditor::toggle (int panel, const juce::String& id, const juce::String& text)
{
    auto c = std::make_unique<ToggleControl> (proc.apvts, id, text);
    addAndMakeVisible (*c);
    panels[(size_t) panel].comps.push_back (c.get());
    owned.push_back (std::move (c));
}

//==============================================================================
GritwaveEditor::GritwaveEditor (GritwaveProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);

    // Fila 1: osciladores y voz  (panels 0..2)
    for (auto pre : { "a", "b" })
    {
        const juce::String s (pre);
        const int pn = addPanel (s == "a" ? "OSC A" : "OSC B", 5);
        knob   (pn, s + "_wave", "Wave");
        choice (pn, s + "_wmode", "Warp Mode");
        knob   (pn, s + "_warp", "Warp");
        knob   (pn, s + "_oct", "Oct");
        knob   (pn, s + "_semi", "Semi");
        knob   (pn, s + "_fine", "Fine");
        knob   (pn, s + "_uni", "Unison");
        knob   (pn, s + "_detune", "Detune");
        knob   (pn, s + "_spread", "Spread");
        knob   (pn, s + "_level", "Level");
    }
    {
        const int pn = addPanel ("SUB / VOICE", 2);
        knob (pn, "sub_level", "Sub");
        knob (pn, "noise_level", "Noise");
        toggle (pn, "mono", "Mono");
        knob (pn, "glide", "Glide");
    }

    // Fila 2: filtro y envolventes  (panels 3..5)
    {
        const int pn = addPanel ("FILTER", 5);
        choice (pn, "f_type", "Type");
        knob (pn, "f_cutoff", "Cutoff / Vowel");
        knob (pn, "f_res", "Reso");
        knob (pn, "f_drive", "Drive");
        knob (pn, "f_key", "KeyTrk");
    }
    {
        const int pn = addPanel ("AMP ENV", 4);
        knob (pn, "amp_a", "A"); knob (pn, "amp_d", "D"); knob (pn, "amp_s", "S"); knob (pn, "amp_r", "R");
    }
    {
        const int pn = addPanel ("MOD ENV", 4);
        knob (pn, "me_a", "A"); knob (pn, "me_d", "D"); knob (pn, "me_s", "S"); knob (pn, "me_r", "R");
        choice (pn, "me1_dest", "Dest 1"); knob (pn, "me1_amt", "Amt 1");
        choice (pn, "me2_dest", "Dest 2"); knob (pn, "me2_amt", "Amt 2");
    }

    // Fila 3: LFOs (panels 6..7)
    for (auto n : { "l1", "l2" })
    {
        const juce::String s (n);
        const int pn = addPanel (s == "l1" ? "LFO 1" : "LFO 2", 7);
        choice (pn, s + "_shape", "Shape");
        knob   (pn, s + "_rate", "Rate Hz");
        toggle (pn, s + "_sync", "Sync");
        choice (pn, s + "_div", "Div");
        knob   (pn, s + "_amt", "Amount");
        choice (pn, s + "_dest", "Dest");
        toggle (pn, s + "_key", "Key Sync");
    }

    // Fila 4: FX (panels 8..13)
    {
        const int pn = addPanel ("DISTORTION", 3);
        choice (pn, "dist_type", "Type"); knob (pn, "dist_drive", "Drive"); knob (pn, "dist_mix", "Mix");
    }
    {
        const int pn = addPanel ("OTT", 5);
        knob (pn, "ott_depth", "Depth"); knob (pn, "ott_time", "Time");
        knob (pn, "ott_low", "Low"); knob (pn, "ott_mid", "Mid"); knob (pn, "ott_high", "High");
    }
    {
        const int pn = addPanel ("CHORUS", 3);
        knob (pn, "ch_mix", "Mix"); knob (pn, "ch_rate", "Rate"); knob (pn, "ch_depth", "Depth");
    }
    {
        const int pn = addPanel ("DELAY", 4);
        choice (pn, "dl_div", "Time"); knob (pn, "dl_fb", "Fdbk"); knob (pn, "dl_mix", "Mix"); toggle (pn, "dl_pp", "PP");
    }
    {
        const int pn = addPanel ("REVERB", 3);
        knob (pn, "rv_size", "Size"); knob (pn, "rv_damp", "Damp"); knob (pn, "rv_mix", "Mix");
    }
    {
        const int pn = addPanel ("MASTER", 1);
        knob (pn, "out_gain", "Gain dB");
    }

    // Barra de presets
    const auto& presets = Gw::getFactoryPresets();
    for (size_t i = 0; i < presets.size(); ++i)
        presetBox.addItem (presets[i].name, (int) i + 1);
    presetBox.setSelectedItemIndex (proc.getCurrentProgram(), juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedItemIndex()); };
    addAndMakeVisible (presetBox);

    prevBtn.onClick = [this]
    {
        presetBox.setSelectedItemIndex (juce::jmax (0, presetBox.getSelectedItemIndex() - 1), juce::sendNotification);
    };
    nextBtn.onClick = [this]
    {
        presetBox.setSelectedItemIndex (juce::jmin (presetBox.getNumItems() - 1, presetBox.getSelectedItemIndex() + 1),
                                        juce::sendNotification);
    };
    addAndMakeVisible (prevBtn);
    addAndMakeVisible (nextBtn);

    setSize (1180, 720);
}

GritwaveEditor::~GritwaveEditor()
{
    setLookAndFeel (nullptr);
}

void GritwaveEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (bgDark, 0.f, 0.f, juce::Colour (0xff16101f), 0.f, (float) getHeight(), false));
    g.fillAll();

    g.setColour (accentG);
    g.setFont (juce::Font (juce::FontOptions (26.f, juce::Font::bold)));
    g.drawText ("GRIT", 16, 8, 80, 34, juce::Justification::centredLeft);
    g.setColour (accentM);
    g.drawText ("WAVE", 70, 8, 90, 34, juce::Justification::centredLeft);

    for (const auto& p : panels)
    {
        g.setColour (panelCol);
        g.fillRoundedRectangle (p.area.toFloat(), 8.f);
        g.setColour (juce::Colour (0xff2c2c3a));
        g.drawRoundedRectangle (p.area.toFloat().reduced (0.5f), 8.f, 1.f);
        g.setColour (accentM.withAlpha (0.9f));
        g.setFont (juce::Font (juce::FontOptions (11.f, juce::Font::bold)));
        g.drawText (p.title, p.area.getX() + 10, p.area.getY() + 3, p.area.getWidth() - 20, 14,
                    juce::Justification::centredLeft);
    }
}

void GritwaveEditor::layoutPanel (Panel& p)
{
    auto inner = p.area.withTrimmedTop (18).reduced (4);
    const int count = (int) p.comps.size();
    const int cols = std::max (1, p.cols);
    const int rows = std::max (1, (count + cols - 1) / cols);
    const int cw = inner.getWidth() / cols;
    const int ch = inner.getHeight() / rows;

    for (int i = 0; i < count; ++i)
        p.comps[(size_t) i]->setBounds (inner.getX() + (i % cols) * cw, inner.getY() + (i / cols) * ch, cw, ch);
}

void GritwaveEditor::resized()
{
    auto b = getLocalBounds().reduced (8);
    auto top = b.removeFromTop (44);

    nextBtn.setBounds (top.removeFromRight (34).reduced (2, 8));
    presetBox.setBounds (top.removeFromRight (380).reduced (2, 8));
    prevBtn.setBounds (top.removeFromRight (34).reduced (2, 8));

    const std::vector<std::vector<int>> rows { { 0, 1, 2 }, { 3, 4, 5 }, { 6, 7 }, { 8, 9, 10, 11, 12, 13 } };
    const float weights[] = { 2.f, 2.f, 1.1f, 1.1f };
    float totalW = 0.f;
    for (float w : weights) totalW += w;

    const int totalH = b.getHeight();
    for (size_t r = 0; r < rows.size(); ++r)
    {
        const int h = (r + 1 == rows.size()) ? b.getHeight()
                                             : juce::roundToInt ((float) totalH * weights[r] / totalW);
        auto rowArea = b.removeFromTop (juce::jmin (h, b.getHeight()));

        int totalCols = 0;
        for (int idx : rows[r]) totalCols += panels[(size_t) idx].cols;

        const int rowW = rowArea.getWidth();
        for (int idx : rows[r])
        {
            auto& p = panels[(size_t) idx];
            auto piece = rowArea.removeFromLeft (rowW * p.cols / totalCols);
            p.area = piece.reduced (3);
            layoutPanel (p);
        }
    }
    repaint();
}
