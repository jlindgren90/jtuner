/*
 * JTuner - io.c
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

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <alsa/asoundlib.h>

#define SAMPLES_PER_STEP (N_SAMPLES / N_STEPS)

static snd_pcm_t * handle;

bool io_init (void)
{
    if (snd_pcm_open (& handle, "default", SND_PCM_STREAM_CAPTURE, 0) < 0)
        return false;

    snd_pcm_hw_params_t * params;
    snd_pcm_hw_params_alloca (& params);

    if (snd_pcm_hw_params_any (handle, params) < 0)
        goto ERR_CLOSE;

    if (snd_pcm_hw_params_set_access (handle, params, SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
        goto ERR_CLOSE;

    if (snd_pcm_hw_params_set_format (handle, params, SND_PCM_FORMAT_S16) < 0)
        goto ERR_CLOSE;

    if (snd_pcm_hw_params_set_channels (handle, params, 1) < 0)
        goto ERR_CLOSE;

    if (snd_pcm_hw_params_set_rate (handle, params, SAMPLERATE, 0) < 0)
        goto ERR_CLOSE;

    if (snd_pcm_hw_params (handle, params) < 0)
        goto ERR_CLOSE;

    return true;

ERR_CLOSE:
    snd_pcm_close (handle);
    return false;
}

static bool io_read_step (float data[SAMPLES_PER_STEP])
{
    int16_t ibuf[SAMPLES_PER_STEP];

    if (snd_pcm_readi (handle, ibuf, SAMPLES_PER_STEP) != SAMPLES_PER_STEP)
        return false;

    for (int i = 0; i < SAMPLES_PER_STEP; i ++)
        data[i] = ibuf[i] / 32767.0;

    return true;
}

bool io_read_samples (float data[N_SAMPLES])
{
    static bool filled = false;

    if (filled)
        memmove (data, data + SAMPLES_PER_STEP, (N_STEPS - 1) * SAMPLES_PER_STEP * sizeof data[0]);
    else
    {
        for (int i = 0; i < N_STEPS - 1; i ++)
        {
            if (! io_read_step (data + i * SAMPLES_PER_STEP))
                return false;
        }

        filled = true;
    }

    return io_read_step (data + (N_STEPS - 1) * SAMPLES_PER_STEP);
}

void io_cleanup (void)
{
    snd_pcm_close (handle);
}
