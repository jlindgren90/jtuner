/*
 * JTuner - tone.c
 * Copyright 2013-2018 John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#include "jtuner.h"

#include <math.h>

#define N_PEAKS 32
#define SQRT_2 1.41421356f

typedef struct {
    float freq_hz;
    float level;
} Peak;

static void find_peaks (const float freqs[N_FREQS], Peak peaks[N_PEAKS])
{
    bool skip[N_FREQS];
    int ipeaks[N_PEAKS];

    for (int i = 0; i < N_FREQS; i ++)
        skip[i] = false;

    for (int p = 0; p < N_PEAKS; p ++)
    {
        ipeaks[p] = 1;
        peaks[p].level = 0;

        for (int i = 1; i < N_FREQS - 1; i ++)
        {
            if (freqs[i] > peaks[p].level && ! skip[i])
            {
                ipeaks[p] = i;
                peaks[p].level = freqs[i];
            }
        }

        int skiplow = (int) lroundf (ipeaks[p] * 0.9f);
        int skiphigh = (int) lroundf (ipeaks[p] * 1.1f);

        if (skiplow < 0)
            skiplow = 0;
        if (skiphigh > N_FREQS - 1)
            skiphigh = N_FREQS - 1;

        for (int i = skiplow; i <= skiphigh; i ++)
            skip[i] = true;
    }

    for (int p = 0; p < N_PEAKS; p ++)
    {
        float a = freqs[ipeaks[p] - 1];
        float b = freqs[ipeaks[p]];
        float c = freqs[ipeaks[p] + 1];

        float num = a - c;
        float denom = 2 * a - 4 * b + 2 * c;

        peaks[p].freq_hz = (ipeaks[p] + num / denom) * SAMPLERATE / N_SAMPLES;
    }
}

static DetectedTone invalid_tone (void)
{
    DetectedTone tone;

    tone.tone_hz = INVALID_VAL;
    tone.harm_score = INVALID_VAL;
    tone.harm_stretch = INVALID_VAL;

    for(int i = 0; i < N_OVERTONES; i ++)
        tone.overtones_hz[i] = INVALID_VAL;

    return tone;
}

static DetectedTone analyze_tone (const Peak peaks[N_PEAKS], float tone_hz)
{
    DetectedTone tone = invalid_tone ();

    tone.tone_hz = tone_hz;
    tone.harm_score = 0;

    float stretchsum = 0;
    float levelsum = 0;

    for (int t = 1; t <= N_OVERTONES; t ++)
    {
        float min_harm_hz = tone_hz * t * 0.95f;
        float max_harm_hz = tone_hz * t * 1.05f;

        bool found = false;

        for (int p = 0; p < N_PEAKS; p ++)
        {
            if (peaks[p].freq_hz > min_harm_hz && peaks[p].freq_hz < max_harm_hz)
            {
                tone.overtones_hz[t - 1] = peaks[p].freq_hz;
                tone.harm_score += peaks[p].freq_hz * peaks[p].level;

                if (t >= 2)
                {
                    float stretch = 12 * logf (peaks[p].freq_hz / tone_hz) / logf (t) - 12;

                    stretchsum += stretch * peaks[p].level;
                    levelsum += peaks[p].level;
                }

                found = true;
                break;
            }
        }

        if (! found)
            break;
    }

    if (levelsum > 0)
        tone.harm_stretch = stretchsum / levelsum;

    return tone;
}

static bool is_same_tone (float tone_hz, float ref_hz)
{
    return tone_hz > ref_hz * 0.95f && tone_hz < ref_hz * 1.05f;
}

static bool is_overtone (float tone_hz, float ref_hz)
{
    return is_same_tone (tone_hz, ref_hz * 2)
     || is_same_tone (tone_hz, ref_hz * 3)
     || is_same_tone (tone_hz, ref_hz * 4)
     || is_same_tone (tone_hz, ref_hz * 5);
}

static float last_tone_hz = INVALID_VAL;

DetectedTone tone_detect (const float freqs[N_FREQS], float min_tone_hz, float max_tone_hz)
{
    Peak peaks[N_PEAKS];
    find_peaks (freqs, peaks);

    DetectedTone best_tone = invalid_tone ();

    for (int p = 0; p < N_PEAKS; p ++)
    {
        if (peaks[p].freq_hz < min_tone_hz || peaks[p].freq_hz > max_tone_hz)
            continue;

        DetectedTone tone = analyze_tone (peaks, peaks[p].freq_hz);

        /*
         * Experimental tweaks:
         * 1. Favor the same peak found last cycle (reduces "jumpiness")
         * 2. Favor low notes that may be hidden by their own overtones
         */
        if (last_tone_hz > INVALID_VAL &&
         (is_same_tone (tone.tone_hz, last_tone_hz) ||
         (tone.tone_hz < 200 && is_overtone (last_tone_hz, tone.tone_hz))))
            tone.harm_score *= (tone.tone_hz < 100) ? 4 : 2;

        if (tone.harm_score > best_tone.harm_score)
            best_tone = tone;
    }

    last_tone_hz = best_tone.tone_hz;

    return best_tone;
}
