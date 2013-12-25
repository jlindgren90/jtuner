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

DetectState pitch_identify (const TunerConfig * config, float tone, int * pitch, float * off_by)
{
    float a440 = 440 * powf (2, config->pitch_adjust / 12);
    float octave = powf (2, 1 + config->octave_stretch / 12);
    float scale = 12 * logf (tone / a440) / logf (octave);
    int newpitch = lroundf (scale);

    static int lastpitch = -1;
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

    * pitch = 57 + newpitch;
    * off_by = scale - newpitch;

    return DETECT_UPDATE;
}
