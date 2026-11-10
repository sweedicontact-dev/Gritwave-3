#pragma once
#include <JuceHeader.h>
#include "Parameters.h"
#include "Voice.h"

namespace Gw
{
//==============================================================================
class Distortion
{
public:
    void reset() { for (int c = 0; c < 2; ++c) { held[c] = 0.f; cnt[c] = 0; } }

    float process (int ch, float x, int type, float drive) noexcept
    {
        const float g = 1.f + drive * drive * 40.f + drive * 4.f;
        switch (type)
        {
            case 0: return fastTanh (x * g) * 0.9f;
            case 1: return juce::jlimit (-1.f, 1.f, x * g) * 0.7f;
            case 2: return std::sin (x * (0.8f + g * 0.5f)) * 0.9f;
            case 3:
            {
                const float levels = std::exp2 (12.f - 9.f * drive);
                const int hold = 1 + (int) (drive * drive * 12.f);
                if (cnt[ch]-- <= 0)
                {
                    held[ch] = std::round (x * levels) / levels;
                    cnt[ch] = hold - 1;
                }
                return held[ch];
            }
            default: return (fastTanh (x * g + 0.25f) - fastTanh (0.25f)) * 0.9f;
        }
    }

private:
    float held[2] { 0.f, 0.f };
    int cnt[2] { 0, 0 };
};

//==============================================================================
// Compresor multibanda 3 bandas, estilo "OTT": comprime hacia abajo y expande hacia arriba
class OttCompressor
{
public:
    void prepare (double sr)
    {
        sampleRate = sr;
        juce::dsp::ProcessSpec spec { sr, 512, 2 };
        lr1.prepare (spec); lr1.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);  lr1.setCutoffFrequency (140.f);
        lr2.prepare (spec); lr2.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);  lr2.setCutoffFrequency (2400.f);
        ap.prepare (spec);  ap.setType (juce::dsp::LinkwitzRileyFilterType::allpass);   ap.setCutoffFrequency (2400.f);
        for (auto& b : bands) b = {};
    }

    void process (float* l, float* r, int n, float depth, float time, float gl, float gm, float gh)
    {
        if (depth <= 0.0005f && gl == 0.f && gm == 0.f && gh == 0.f) return;

        const float attMs = juce::jmap (time, 0.5f, 18.f);
        const float relMs = juce::jmap (time, 25.f, 260.f);
        const float aC[3] = { coef (attMs), coef (attMs), coef (attMs * 0.5f) };
        const float rC[3] = { coef (relMs), coef (relMs * 0.7f), coef (relMs * 0.4f) };
        const float gains[3] = { juce::Decibels::decibelsToGain (gl),
                                 juce::Decibels::decibelsToGain (gm),
                                 juce::Decibels::decibelsToGain (gh) };

        for (int i = 0; i < n; ++i)
        {
            float lowL, restL, lowR, restR, midL, highL, midR, highR;
            lr1.processSample (0, l[i], lowL, restL);
            lr1.processSample (1, r[i], lowR, restR);
            lr2.processSample (0, restL, midL, highL);
            lr2.processSample (1, restR, midR, highR);
            lowL = ap.processSample (0, lowL);
            lowR = ap.processSample (1, lowR);

            const float g0 = bands[0].gain (lowL, lowR, depth, aC[0], rC[0]) * gains[0];
            const float g1 = bands[1].gain (midL, midR, depth, aC[1], rC[1]) * gains[1];
            const float g2 = bands[2].gain (highL, highR, depth, aC[2], rC[2]) * gains[2];

            l[i] = lowL * g0 + midL * g1 + highL * g2;
            r[i] = lowR * g0 + midR * g1 + highR * g2;
        }
    }

private:
    float coef (float ms) const { return 1.f - std::exp (-1.f / ((float) sampleRate * ms * 0.001f)); }

    struct Band
    {
        float env = 0.f;
        float gain (float l, float r, float depth, float aC, float rC) noexcept
        {
            const float lvl = std::max (std::abs (l), std::abs (r));
            env += (lvl - env) * (lvl > env ? aC : rC);
            const float db = 20.f * std::log10 (env + 1e-5f);
            const float hi = -22.f, lo = -42.f;
            float g = 0.f;
            if (db > hi)      g = -(db - hi) * 0.75f;
            else if (db < lo) g = (lo - db) * 0.5f;
            g = std::min (g, 20.f);
            g *= juce::jlimit (0.f, 1.f, (db + 80.f) / 10.f); // no subir el piso de ruido
            g *= depth;
            return std::exp (g * 0.115129f);
        }
    };

    double sampleRate = 44100.0;
    juce::dsp::LinkwitzRileyFilter<float> lr1, lr2, ap;
    Band bands[3];
};

//==============================================================================
class StereoDelay
{
public:
    void prepare (double sr)
    {
        sampleRate = sr;
        size = (int) (sr * 4.0) + 8;
        for (auto& b : buf) b.assign ((size_t) size, 0.f);
        writeIdx = 0;
        curDelay = 0.25f * (float) sr;
        lpL = lpR = 0.f;
    }

