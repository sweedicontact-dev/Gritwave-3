#pragma once
#include <JuceHeader.h>
#include "Parameters.h"
#include "WavetableBank.h"

namespace Gw
{
inline float fastTanh (float x) noexcept
{
    if (x > 3.f)  return 1.f;
    if (x < -3.f) return -1.f;
    const float x2 = x * x;
    return x * (27.f + x2) / (27.f + 9.f * x2);
}

struct SvfState { float s1 = 0.f, s2 = 0.f; };

struct SvfCoef
{
    float k = 1.f, a1 = 0.f, a2 = 0.f, a3 = 0.f;
    void set (float g, float kk) noexcept
    {
        k = kk;
        a1 = 1.f / (1.f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
};

// SVF TPT (Zavalishin)
inline void svfTick (SvfState& s, const SvfCoef& c, float x, float& lp, float& bp, float& hp) noexcept
{
    const float v3 = x - s.s2;
    const float v1 = c.a1 * s.s1 + c.a2 * v3;
    const float v2 = s.s2 + c.a2 * s.s1 + c.a3 * v3;
    s.s1 = 2.f * v1 - s.s1;
    s.s2 = 2.f * v2 - s.s2;
    lp = v2;
    bp = v1;
    hp = x - c.k * v1 - v2;
}

class Voice
{
public:
    void prepare (double sr)
    {
        sampleRate = sr;
        ampEnv.setSampleRate (sr);
        modEnv.setSampleRate (sr);
        reset();
    }

    void reset()
    {
        active = false;
        gate = false;
        ampEnv.reset();
        modEnv.reset();
        clearFilters();
    }

    bool isActive() const noexcept   { return active; }
    bool isGateOn() const noexcept   { return gate; }
    int getNote() const noexcept     { return note; }
    uint32_t getAge() const noexcept { return age; }

    void noteOn (int n, float vel, bool legato, const VoiceParams& p, uint32_t newAge)
    {
        note = n;
        velocity = vel;
        targetNote = (float) n;
        age = newAge;
        gate = true;

        if (! legato || ! active)
        {
            curNote = (float) n;
            applyEnvParams (p);
            ampEnv.noteOn();
            modEnv.noteOn();

            for (int k = 0; k < 7; ++k)
            {
                phaseA[k] = (p.a.uni > 1) ? rand01() : 0.f;
                phaseB[k] = (p.b.uni > 1) ? rand01() : 0.f;
            }
            subPhase = 0.f;

            for (int l = 0; l < 2; ++l)
                if (p.lfo[l].keySync) lfoPhase[l] = 0.f;

            if (! active) { clearFilters(); cutSm = p.cutoff; }
            active = true;
        }
    }

    void retarget (int n) noexcept { note = n; targetNote = (float) n; }

    void noteOff()
    {
        if (! active) return;
        gate = false;
        ampEnv.noteOff();
        modEnv.noteOff();
    }

    void render (float* outL, float* outR, int numSamples, const VoiceParams& p, float pitchBend)
    {
        if (! active) return;

        const auto& bank = WavetableBank::get();
        applyEnvParams (p);

        const float maxRatioA = prepareUnison (p.a, ratioA, panLA, panRA);
        const float maxRatioB = prepareUnison (p.b, ratioB, panLB, panRB);

        const float sr = (float) sampleRate;
        const float invSr = 1.f / sr;
        const float twoPi = juce::MathConstants<float>::twoPi;
        const float glideCoef = (p.mono && p.glide > 0.0005f)
                                    ? 1.f - std::exp (-1.f / (p.glide * 0.2f * sr))
                                    : 1.f;

        for (int i = 0; i < numSamples; ++i)
        {
            curNote += (targetNote - curNote) * glideCoef;

            const float ampE = ampEnv.getNextSample();
            const float modE = modEnv.getNextSample();
            if (! ampEnv.isActive()) { active = false; break; }

            // --- LFOs ---
            float lfoV[2];
            for (int l = 0; l < 2; ++l)
            {
                const auto& lp = p.lfo[l];
                const float hz = lp.sync ? (p.bpm / 60.f) / divisionBeats (lp.div) : lp.rate;
                lfoPhase[l] += hz * invSr;
                if (lfoPhase[l] >= 1.f)
                {
                    lfoPhase[l] -= 1.f;
                    shVal[l] = rand01() * 2.f - 1.f;
                }
                lfoV[l] = lfoShape (lp.shape, lfoPhase[l], shVal[l]);
            }

            Mods m;
            for (int l = 0; l < 2; ++l) addMod (m, p.lfo[l].dest, lfoV[l], p.lfo[l].amt, false);
            for (int e = 0; e < 2; ++e) addMod (m, p.meDest[e], modE, p.meAmt[e], true);

            // --- Pitch ---
            const float base = curNote + pitchBend + m.pitch;
            const float fA = midiToHz (base + (float) (p.a.oct * 12 + p.a.semi) + p.a.fine * 0.01f);
            const float fB = midiToHz (base + (float) (p.b.oct * 12 + p.b.semi) + p.b.fine * 0.01f);
            const float fSub = midiToHz (base - 12.f);

            // --- Control rate (cada 4 muestras) ---
            if ((ctrl++ & 3) == 0)
            {
                mipA = WavetableBank::mipForFrequency (fA * maxRatioA);
                mipB = WavetableBank::mipForFrequency (fB * maxRatioB);
                updateFilter (p, m, sr);
            }

            const float warpA = juce::jlimit (0.f, 1.f, p.a.warp + m.warp);
            const float warpB = juce::jlimit (0.f, 1.f, p.b.warp + m.warp);
            const float waveA = juce::jlimit (0.f, 1.f, p.a.wave + m.wtA);
            const float waveB = juce::jlimit (0.f, 1.f, p.b.wave + m.wtB);

            float aL = 0.f, aR = 0.f, bL = 0.f, bR = 0.f;
            if (p.a.level > 0.0005f)
                renderOsc (bank, p.a, waveA, warpA, mipA, fA, phaseA, ratioA, panLA, panRA, invSr, aL, aR);
            if (p.b.level > 0.0005f)
                renderOsc (bank, p.b, waveB, warpB, mipB, fB, phaseB, ratioB, panLB, panRB, invSr, bL, bR);

            float sL = aL * p.a.level + bL * p.b.level;
            float sR = aR * p.a.level + bR * p.b.level;

            if (p.subLevel > 0.0005f)
            {
                subPhase += fSub * invSr;
                if (subPhase >= 1.f) subPhase -= 1.f;
                const float sub = std::sin (twoPi * subPhase) * p.subLevel;
                sL += sub;
                sR += sub;
            }
            if (p.noiseLevel > 0.0005f)
            {
                sL += (rand01() * 2.f - 1.f) * p.noiseLevel * 0.5f;
                sR += (rand01() * 2.f - 1.f) * p.noiseLevel * 0.5f;
            }

            const float yL = filterSample (0, sL);
            const float yR = filterSample (1, sR);

            const float g = ampE * (0.3f + 0.7f * velocity) * m.vol * 0.5f;
            outL[i] += yL * g;
            outR[i] += yR * g;
        }
    }

private:
    struct Mods
    {
        float cutOct = 0.f, wtA = 0.f, wtB = 0.f, warp = 0.f, pitch = 0.f, drive = 0.f, vol = 1.f, reso = 0.f;
    };

    static void addMod (Mods& m, int dest, float v, float amt, bool unipolar) noexcept
    {
        if (dest == DestOff || amt == 0.f) return;
        const float x = v * amt;
        switch (dest)
        {
            case DestCutoff: m.cutOct += x * 5.f; break;
            case DestWtA:    m.wtA += x * 0.5f; break;
            case DestWtB:    m.wtB += x * 0.5f; break;
            case DestWarp:   m.warp += x * 0.5f; break;
            case DestPitch:  m.pitch += x * 12.f; break;
            case DestDrive:  m.drive += x * 0.5f; break;
            case DestReso:   m.reso += x * 0.5f; break;
            case DestVolume:
            {
                float u = unipolar ? v : 0.5f * v + 0.5f;
                if (amt < 0.f) u = 1.f - u;
                const float a = std::abs (amt);
                m.vol *= (1.f - a) + a * u;
                break;
            }
            default: break;
        }
    }

    static float lfoShape (int s, float p, float sh) noexcept
    {
        switch (s)
        {
            case 0: return -std::cos (juce::MathConstants<float>::twoPi * p);
            case 1: return 1.f - 4.f * std::abs (p - 0.5f);
            case 2: return 2.f * p - 1.f;
            case 3: return 1.f - 2.f * p;
            case 4: return p < 0.5f ? 1.f : -1.f;
            default: return sh;
        }
    }

    static float midiToHz (float n) noexcept { return 440.f * std::exp2 ((n - 69.f) * (1.f / 12.f)); }

    float rand01() noexcept
    {
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        return (float) (rng & 0xFFFFFF) / 16777216.f;
    }

    void applyEnvParams (const VoiceParams& p)
    {
        ampEnv.setParameters ({ p.ampA, p.ampD, p.ampS, p.ampR });
        modEnv.setParameters ({ p.meA, p.meD, p.meS, p.meR });
    }

    void clearFilters()
    {
        for (int c = 0; c < 2; ++c)
        {
            f1[c] = {}; f2[c] = {};
            for (int j = 0; j < 3; ++j) fs[c][j] = {};
        }
    }

    static float prepareUnison (const OscParams& o, float* ratio, float* pl, float* pr) noexcept
    {
        const int n = juce::jlimit (1, 7, o.uni);
        const float norm = 1.f / std::sqrt ((float) n);
        float maxR = 1.f;
        for (int k = 0; k < n; ++k)
        {
            const float off = n > 1 ? ((float) k / (float) (n - 1)) * 2.f - 1.f : 0.f;
            const float cents = off * o.detune * 60.f;
            ratio[k] = std::exp2 (cents / 1200.f);
            maxR = std::max (maxR, ratio[k]);
            const float pan = off * o.spread;
            const float ang = (pan + 1.f) * 0.25f * juce::MathConstants<float>::pi;
            pl[k] = std::cos (ang) * norm * 1.414f;
            pr[k] = std::sin (ang) * norm * 1.414f;
        }
        return maxR;
    }

    static void renderOsc (const WavetableBank& bank, const OscParams& o, float wave, float warp, int mip,
                           float freq, float* ph, const float* ratio, const float* pl, const float* pr,
                           float invSr, float& outL, float& outR) noexcept
    {
        const int n = juce::jlimit (1, 7, o.uni);
        const float twoPi = juce::MathConstants<float>::twoPi;
        for (int k = 0; k < n; ++k)
        {
            float p = ph[k] + freq * ratio[k] * invSr;
            if (p >= 1.f) p -= 1.f;
            ph[k] = p;

            float x = p;
            switch (o.warpMode)
            {
                case 1: x = p + warp * (p * p * p - p); break;
                case 2: { const float q = p * (1.f + warp * 7.f); x = q - std::floor (q); break; }
                case 3: { x = p + warp * 0.3f * std::sin (twoPi * p * 2.f); x -= std::floor (x); break; }
                default: break;
            }
            const float s = bank.read (mip, wave, x);
            outL += s * pl[k];
            outR += s * pr[k];
        }
    }

    void updateFilter (const VoiceParams& p, const Mods& m, float sr)
    {
        cutSm += (p.cutoff - cutSm) * 0.05f;
        float cut = cutSm * std::exp2 (m.cutOct + p.keytrack * (curNote - 60.f) / 12.f);
        cut = juce::jlimit (20.f, std::min (20000.f, 0.45f * sr), cut);
        const float reso = juce::jlimit (0.f, 1.f, p.res + m.reso);
        drvAmt = juce::jlimit (0.f, 1.f, p.drive + m.drive);
        fType = p.fType;

        if (fType == 5)
        {
            static const float vow[5][3] = { { 800.f, 1150.f, 2900.f }, { 400.f, 1600.f, 2700.f },
                                             { 350.f, 1700.f, 2700.f }, { 450.f, 800.f, 2830.f },
                                             { 325.f, 700.f, 2530.f } };
            const float pos = juce::jlimit (0.f, 1.f, std::log (cut / 20.f) / 6.9078f) * 4.f;
            const int i0 = juce::jmin (3, (int) pos);
            const float fr = pos - (float) i0;
            const float kForm = 1.f / (2.f + reso * 14.f);
            static const float w[3] = { 1.f, 0.7f, 0.4f };
            for (int j = 0; j < 3; ++j)
            {
                float f = vow[i0][j] * std::pow (vow[i0 + 1][j] / vow[i0][j], fr);
                f = std::min (f, 0.45f * sr);
                fc[j].set (std::tan (juce::MathConstants<float>::pi * f / sr), kForm);
                fw[j] = w[j];
            }
        }
        else
        {
            const float g = std::tan (juce::MathConstants<float>::pi * cut / sr);
            c1.set (g, 2.f - 1.96f * reso);
            c2.set (g, 1.2f);
        }
    }

    float filterSample (int ch, float x) noexcept
    {
        x = fastTanh (x * (1.f + drvAmt * 6.f));
        float y = 0.f;
        float lp, bp, hp;

        if (fType == 5)
        {
            for (int j = 0; j < 3; ++j)
            {
                svfTick (fs[ch][j], fc[j], x, lp, bp, hp);
                y += bp * fc[j].k * fw[j];
            }
            y *= 2.5f;
        }
        else
        {
            svfTick (f1[ch], c1, x, lp, bp, hp);
            switch (fType)
            {
                case 0: { float l2, b2, h2; svfTick (f2[ch], c2, lp, l2, b2, h2); y = l2; break; }
                case 1: y = lp; break;
                case 2: y = bp * c1.k; break;
                case 3: y = hp; break;
                default: y = lp + hp; break;
            }
        }
        return fastTanh (y * (1.f + drvAmt * 2.f));
    }

    double sampleRate = 44100.0;
    bool active = false, gate = false;
    int note = -1;
    float velocity = 0.f, curNote = 60.f, targetNote = 60.f;
    uint32_t age = 0, rng = 22222u, ctrl = 0;

    juce::ADSR ampEnv, modEnv;

    float phaseA[7] {}, phaseB[7] {}, subPhase = 0.f;
    float ratioA[7] {}, panLA[7] {}, panRA[7] {};
    float ratioB[7] {}, panLB[7] {}, panRB[7] {};
    int mipA = 0, mipB = 0;

    float lfoPhase[2] { 0.f, 0.f }, shVal[2] { 0.f, 0.f };

    int fType = 0;
    float cutSm = 8000.f, drvAmt = 0.f;
    SvfState f1[2], f2[2], fs[2][3];
    SvfCoef c1, c2, fc[3];
    float fw[3] { 1.f, 0.7f, 0.4f };
};
} // namespace Gw
