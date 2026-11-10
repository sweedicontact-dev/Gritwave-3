#include "PluginProcessor.h"
#include "PluginEditor.h"

GritwaveProcessor::GritwaveProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", Gw::createLayout())
{
}

bool GritwaveProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void GritwaveProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    for (auto& v : voices) v.prepare (sampleRate);
    fx.prepare (sampleRate, samplesPerBlock);
    setLatencySamples (juce::roundToInt (fx.getLatency()));
    synthBuf.setSize (2, samplesPerBlock);
    monoStack.clear();
}

//==============================================================================
void GritwaveProcessor::readParams()
{
    auto f = [this] (const juce::String& id) { return apvts.getRawParameterValue (id)->load(); };
    auto i = [&] (const juce::String& id) { return juce::roundToInt (f (id)); };

    auto readOsc = [&] (const juce::String& s, Gw::OscParams& o)
    {
        o.wave = f (s + "_wave");     o.warpMode = i (s + "_wmode"); o.warp = f (s + "_warp");
        o.oct = i (s + "_oct");       o.semi = i (s + "_semi");      o.fine = f (s + "_fine");
        o.uni = i (s + "_uni");       o.detune = f (s + "_detune");  o.spread = f (s + "_spread");
        o.level = f (s + "_level");
    };
    readOsc ("a", vp.a);
    readOsc ("b", vp.b);

    vp.subLevel = f ("sub_level");  vp.noiseLevel = f ("noise_level");
    vp.mono = f ("mono") > 0.5f;    vp.glide = f ("glide");

    vp.fType = i ("f_type");  vp.cutoff = f ("f_cutoff");  vp.res = f ("f_res");
    vp.drive = f ("f_drive"); vp.keytrack = f ("f_key");

    vp.ampA = f ("amp_a"); vp.ampD = f ("amp_d"); vp.ampS = f ("amp_s"); vp.ampR = f ("amp_r");
    vp.meA = f ("me_a");   vp.meD = f ("me_d");   vp.meS = f ("me_s");   vp.meR = f ("me_r");
    vp.meDest[0] = i ("me1_dest"); vp.meAmt[0] = f ("me1_amt");
    vp.meDest[1] = i ("me2_dest"); vp.meAmt[1] = f ("me2_amt");

    for (int l = 0; l < 2; ++l)
    {
        const juce::String s = "l" + juce::String (l + 1);
        auto& lp = vp.lfo[l];
        lp.shape = i (s + "_shape"); lp.rate = f (s + "_rate"); lp.sync = f (s + "_sync") > 0.5f;
        lp.div = i (s + "_div");     lp.amt = f (s + "_amt");   lp.dest = i (s + "_dest");
        lp.keySync = f (s + "_key") > 0.5f;
    }

    fxp.distType = i ("dist_type"); fxp.distDrive = f ("dist_drive"); fxp.distMix = f ("dist_mix");
    fxp.ottDepth = f ("ott_depth"); fxp.ottTime = f ("ott_time");
    fxp.ottLow = f ("ott_low"); fxp.ottMid = f ("ott_mid"); fxp.ottHigh = f ("ott_high");
    fxp.chMix = f ("ch_mix"); fxp.chRate = f ("ch_rate"); fxp.chDepth = f ("ch_depth");
    fxp.dlDiv = i ("dl_div"); fxp.dlFb = f ("dl_fb"); fxp.dlMix = f ("dl_mix"); fxp.dlPP = f ("dl_pp") > 0.5f;
    fxp.rvSize = f ("rv_size"); fxp.rvDamp = f ("rv_damp"); fxp.rvMix = f ("rv_mix");
    fxp.outGainDb = f ("out_gain");
}

//==============================================================================
void GritwaveProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();

    readParams();

    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm())
                bpm = *b;
    bpm = juce::jlimit (20.0, 300.0, bpm);
    vp.bpm = (float) bpm;

    synthBuf.setSize (2, n, false, false, true);
    synthBuf.clear();
    float* L = synthBuf.getWritePointer (0);
    float* R = synthBuf.getWritePointer (1);

    int pos = 0;
    for (const auto meta : midi)
    {
        const int ev = juce::jlimit (pos, n, meta.samplePosition);
        renderVoices (L + pos, R + pos, ev - pos);
        handleMidi (meta.getMessage());
        pos = ev;
    }
    renderVoices (L + pos, R + pos, n - pos);

    fx.process (synthBuf, fxp, bpm);

    for (int ch = 0; ch < juce::jmin (2, buffer.getNumChannels()); ++ch)
        buffer.copyFrom (ch, 0, synthBuf, ch, 0, n);

    midi.clear();
}

