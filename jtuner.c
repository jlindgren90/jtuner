/*
 * JTuner - jtuner.c
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

#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "draw.h"

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static TunerConfig config;
static TunerStatus status;
static GtkWidget * tuner;
static bool quit_flag;

static void disable_fill (GtkWidget * window)
{
    GdkWindow * gdk_window = gtk_widget_get_window (window);
    gdk_window_set_background_pattern (gdk_window, NULL);
}

static gboolean redraw (GtkWidget * window, cairo_t * cr)
{
    pthread_mutex_lock (& mutex);
    draw_tuner (window, cr, & status);
    pthread_mutex_unlock (& mutex);
    return TRUE;
}

static gboolean queue_redraw (void * arg)
{
    gtk_widget_queue_draw (tuner);
    return FALSE;
}

static void adjust_pitch (GtkWidget * spin)
{
    pthread_mutex_lock (& mutex);
    config.pitch_adjust = gtk_spin_button_get_value ((GtkSpinButton *) spin);
    pthread_mutex_unlock (& mutex);
}

static void adjust_stretch (GtkWidget * spin)
{
    pthread_mutex_lock (& mutex);
    config.octave_stretch = gtk_spin_button_get_value ((GtkSpinButton *) spin);
    pthread_mutex_unlock (& mutex);
}

static void adjust_target (GtkWidget * spin)
{
    pthread_mutex_lock (& mutex);
    config.target_octave = gtk_spin_button_get_value ((GtkSpinButton *) spin);
    pthread_mutex_unlock (& mutex);
}

static void error_exit (const char * error)
{
    fprintf (stderr, "%s\n", error);
    exit (1);
}

static void * io_worker (void * arg)
{
    fft_init ();

    if (! io_init ())
        error_exit ("audio init error");

    float data[N_SAMPLES];
    float freqs[N_FREQS];

    bool quit = false;

    while (! quit)
    {
        if (! io_read_samples (data))
            error_exit ("audio read error");

        fft_run (data, freqs);

        pthread_mutex_lock (& mutex);

        TunerStatus new_status;

        float target = calc_target (& config);
        new_status.tone = tone_detect (freqs, target, & new_status.harm_stretch);
        new_status.state = pitch_identify (& config, new_status.tone,
         & new_status.pitch, & new_status.off_by);

        if (new_status.state == DETECT_UPDATE ||
         (new_status.state == DETECT_NONE && status.state != DETECT_NONE))
        {
            status = new_status;
            g_timeout_add (0, queue_redraw, NULL);
        }

        quit = quit_flag;

        pthread_mutex_unlock (& mutex);
    }

    io_cleanup ();

    return NULL;
}

int main (void)
{
    gtk_init (NULL, NULL);

    pthread_t io_thread;
    pthread_create (& io_thread, NULL, io_worker, NULL);

    gtk_window_set_default_icon_name ("jtuner");

    GtkWidget * window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title ((GtkWindow *) window, "JTuner");
    gtk_window_set_default_size ((GtkWindow *) window, 600, 400);

    g_signal_connect (window, "realize", (GCallback) disable_fill, NULL);
    g_signal_connect (window, "destroy", (GCallback) gtk_main_quit, NULL);

    GtkWidget * grid = gtk_grid_new ();
    gtk_container_add ((GtkContainer *) window, grid);

    tuner = gtk_drawing_area_new ();
    gtk_widget_set_hexpand (tuner, TRUE);
    gtk_widget_set_vexpand (tuner, TRUE);
    gtk_grid_attach ((GtkGrid *) grid, tuner, 0, 0, 4, 1);

    g_signal_connect (tuner, "draw", (GCallback) redraw, NULL);

    gtk_grid_attach ((GtkGrid *) grid, gtk_label_new ("Pitch adjust (half steps):"), 0, 1, 1, 1);

    GtkWidget * pitch_spin = gtk_spin_button_new_with_range (-1.0, 1.0, 0.01);
    gtk_spin_button_set_value ((GtkSpinButton *) pitch_spin, 0.0);
    gtk_grid_attach ((GtkGrid *) grid, pitch_spin, 1, 1, 1, 1);

    g_signal_connect (pitch_spin, "value-changed", (GCallback) adjust_pitch, NULL);

    gtk_grid_attach ((GtkGrid *) grid, gtk_label_new ("Octave stretch (half steps):"), 2, 1, 1, 1);

    GtkWidget * stretch_spin = gtk_spin_button_new_with_range (-1.0, 1.0, 0.01);
    gtk_spin_button_set_value ((GtkSpinButton *) stretch_spin, 0.0);
    gtk_grid_attach ((GtkGrid *) grid, stretch_spin, 3, 1, 1, 1);

    g_signal_connect (stretch_spin, "value-changed", (GCallback) adjust_stretch, NULL);

    gtk_grid_attach ((GtkGrid *) grid, gtk_label_new ("Target octave:"), 0, 2, 1, 1);

    GtkWidget * target_spin = gtk_spin_button_new_with_range (0.0, 8.0, 0.1);
    gtk_spin_button_set_value ((GtkSpinButton *) target_spin, 0.0);
    gtk_grid_attach ((GtkGrid *) grid, target_spin, 1, 2, 1, 1);

    g_signal_connect (target_spin, "value-changed", (GCallback) adjust_target, NULL);

    gtk_widget_show_all (window);

    gtk_main ();

    pthread_mutex_lock (& mutex);
    quit_flag = TRUE;
    pthread_mutex_unlock (& mutex);

    pthread_join (io_thread, NULL);

    return 0;
}
