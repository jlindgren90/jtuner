/*
 * JTuner - tone.c
 * Copyright 2013 John Lindgren
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
#include <string.h>

#define N_PEAKS 32
#define N_OVERTONES 16

#define MIN_FREQ 20
#define MAX_FREQ 4000

#define SQRT_2 1.41421356237

void find_peaks (const float freqs[N_FREQS], float peaks[N_PEAKS], float levels[N_PEAKS])
{
    char skip[N_FREQS] = {0};
    int ipeaks[N_PEAKS];

    for (int p = 0; p < N_PEAKS; p ++)
    {
        ipeaks[p] = 1;
        levels[p] = 0;

        for (int i = 1; i < N_FREQS - 1; i ++)
        {
            if (freqs[i] > levels[p] && ! skip[i])
            {
                ipeaks[p] = i;
                levels[p] = freqs[i];
            }
        }

        int skiplow = lroundf (ipeaks[p] * 0.9);
        int skiphigh = lroundf (ipeaks[p] * 1.1);

        if (skiplow < 0)
            skiplow = 0;
        if (skiphigh > N_FREQS - 1)
            skiphigh = N_FREQS - 1;

        memset (skip + skiplow, 1, skiphigh + 1 - skiplow);
    }

    for (int p = 0; p < N_PEAKS; p ++)
    {
        float a = freqs[ipeaks[p] - 1];
        float b = freqs[ipeaks[p]];
        float c = freqs[ipeaks[p] + 1];

        float num = a - c;
        float denom = 2 * a - 4 * b + 2 * c;

        peaks[p] = (ipeaks[p] + num / denom) * SAMPLERATE / N_SAMPLES;
    }
}

float calc_harm_score (float peaks[N_PEAKS], float levels[N_PEAKS], float root)
{
    if (root < MIN_FREQ || root > MAX_FREQ)
        return 0;

    float score = 0;

    for (int t = 1; t <= N_OVERTONES; t ++)
    {
        float mintone = root * t * 0.95;
        float maxtone = root * t * 1.05;

        bool found = false;

        for (int p = 0; p < N_PEAKS; p ++)
        {
            if (peaks[p] > mintone && peaks[p] < maxtone)
            {
                score += levels[p] * peaks[p];
                found = true;
                break;
            }
        }

        if (! found)
            break;
    }

    return score;
}

float calc_harm_stretch (float peaks[N_PEAKS], float levels[N_PEAKS], float root)
{
    float stretchsum = 0;
    float levelsum = 0;

    for (int t = 2; t <= N_OVERTONES; t ++)
    {
        float mintone = root * t * 0.95;
        float maxtone = root * t * 1.05;

        bool found = false;

        for (int p = 0; p < N_PEAKS; p ++)
        {
            if (peaks[p] > mintone && peaks[p] < maxtone)
            {
                stretchsum += levels[p] * (12 * logf (peaks[p] / root) / logf (t) - 12);
                levelsum += levels[p];
                found = true;
            }
        }

        if (! found)
            break;
    }

    return stretchsum / levelsum;
}

float tone_detect (const float freqs[N_FREQS], float target, float * harm_stretch)
{
    float levels[N_PEAKS];
    float peaks[N_PEAKS];

    find_peaks (freqs, peaks, levels);

    float tone = 0;
    float topscore = 0;

    for (int p = 0; p < N_PEAKS; p ++)
    {
        if (target && (peaks[p] < target / SQRT_2 || peaks[p] > target * SQRT_2))
            continue;

        float score = calc_harm_score (peaks, levels, peaks[p]);

        if (score > topscore)
        {
            tone = peaks[p];
            topscore = score;
        }
    }

    * harm_stretch = calc_harm_stretch (peaks, levels, tone);

    return tone;
}
