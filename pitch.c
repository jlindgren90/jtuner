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

#define INVALID_PITCH -100

float calc_target (const TunerConfig * config)
{
    if (! config->target_octave)
        return 0;

    float octave = powf (2, 1 + config->octave_stretch / 12);
    float scale = 12 * config->target_octave - 57;

    return 440 * powf (octave, scale / 12);
}

DetectState pitch_identify (const TunerConfig * config, float tone, int * pitch, float * off_by)
{
    float octave = powf (2, 1 + config->octave_stretch / 12);
    float scale = 12 * logf (tone / 440) / logf (octave);
    int newpitch = tone ? lroundf (scale) : INVALID_PITCH;

    static int lastpitch = INVALID_PITCH;
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

    if (newpitch == INVALID_PITCH)
        return DETECT_NONE;

    * pitch = 57 + newpitch;
    * off_by = scale - newpitch;

    return DETECT_UPDATE;
}
