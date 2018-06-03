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
#define OVERTONES_USED 5

#define MAX_COLLECT 100

typedef struct {
    float vals[MAX_COLLECT];
    int num_vals;
} Collector;

static Collector collect_off_by[NUM_PITCHES];
static Collector collect_harm_stretch[NUM_PITCHES];
static Collector collect_overtones[NUM_PITCHES][OVERTONES_USED];

static const char * note_names[12] =
 {"C", "C♯", "D", "E♭", "E", "F", "F♯", "G", "A♭", "A", "B♭", "B"};
static const int overtone_pitches[OVERTONES_USED] =
 {12, 19, 24, 28, 31};

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

static void collect_val (Collector * c, float val)
{
    if (c->num_vals < MAX_COLLECT)
        c->vals[c->num_vals ++] = val;
}

static void collect_pitch (const DetectedPitch * pitch, float harm_stretch,
 const OvertonePitch opitches[N_OVERTONES])
{
    if (pitch->pitch < MIN_PITCH || pitch->pitch > MAX_PITCH)
        return;

    int index = pitch->pitch - MIN_PITCH;

    collect_val (& collect_off_by[index], pitch->off_by);

    if (harm_stretch > INVALID_VAL)
        collect_val (& collect_harm_stretch[index], harm_stretch);

    for (int i = 0; i < OVERTONES_USED; i ++)
    {
        const OvertonePitch * opitch = & opitches[1 + i];

        if (opitch->pitch != pitch->pitch + overtone_pitches[i])
            break;

        collect_val (& collect_overtones[index][i], opitch->off_by);
    }
}

static void process_freqs (float freqs[N_FREQS], FILE * out)
{
    float target_hz = pitch_to_tone_hz (OCTAVE_STRETCH, stable_pitch);
    DetectedTone tone = tone_detect (freqs, target_hz);
    DetectedPitch pitch = pitch_identify (OCTAVE_STRETCH, tone.tone_hz);

    if (pitch.state == DETECT_UPDATE)
    {
        if (pitch.pitch == stable_pitch || pitch.pitch == stable_pitch + 1)
        {
            fprintf (out, "%s%d,%.02f Hz,%+.04f,%+.04f",
             note_names[pitch.pitch % 12], pitch.pitch / 12, tone.tone_hz,
             tone.harm_stretch, pitch.off_by);

            OvertonePitch opitches[N_OVERTONES];
            identify_overtones (OCTAVE_STRETCH, tone.overtones_hz, opitches);

            for (int i = 0; i < OVERTONES_USED; i ++)
            {
                const OvertonePitch * opitch = & opitches[1 + i];

                if (opitch->pitch != pitch.pitch + overtone_pitches[i])
                    break;

                fprintf (out, ",,%s%d,%.02f Hz,%+.04f",
                 note_names[opitch->pitch % 12], opitch->pitch / 12,
                 tone.overtones_hz[1 + i], opitch->off_by);
            }

            fprintf (out, "\n");

            collect_pitch (& pitch, tone.harm_stretch, opitches);
        }

        detect_stable_pitch (pitch.pitch);
    }
}

static int compare_float (const void * f1, const void * f2)
{
    return (* (const float *) f1 < * (const float *) f2) ? -1 :
           (* (const float *) f1 > * (const float *) f2) ? 1 : 0;
}

static float compute_median (Collector * c)
{
    if (c->num_vals < 1)
        return INVALID_VAL;

    qsort (c->vals, (size_t) c->num_vals, sizeof (float), compare_float);

    return (c->num_vals % 2) ? c->vals[c->num_vals / 2] :
           0.5f * (c->vals[c->num_vals / 2] + c->vals[c->num_vals / 2 + 1]);
}

static void run_offline (FILE * in, FILE * out)
{
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
        float model = model_harm_stretch (OCTAVE_STRETCH, pitch, pitch + 12);
        float harm_stretch = compute_median (& collect_harm_stretch[index]);
        float off_by = compute_median (& collect_off_by[index]);

        fprintf (out, "%s%d,%+.02f,%+.02f,%+.02f",
         note_names[pitch % 12], pitch / 12, model, harm_stretch, off_by);

        for (int i = 0; i < OVERTONES_USED; i ++)
        {
            int overtone_pitch = pitch + overtone_pitches[i];
            float overtone_off_by = compute_median (& collect_overtones[index][i]);

            if (overtone_off_by <= INVALID_VAL)
                break;

            fprintf (out, ",,%s%d,%+.02f", note_names[overtone_pitch % 12],
             overtone_pitch / 12, overtone_off_by);
        }

        fprintf (out, "\n");
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