void GritwaveProcessor::renderVoices (float* l, float* r, int count)
{
    if (count <= 0) return;
    for (auto& v : voices) v.render (l, r, count, vp, pitchBend);
}

//==============================================================================
void GritwaveProcessor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn())                              noteOn (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())                        noteOff (m.getNoteNumber());
    else if (m.isAllNotesOff() || m.isAllSoundOff()) allNotesOff();
    else if (m.isPitchWheel())                     pitchBend = (float) (m.getPitchWheelValue() - 8192) / 8192.f * 2.f;
}

void GritwaveProcessor::noteOn (int note, float vel)
{
    if (vp.mono)
    {
        monoStack.erase (std::remove_if (monoStack.begin(), monoStack.end(),
                                         [note] (const auto& e) { return e.first == note; }),
                         monoStack.end());
        monoStack.push_back ({ note, vel });

        for (int i = 1; i < numVoices; ++i) voices[i].noteOff();
        const bool legato = voices[0].isActive() && voices[0].isGateOn();
        voices[0].noteOn (note, vel, legato, vp, ++voiceCounter);
        return;
    }

    monoStack.clear();
    Gw::Voice* target = nullptr;

    for (auto& v : voices)
        if (v.isActive() && v.isGateOn() && v.getNote() == note) { target = &v; break; }

    if (target == nullptr)
        for (auto& v : voices)
            if (! v.isActive()) { target = &v; break; }

    if (target == nullptr)
    {
        // robar: primero la voz mas vieja en release, luego la mas vieja en general
        uint32_t best = std::numeric_limits<uint32_t>::max();
        for (auto& v : voices)
            if (! v.isGateOn() && v.getAge() < best) { best = v.getAge(); target = &v; }
        if (target == nullptr)
        {
            best = std::numeric_limits<uint32_t>::max();
            for (auto& v : voices)
                if (v.getAge() < best) { best = v.getAge(); target = &v; }
        }
    }
    target->noteOn (note, vel, false, vp, ++voiceCounter);
}

void GritwaveProcessor::noteOff (int note)
{
    if (vp.mono)
    {
        const bool wasTop = ! monoStack.empty() && monoStack.back().first == note;
        monoStack.erase (std::remove_if (monoStack.begin(), monoStack.end(),
                                         [note] (const auto& e) { return e.first == note; }),
                         monoStack.end());
        if (monoStack.empty())  voices[0].noteOff();
        else if (wasTop)        voices[0].retarget (monoStack.back().first);
        return;
    }

    for (auto& v : voices)
        if (v.isGateOn() && v.getNote() == note) v.noteOff();
}

void GritwaveProcessor::allNotesOff()
{
    monoStack.clear();
    for (auto& v : voices) v.noteOff();
}

//==============================================================================
void GritwaveProcessor::setCurrentProgram (int index)
{
    const auto& presets = Gw::getFactoryPresets();
    if (index < 0 || index >= (int) presets.size()) return;
    currentProgram = index;

    for (auto* p : getParameters())
        p->setValueNotifyingHost (p->getDefaultValue());

    for (const auto& kv : presets[(size_t) index].values)
        if (auto* p = apvts.getParameter (kv.first))
            p->setValueNotifyingHost (p->convertTo0to1 (kv.second));
}

const juce::String GritwaveProcessor::getProgramName (int index)
{
    const auto& presets = Gw::getFactoryPresets();
    if (index < 0 || index >= (int) presets.size()) return {};
    return presets[(size_t) index].name;
}

void GritwaveProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void GritwaveProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto vt = juce::ValueTree::fromXml (*xml);
            currentProgram = juce::jmax (0, (int) vt.getProperty ("program", 0));
            apvts.replaceState (vt);
        }
}

juce::AudioProcessorEditor* GritwaveProcessor::createEditor() { return new GritwaveEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new GritwaveProcessor(); }
