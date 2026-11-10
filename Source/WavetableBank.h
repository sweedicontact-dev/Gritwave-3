#pragma once
#include <JuceHeader.h>
#include <cmath>
#include <vector>

namespace Gw
{
// 8 frames generados por sintesis aditiva, con 10 niveles mip (limitados en banda)
// 0 Sine | 1 Triangle | 2 Saw | 3 Square | 4 Formant Saw | 5 Vowel | 6 Pulse | 7 Digital Buzz
class WavetableBank
{
public:
    static constexpr int tableSize = 2048;
    static constexpr int numFrames = 8;
    static constexpr int numMips   = 10;

    static const WavetableBank& get()
    {
        static WavetableBank bank;
        return bank;
    }

    static int mipForFrequency (float f) noexcept
    {
        int m = 0;
        float th = 40.f;
        while (f > th && m < numMips - 1) { th *= 2.f; ++m; }
        return m;
    }

    inline float read (int mip, float framePos, float phase) const noexcept
    {
        const float fp = juce::jlimit (0.f, 1.f, framePos) * (float) (numFrames - 1);
        int f0 = (int) fp;
        if (f0 >= numFrames - 1) f0 = numFrames - 2;
        const float ff = fp - (float) f0;

        const float idx = phase * (float) tableSize;
        int i0 = juce::jlimit (0, tableSize - 1, (int) idx);
        const float fr = idx - (float) i0;

        const float* a = table (mip, f0);
        const float* b = table (mip, f0 + 1);
        const float sa = a[i0] + fr * (a[i0 + 1] - a[i0]);
        const float sb = b[i0] + fr * (b[i0 + 1] - b[i0]);
        return sa + ff * (sb - sa);
    }

private:
    WavetableBank() { build(); }

    std::vector<float> data;

    const float* table (int mip, int frame) const noexcept
    {
        return data.data() + ((size_t) mip * numFrames + (size_t) frame) * (tableSize + 2);
    }
    float* tableW (int mip, int frame) noexcept
    {
        return data.data() + ((size_t) mip * numFrames + (size_t) frame) * (tableSize + 2);
    }

    static float gauss (float x, float c, float w)
    {
        const float d = (x - c) / w;
        return std::exp (-d * d);
    }

    void build()
    {
        data.assign ((size_t) numMips * numFrames * (tableSize + 2), 0.f);

        std::vector<float> sinLut ((size_t) tableSize);
        for (int i = 0; i < tableSize; ++i)
            sinLut[(size_t) i] = std::sin (juce::MathConstants<float>::twoPi * (float) i / (float) tableSize);

        const float pi = juce::MathConstants<float>::pi;
        const int mask = tableSize - 1;

        for (int frame = 0; frame < numFrames; ++frame)
        {
            for (int mip = 0; mip < numMips; ++mip)
            {
                const int maxH = std::max (1, 512 >> mip);
                float* t = tableW (mip, frame);

                for (int n = 1; n <= maxH; ++n)
                {
                    float a = 0.f;
                    int off = 0;
                    const float fn = (float) n;

                    switch (frame)
                    {
                        case 0: a = (n == 1) ? 1.f : 0.f; break;
                        case 1: a = (n & 1) ? ((((n - 1) / 2) & 1) ? -1.f : 1.f) / (fn * fn) : 0.f; break;
                        case 2: a = 1.f / fn; break;
                        case 3: a = (n & 1) ? 1.f / fn : 0.f; break;
                        case 4: a = (1.f / fn) * (1.f + 5.f * gauss (fn, 6.f, 1.8f)); break;
                        case 5: a = (1.f / fn) * (0.25f + 4.f * gauss (fn, 3.f, 1.f) + 3.f * gauss (fn, 9.f, 1.8f)); break;
                        case 6: a = (2.f / (fn * pi)) * std::sin (fn * pi * 0.12f); off = tableSize / 4; break;
                        case 7: a = (1.f / std::sqrt (fn)) / (1.f + fn / 64.f); off = (n * n * 37) & mask; break;
                        default: break;
                    }
                    if (a == 0.f) continue;

                    for (int i = 0; i < tableSize; ++i)
                        t[i] += a * sinLut[(size_t) ((n * i + off) & mask)];
                }
            }

            // Normalizacion segun el pico del mip 0 (mismo nivel en todos los mips)
            float peak = 1e-6f;
            const float* t0 = table (0, frame);
            for (int i = 0; i < tableSize; ++i) peak = std::max (peak, std::abs (t0[i]));
            const float gain = 0.9f / peak;

            for (int mip = 0; mip < numMips; ++mip)
            {
                float* t = tableW (mip, frame);
                for (int i = 0; i < tableSize; ++i) t[i] *= gain;
                t[tableSize]     = t[0];
                t[tableSize + 1] = t[1];
            }
        }
    }
};
} // namespace Gw
