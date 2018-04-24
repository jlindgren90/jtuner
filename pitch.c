/*
 * JTuner - pitch.c
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

#define C4_SEMITONES  48  /* semitones from C0 to C4 */
#define A4_TONE      440  /* concert pitch (A4 = 440 Hz) */

/* The following procedures use a quadratic adjustment to implement "stretched"
 * tuning, where each semitone n (relative to middle C) is tuned sharper or
 * flatter than equal temperament by s/2*(n/12)^2 semitones.  This results in
 * each octave moving away from middle C being incrementally stretched by s
 * semitones relative to the previous octave. */

static float semitones_to_ratio (const TunerConfig * config, float n)
{
    float s = config->octave_stretch;
    float sign = (n > 0) ? 1 : -1;
    return powf (2, n / 12 + sign * n * n * s / 3456);
}

static float ratio_to_semitones (const TunerConfig * config, float x)
{
    float s = config->octave_stretch;

    if (! s)  /* prevent division by zero */
        return 12 * log2f (x);

    float sign = (x > 1) ? 1 : -1;
    return sign * (24 * sqrt (36 + sign * 6 * s * log2f (x)) - 144) / s;
}

static float c4_tone (const TunerConfig * config)
{
    return A4_TONE / semitones_to_ratio (config, 9);
}

float calc_target (const TunerConfig * config)
{
    if (! config->target_octave)
        return 0;

    float scale = 12 * config->target_octave - C4_SEMITONES;
    return c4_tone (config) * semitones_to_ratio (config, scale);
}

DetectState pitch_identify (const TunerConfig * config, float tone, int * pitch, float * off_by)
{
    float scale = ratio_to_semitones (config, tone / c4_tone (config));
    int newpitch = tone ? lroundf (scale) : INVALID_VAL;

    static int lastpitch = INVALID_VAL;
    static int timein = 0;
    static int timeout = 0;

    if (newpitch == lastpitch)
    {
        if (timein)
            timein --;

        if (! timein)
            timeout = TIMEOUT;
    }
    else
    {
        lastpitch = newpitch;
        timein = TIMEIN - 1;

        if (timeout)
            timeout --;
    }

    if (! timeout)
        return DETECT_NONE;

    if (timein)
        return DETECT_KEEP;

    if (newpitch == INVALID_VAL)
        return DETECT_NONE;

    * pitch = C4_SEMITONES + newpitch;
    * off_by = scale - newpitch;

    return DETECT_UPDATE;
}