    void process (float* l, float* r, int n, float delaySec, float fb, float mix, bool pingPong)
    {
        if (mix <= 0.0005f) { for (auto& b : buf) std::fill (b.begin(), b.end(), 0.f); return; }

        const float target = juce::jlimit (1.f, (float) (size - 4), delaySec * (float) sampleRate);
        for (int i = 0; i < n; ++i)
        {
            curDelay += (target - curDelay) * 0.0008f;
            float rp = (float) writeIdx - curDelay;
            while (rp < 0.f) rp += (float) size;
            const int i0 = (int) rp % size;
            const float fr = rp - std::floor (rp);
            const int i1 = (i0 + 1) % size;

            const float dl = buf[0][(size_t) i0] + fr * (buf[0][(size_t) i1] - buf[0][(size_t) i0]);
            const float dr = buf[1][(size_t) i0] + fr * (buf[1][(size_t) i1] - buf[1][(size_t) i0]);
            lpL += 0.55f * (dl - lpL);
            lpR += 0.55f * (dr - lpR);

            const float inL = l[i], inR = r[i];
            if (pingPong)
            {
                buf[0][(size_t) writeIdx] = 0.5f * (inL + inR) + fb * lpR;
                buf[1][(size_t) writeIdx] = fb * lpL;
            }
            else
            {
                buf[0][(size_t) writeIdx] = inL + fb * lpL;
                buf[1][(size_t) writeIdx] = inR + fb * lpR;
            }
            if (++writeIdx >= size) writeIdx = 0;

            l[i] = inL + dl * mix;
            r[i] = inR + dr * mix;
        }
    }

private:
    double sampleRate = 44100.0;
    std::vector<float> buf[2];
    int size = 1, writeIdx = 0;
    float curDelay = 0.f, lpL = 0.f, lpR = 0.f;
};

//==============================================================================
class FxChain
{
public:
    void prepare (double sr, int maxBlock)
    {
        sampleRate = sr;
        maxBlockSize = std::max (1, maxBlock);
        juce::dsp::ProcessSpec spec { sr, (juce::uint32) maxBlockSize, 2 };

        oversampling.initProcessing ((size_t) maxBlockSize);
        oversampling.reset();
        chorus.prepare (spec);
        reverb.prepare (spec);
        limiter.prepare (spec);
        limiter.setThreshold (-1.f);
        limiter.setRelease (60.f);
        ott.prepare (sr);
        delay.prepare (sr);
        dist.reset();
        outGain.reset (sr, 0.02);
        outGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (-3.f));
    }

    float getLatency() const { return oversampling.getLatencyInSamples(); }

    void process (juce::AudioBuffer<float>& buf, const FxParams& fp, double bpm)
    {
        const int total = buf.getNumSamples();
        juce::dsp::AudioBlock<float> full (buf);

        for (int start = 0; start < total; start += maxBlockSize)
        {
            const int n = std::min (maxBlockSize, total - start);
            auto blk = full.getSubBlock ((size_t) start, (size_t) n);

            // --- Distorsion con 2x oversampling ---
            auto up = oversampling.processSamplesUp (blk);
            const size_t un = up.getNumSamples();
            float* ul = up.getChannelPointer (0);
            float* ur = up.getChannelPointer (1);
            for (size_t i = 0; i < un; ++i)
            {
                const float wl = dist.process (0, ul[i], fp.distType, fp.distDrive);
                const float wr = dist.process (1, ur[i], fp.distType, fp.distDrive);
                ul[i] = ul[i] * (1.f - fp.distMix) + wl * fp.distMix;
                ur[i] = ur[i] * (1.f - fp.distMix) + wr * fp.distMix;
            }
            oversampling.processSamplesDown (blk);

            float* L = blk.getChannelPointer (0);
            float* R = blk.getChannelPointer (1);

            // --- OTT ---
            ott.process (L, R, n, fp.ottDepth, fp.ottTime, fp.ottLow, fp.ottMid, fp.ottHigh);

            // --- Chorus ---
            chorus.setRate (fp.chRate);
            chorus.setDepth (fp.chDepth);
            chorus.setCentreDelay (7.f);
            chorus.setFeedback (0.1f);
            chorus.setMix (fp.chMix);
            chorus.process (juce::dsp::ProcessContextReplacing<float> (blk));

            // --- Delay ---
            const float delaySec = divisionBeats (fp.dlDiv) * 60.f / (float) bpm;
            delay.process (L, R, n, delaySec, fp.dlFb, fp.dlMix, fp.dlPP);

            // --- Reverb ---
            juce::dsp::Reverb::Parameters rp;
            rp.roomSize = fp.rvSize;
            rp.damping = fp.rvDamp;
            rp.wetLevel = fp.rvMix * 0.45f;
            rp.dryLevel = 1.f - fp.rvMix * 0.25f;
            rp.width = 1.f;
            rp.freezeMode = 0.f;
            reverb.setParameters (rp);
            reverb.process (juce::dsp::ProcessContextReplacing<float> (blk));

            // --- Ganancia + limiter ---
            outGain.setTargetValue (juce::Decibels::decibelsToGain (fp.outGainDb));
            for (int i = 0; i < n; ++i)
            {
                const float g = outGain.getNextValue();
                L[i] *= g;
                R[i] *= g;
            }
            limiter.process (juce::dsp::ProcessContextReplacing<float> (blk));
        }
    }

private:
    double sampleRate = 44100.0;
    int maxBlockSize = 512;

    juce::dsp::Oversampling<float> oversampling { 2, 1,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true };
    Distortion dist;
    OttCompressor ott;
    juce::dsp::Chorus<float> chorus;
    StereoDelay delay;
    juce::dsp::Reverb reverb;
    juce::dsp::Limiter<float> limiter;
    juce::SmoothedValue<float> outGain;
};
} // namespace Gw
