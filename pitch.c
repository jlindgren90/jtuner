/*
 * JTuner - pitch.c
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

/* The following procedures use a quadratic adjustment to implement "stretched"
 * tuning, where each semitone n (relative to middle C) is tuned sharper or
 * flatter than equal temperament by s/2*(n/12)^2 semitones.  This results in
 * each octave moving away from middle C being incrementally stretched by s
 * semitones relative to the previous octave. */

static float semitones_to_ratio (float s, float n)
{
    float sign = (n > 0) ? 1 : -1;
    return powf (2, n / 12 + sign * n * n * s / 3456);
}

static float ratio_to_semitones (float s, float x)
{
    if (fabsf (s) < 0.0001f)  /* prevent division by zero */
        return 12 * log2f (x);

    float sign = (x > 1) ? 1 : -1;
    return sign * (24 * sqrtf (36 + sign * 6 * s * log2f (x)) - 144) / s;
}

static float c4_tone_hz (float s)
{
    return A4_TONE_HZ / semitones_to_ratio (s, A4_PITCH - C4_PITCH);
}

float pitch_to_tone_hz (float s, float pitch)
{
    return c4_tone_hz (s) * semitones_to_ratio (s, pitch - C4_PITCH);
}

float model_harm_stretch (float s, float pitch1, float pitch2)
{
    float n1 = pitch1 - C4_PITCH;
    float n2 = pitch2 - C4_PITCH;
    float sign1 = (n1 > 0) ? 1 : -1;
    float sign2 = (n2 > 0) ? 1 : -1;
    float adj1 = sign1 * n1 * n1 * s / 288;
    float adj2 = sign2 * n2 * n2 * s / 288;
    return adj2 - adj1;
}

DetectedPitch pitch_identify (float s, float tone_hz)
{
    static int last_pitch = INVALID_VAL;
    static int timein = 0;
    static int timeout = 0;

    float pitch_real = INVALID_VAL;
    int pitch_rounded = INVALID_VAL;

    if (tone_hz > INVALID_VAL)
    {
        pitch_real = C4_PITCH + ratio_to_semitones (s, tone_hz / c4_tone_hz (s));
        pitch_rounded = (int) lroundf (pitch_real);
    }

    if (pitch_rounded == last_pitch)
    {
        if (timein)
            timein --;

        if (! timein)
            timeout = TIMEOUT;
    }
    else
    {
        last_pitch = pitch_rounded;
        timein = TIMEIN - 1;

        if (timeout)
            timeout --;
    }

    DetectedPitch pitch = {
        .state = DETECT_NONE,
        .pitch = pitch_rounded,
        .off_by = pitch_real - pitch_rounded
    };

    if (timeout)
    {
        if (timein)
            pitch.state = DETECT_KEEP;
        else if (pitch_rounded > INVALID_VAL)
            pitch.state = DETECT_UPDATE;
    }

    return pitch;
}
