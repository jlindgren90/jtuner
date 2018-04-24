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
    gdk_window_set_back_pixmap (gdk_window, NULL, FALSE);
}

static gboolean redraw (GtkWidget * window, GdkEventExpose * event)
{
    cairo_t * cr = gdk_cairo_create (gtk_widget_get_window (window));
    pthread_mutex_lock (& mutex);
    draw_tuner (window, cr, & status);
    pthread_mutex_unlock (& mutex);
    cairo_destroy (cr);
    return TRUE;
}

static gboolean queue_redraw (void * arg)
{
    gtk_widget_queue_draw (tuner);
    return FALSE;
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

    config.octave_stretch = 0.05;

    pthread_t io_thread;
    pthread_create (& io_thread, NULL, io_worker, NULL);

    gtk_window_set_default_icon_name ("jtuner");

    GtkWidget * window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title ((GtkWindow *) window, "JTuner");
    gtk_window_set_default_size ((GtkWindow *) window, 600, 400);

    g_signal_connect (window, "realize", (GCallback) disable_fill, NULL);
    g_signal_connect (window, "destroy", (GCallback) gtk_main_quit, NULL);

    GtkWidget * vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add ((GtkContainer *) window, vbox);

    tuner = gtk_drawing_area_new ();
    gtk_box_pack_start ((GtkBox *) vbox, tuner, TRUE, TRUE, 0);

    g_signal_connect (tuner, "expose-event", (GCallback) redraw, NULL);

    GtkWidget * hbox = gtk_hbox_new (FALSE, 6);
    gtk_container_set_border_width ((GtkContainer *) hbox, 3);
    gtk_box_pack_start ((GtkBox *) vbox, hbox, FALSE, FALSE, 0);

    GtkWidget * stretch_label = gtk_label_new ("Octave stretch (semitones):");
    gtk_box_pack_start ((GtkBox *) hbox, stretch_label, TRUE, FALSE, 0);

    GtkWidget * stretch_spin = gtk_spin_button_new_with_range (-1.0, 1.0, 0.01);
    gtk_spin_button_set_value ((GtkSpinButton *) stretch_spin, config.octave_stretch);
    gtk_box_pack_start ((GtkBox *) hbox, stretch_spin, TRUE, FALSE, 0);

    g_signal_connect (stretch_spin, "value-changed", (GCallback) adjust_stretch, NULL);

    GtkWidget * target_label = gtk_label_new ("Target octave:");
    gtk_box_pack_start ((GtkBox *) hbox, target_label, TRUE, FALSE, 0);

    GtkWidget * target_spin = gtk_spin_button_new_with_range (0.0, 8.0, 0.1);
    gtk_spin_button_set_value ((GtkSpinButton *) target_spin, config.target_octave);
    gtk_box_pack_start ((GtkBox *) hbox, target_spin, TRUE, FALSE, 0);

    g_signal_connect (target_spin, "value-changed", (GCallback) adjust_target, NULL);

    gtk_widget_show_all (window);

    gtk_main ();

    pthread_mutex_lock (& mutex);
    quit_flag = TRUE;
    pthread_mutex_unlock (& mutex);

    pthread_join (io_thread, NULL);

    return 0;
}
