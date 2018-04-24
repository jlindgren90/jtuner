/*
 * JTuner - jtuner-offline.c
 * Copyright 2018 John Lindgren
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jtuner.h"

static const char * note_names[12] =
 {"C", "C♯", "D", "E♭", "E", "F", "F♯", "G", "A♭", "A", "B♭", "B"};

static const TunerConfig config = {
    .octave_stretch = 0.05f,
    .target_octave = 0.0f
};

static void error_exit (const char * error)
{
    fprintf (stderr, "%s\n", error);
    exit (1);
}

static bool read_step (FILE * in, float data[SAMPLES_PER_STEP])
{
    int16_t ibuf[SAMPLES_PER_STEP];

    if (fread (ibuf, sizeof ibuf[0], SAMPLES_PER_STEP, in) != SAMPLES_PER_STEP)
        return false;

    for (int i = 0; i < SAMPLES_PER_STEP; i ++)
        data[i] = ibuf[i] / 32767.0f;

    return true;
}

static bool read_samples (FILE * in, float data[N_SAMPLES])
{
    static bool filled = false;

    if (filled)
        memmove (data, data + SAMPLES_PER_STEP, (N_STEPS - 1) * SAMPLES_PER_STEP * sizeof data[0]);
    else
    {
        for (int i = 0; i < N_STEPS - 1; i ++)
        {
            if (! read_step (in, data + i * SAMPLES_PER_STEP))
                return false;
        }

        filled = true;
    }

    return read_step (in, data + (N_STEPS - 1) * SAMPLES_PER_STEP);
}

static void run_offline (FILE * in, FILE * out)
{
    float data[N_SAMPLES];
    float freqs[N_FREQS];

    fft_init ();

    fprintf (out, "Note,Freq,Harm,Err\n");

    while (read_samples (in, data))
    {
        fft_run (data, freqs);

        TunerStatus status;

        float target = calc_target (& config);
        status.tone = tone_detect (freqs, target, & status.harm_stretch);
        status.state = pitch_identify (& config, status.tone,
         & status.pitch, & status.off_by);

        if (status.state == DETECT_UPDATE)
            fprintf (out, "%s%d,%.02f Hz,%+.02f,%+.02f\n",
             note_names[status.pitch % 12], status.pitch / 12, status.tone,
             status.harm_stretch, status.off_by);
    }
}

int main (int argc, char * * argv)
{
    if (argc != 3)
        error_exit ("Usage: jtuner-offline <file>.raw <file>.csv");

    FILE * in = fopen (argv[1], "rb");
    if (! in)
        error_exit ("error opening input file");

    FILE * out = fopen (argv[2], "wb");
    if (! out)
        error_exit ("error opening output file");

    run_offline (in, out);

    fclose (in);
    fclose (out);
    return 0;
}
