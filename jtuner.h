/*
 * JTuner - jtuner.h
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

#ifndef JTUNER_H
#define JTUNER_H

#include <stdbool.h>
#include <stdint.h>

#define SAMPLERATE 44100

#define N_SAMPLES 32768
#define N_SAMPLES_LOG2 15

#define N_STEPS 16

#define SAMPLES_PER_STEP (N_SAMPLES / N_STEPS)

#define TIMEIN 5
#define TIMEOUT 10

#define N_FREQS (N_SAMPLES / 2 + 1)

#define INVALID_VAL -999

typedef enum {
    DETECT_NONE,
    DETECT_UPDATE,
    DETECT_KEEP
} DetectState;

typedef struct {
    float octave_stretch;
    float target_octave;
} TunerConfig;

typedef struct {
    float tone_hz;
    float harm_stretch;
} DetectedTone;

typedef struct {
    DetectState state;
    DetectedTone tone;
    float off_by;
    int pitch;
} TunerStatus;

/* fft.c */
void fft_init (void);
void fft_run (const float data[N_SAMPLES], float freqs[N_FREQS]);

/* io.c */
bool io_init (void);
bool io_read_samples (float data[N_SAMPLES]);
void io_cleanup (void);

/* pitch.c */
float model_harm_stretch (const TunerConfig * config, int pitch);
float calc_target (const TunerConfig * config);
DetectState pitch_identify (const TunerConfig * config, float tone, int * pitch, float * off_by);

/* tone.c */
DetectedTone tone_detect (const float freqs[N_FREQS], float target_hz);

#endif // JTUNER_H
