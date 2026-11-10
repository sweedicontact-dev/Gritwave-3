#pragma once
#include <JuceHeader.h>

namespace Gw
{
inline const juce::StringArray divisionNames { "1/1", "1/2", "1/4", "1/8", "1/16", "1/32",
                                               "1/2T", "1/4T", "1/8T", "1/16T", "1/8D", "1/16D" };
inline const juce::StringArray destNames { "Off", "Cutoff", "WT A", "WT B", "Warp",
                                           "Pitch", "Drive", "Volume", "Reso" };
inline const juce::StringArray lfoShapes { "Sine", "Triangle", "Saw Up", "Saw Down", "Square", "S&H" };
inline const juce::StringArray warpModes { "Off", "Bend", "Sync", "FM" };
inline const juce::StringArray filterTypes { "LP24", "LP12", "BP", "HP", "Notch", "Vowel" };
inline const juce::StringArray distTypes { "Soft", "Hard", "Fold", "Crush", "Tube" };

enum Dest { DestOff = 0, DestCutoff, DestWtA, DestWtB, DestWarp, DestPitch, DestDrive, DestVolume, DestReso };

inline float divisionBeats (int idx)
{
    static const float b[] = { 4.f, 2.f, 1.f, 0.5f, 0.25f, 0.125f,
                               4.f / 3.f, 2.f / 3.f, 1.f / 3.f, 1.f / 6.f, 0.75f, 0.375f };
    return b[juce::jlimit (0, 11, idx)];
}

struct OscParams
{
    float wave = 0.f;  int warpMode = 0;  float warp = 0.f;
    int oct = 0, semi = 0;  float fine = 0.f;
    int uni = 1;  float detune = 0.f, spread = 0.f, level = 0.f;
};

struct LfoParams
{
    int shape = 0;  float rate = 1.f;  bool sync = true;  int div = 3;
    float amt = 0.f;  int dest = 0;  bool keySync = true;
};

struct VoiceParams
{
    OscParams a, b;
    float subLevel = 0.f, noiseLevel = 0.f;
    bool mono = false;  float glide = 0.f;
    int fType = 0;  float cutoff = 8000.f, res = 0.2f, drive = 0.f, keytrack = 0.f;
    float ampA = 0.001f, ampD = 0.3f, ampS = 0.8f, ampR = 0.15f;
    float meA = 0.001f, meD = 0.3f, meS = 0.f, meR = 0.2f;
    int meDest[2] { 0, 0 };  float meAmt[2] { 0.f, 0.f };
    LfoParams lfo[2];
    float bpm = 120.f;
};

struct FxParams
{
    int distType = 0;  float distDrive = 0.f, distMix = 0.f;
    float ottDepth = 0.f, ottTime = 0.5f, ottLow = 0.f, ottMid = 0.f, ottHigh = 0.f;
    float chMix = 0.f, chRate = 0.8f, chDepth = 0.4f;
    int dlDiv = 3;  float dlFb = 0.35f, dlMix = 0.f;  bool dlPP = true;
    float rvSize = 0.5f, rvDamp = 0.5f, rvMix = 0.f;
    float outGainDb = -3.f;
};

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    auto F = [&] (const juce::String& id, const juce::String& name, float lo, float hi, float def, float skew = 1.f)
    {
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name,
                                                                   juce::NormalisableRange<float> (lo, hi, 0.f, skew), def));
    };
    auto I = [&] (const juce::String& id, const juce::String& name, int lo, int hi, int def)
    {
        p.push_back (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { id, 1 }, name, lo, hi, def));
    };
    auto B = [&] (const juce::String& id, const juce::String& name, bool def)
    {
        p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def));
    };
    auto C = [&] (const juce::String& id, const juce::String& name, const juce::StringArray& items, int def)
    {
        p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name, items, def));
    };

    // ---- Osciladores A y B ----
    for (auto pre : { "a", "b" })
    {
        const juce::String s (pre);
        const bool isA = (s == "a");
        F (s + "_wave",   "Wave",   0.f, 1.f, 0.2857f);
        C (s + "_wmode",  "Warp Mode", warpModes, 0);
        F (s + "_warp",   "Warp",   0.f, 1.f, 0.f);
        I (s + "_oct",    "Oct",   -3, 3, 0);
        I (s + "_semi",   "Semi", -12, 12, 0);
        F (s + "_fine",   "Fine", -100.f, 100.f, 0.f);
        I (s + "_uni",    "Unison", 1, 7, 1);
        F (s + "_detune", "Detune", 0.f, 1.f, 0.3f);
        F (s + "_spread", "Spread", 0.f, 1.f, 0.5f);
        F (s + "_level",  "Level",  0.f, 1.f, isA ? 0.8f : 0.f);
    }

    F ("sub_level",   "Sub",   0.f, 1.f, 0.f);
    F ("noise_level", "Noise", 0.f, 1.f, 0.f);
    B ("mono",        "Mono",  false);
    F ("glide",       "Glide", 0.f, 1.f, 0.f);

    // ---- Filtro ----
    C ("f_type",   "Filter", filterTypes, 0);
    F ("f_cutoff", "Cutoff", 20.f, 20000.f, 8000.f, 0.25f);
    F ("f_res",    "Reso",   0.f, 1.f, 0.2f);
    F ("f_drive",  "Drive",  0.f, 1.f, 0.f);
    F ("f_key",    "KeyTrk", 0.f, 1.f, 0.f);

    // ---- Envolventes ----
    F ("amp_a", "A", 0.001f, 5.f, 0.001f, 0.3f);
    F ("amp_d", "D", 0.005f, 5.f, 0.3f, 0.3f);
    F ("amp_s", "S", 0.f, 1.f, 0.8f);
    F ("amp_r", "R", 0.005f, 5.f, 0.15f, 0.3f);
    F ("me_a", "A", 0.001f, 5.f, 0.001f, 0.3f);
    F ("me_d", "D", 0.005f, 5.f, 0.3f, 0.3f);
    F ("me_s", "S", 0.f, 1.f, 0.f);
    F ("me_r", "R", 0.005f, 5.f, 0.2f, 0.3f);
    C ("me1_dest", "Env>1", destNames, 0);
    F ("me1_amt",  "Amt 1", -1.f, 1.f, 0.f);
    C ("me2_dest", "Env>2", destNames, 0);
    F ("me2_amt",  "Amt 2", -1.f, 1.f, 0.f);

    // ---- LFOs ----
    for (auto n : { "l1", "l2" })
    {
        const juce::String s (n);
        C (s + "_shape", "Shape", lfoShapes, 0);
        F (s + "_rate",  "Rate Hz", 0.05f, 30.f, 2.f, 0.4f);
        B (s + "_sync",  "Sync", true);
        C (s + "_div",   "Div", divisionNames, 3);
        F (s + "_amt",   "Amount", -1.f, 1.f, 0.f);
        C (s + "_dest",  "Dest", destNames, 0);
        B (s + "_key",   "Key", true);
    }

    // ---- FX ----
    C ("dist_type",  "Type",  distTypes, 0);
    F ("dist_drive", "Drive", 0.f, 1.f, 0.f);
    F ("dist_mix",   "Mix",   0.f, 1.f, 0.f);
    F ("ott_depth",  "Depth", 0.f, 1.f, 0.f);
    F ("ott_time",   "Time",  0.f, 1.f, 0.5f);
    F ("ott_low",    "Low",  -12.f, 12.f, 0.f);
    F ("ott_mid",    "Mid",  -12.f, 12.f, 0.f);
    F ("ott_high",   "High", -12.f, 12.f, 0.f);
    F ("ch_mix",     "Mix",   0.f, 1.f, 0.f);
    F ("ch_rate",    "Rate",  0.1f, 8.f, 0.8f, 0.5f);
    F ("ch_depth",   "Depth", 0.f, 1.f, 0.4f);
    C ("dl_div",     "Time",  divisionNames, 3);
    F ("dl_fb",      "Fdbk",  0.f, 0.95f, 0.35f);
    F ("dl_mix",     "Mix",   0.f, 1.f, 0.f);
    B ("dl_pp",      "PingPong", true);
    F ("rv_size",    "Size",  0.f, 1.f, 0.5f);
    F ("rv_damp",    "Damp",  0.f, 1.f, 0.5f);
    F ("rv_mix",     "Mix",   0.f, 1.f, 0.f);
    F ("out_gain",   "Gain dB", -24.f, 6.f, -3.f);

    return { p.begin(), p.end() };
}
} // namespace Gw
