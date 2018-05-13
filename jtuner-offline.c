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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jtuner.h"

#define MIN_PITCH 9  /* A0 */
#define MAX_PITCH 96 /* C8 */

#define OCTAVE_STRETCH 0.05f

#define NUM_PITCHES (MAX_PITCH + 1 - MIN_PITCH)

#define MAX_COLLECT 100

static float collect_off_by[NUM_PITCHES][MAX_COLLECT];
static int num_collect_off_by[NUM_PITCHES];
static float collect_harm_stretch[NUM_PITCHES][MAX_COLLECT];
static int num_collect_harm_stretch[NUM_PITCHES];

static const char * note_names[12] =
 {"C", "C♯", "D", "E♭", "E", "F", "F♯", "G", "A♭", "A", "B♭", "B"};

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

static int stable_pitch = 9; /* A0 */

static void detect_stable_pitch (int pitch)
{
    static int last_pitch = -1;
    static int last_pitch_count = 0;

    if (pitch == last_pitch)
    {
        if (++ last_pitch_count == 10)
            stable_pitch = last_pitch;
    }
    else
    {
        last_pitch = pitch;
        last_pitch_count = 0;
    }
}

static void collect_status (const TunerStatus * status)
{
    if (status->pitch < MIN_PITCH || status->pitch > MAX_PITCH)
        return;

    int index = status->pitch - MIN_PITCH;

    if (num_collect_off_by[index] < MAX_COLLECT)
    {
        collect_off_by[index][num_collect_off_by[index]] = status->off_by;
        num_collect_off_by[index] ++;
    }

    if (status->harm_stretch != INVALID_VAL && num_collect_harm_stretch[index] < MAX_COLLECT)
    {
        collect_harm_stretch[index][num_collect_harm_stretch[index]] = status->harm_stretch;
        num_collect_harm_stretch[index] ++;
    }
}

static void process_freqs (float freqs[N_FREQS], FILE * out)
{
    const TunerConfig config = {
        .octave_stretch = OCTAVE_STRETCH,
        .target_octave = stable_pitch / 12.0f
    };

    TunerStatus status;

    float target = calc_target (& config);
    status.tone = tone_detect (freqs, target, & status.harm_stretch);
    status.state = pitch_identify (& config, status.tone,
     & status.pitch, & status.off_by);

    if (status.state == DETECT_UPDATE)
    {
        if (status.pitch == stable_pitch || status.pitch == stable_pitch + 1)
        {
            fprintf (out, "%s%d,%.02f Hz,%+.04f,%+.04f\n",
             note_names[status.pitch % 12], status.pitch / 12, status.tone,
             status.harm_stretch, status.off_by);

            collect_status (& status);
        }

        detect_stable_pitch (status.pitch);
    }
}

static int compare_float (const void * f1, const void * f2)
{
    return (* (const float *) f1 < * (const float *) f2) ? -1 :
           (* (const float *) f1 > * (const float *) f2) ? 1 : 0;
}

static float compute_median (float * values, int num_values)
{
    if (num_values < 1)
        return INVALID_VAL;

    qsort (values, num_values, sizeof values[0], compare_float);

    return (num_values % 2) ? values[num_values / 2] :
           0.5f * (values[num_values / 2] + values[num_values / 2 + 1]);
}

static void run_offline (FILE * in, FILE * out)
{
    const TunerConfig config = {
        .octave_stretch = OCTAVE_STRETCH,
        .target_octave = 0.0f
    };

    float data[N_SAMPLES];
    float freqs[N_FREQS];

    fft_init ();

    fprintf (out, "Raw Data\n");
    fprintf (out, "Note,Freq,Harm,Err\n");

    while (read_samples (in, data))
    {
        fft_run (data, freqs);
        process_freqs (freqs, out);
    }

    fprintf (out, "\nMedians\n");
    fprintf (out, "Note,Model,Harm,Err\n");

    for (int index = 0; index < NUM_PITCHES; index ++)
    {
        int pitch = MIN_PITCH + index;
        float model = model_harm_stretch (& config, pitch);
        float harm_stretch = compute_median (collect_harm_stretch[index],
         num_collect_harm_stretch[index]);
        float off_by = compute_median (collect_off_by[index],
         num_collect_off_by[index]);

        fprintf (out, "%s%d,%+.02f,%+.02f,%+.02f\n",
         note_names[pitch % 12], pitch / 12, model, harm_stretch, off_by);
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
