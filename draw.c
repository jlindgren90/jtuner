/*
 * JTuner - draw.c
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

#include "draw.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static const char * note_names[12] =
 {" C", "C♯", " D", "E♭", " E", " F", "F♯", " G", "A♭", " A", "B♭", " B"};

static void draw_text (GtkWidget * widget, cairo_t * cr, int x, int y,
 int width, const char * text, const char * font)
{
    cairo_move_to (cr, x, y);
    cairo_set_source_rgba (cr, 1, 1, 1, 1);

    PangoFontDescription * desc = pango_font_description_from_string (font);
    PangoLayout * pl = gtk_widget_create_pango_layout (widget, NULL);
    pango_layout_set_text (pl, text, -1);
    pango_layout_set_font_description (pl, desc);
    pango_layout_set_width (pl, width * PANGO_SCALE);
    pango_layout_set_alignment (pl, PANGO_ALIGN_CENTER);
    pango_font_description_free (desc);

    pango_cairo_show_layout (cr, pl);

    g_object_unref (pl);
}

static void draw_dial (GtkWidget * widget, cairo_t * cr, int x, int y,
 int width, int height, bool valid, float value)
{
    int radius = MIN (width / 2, height * 3 / 4);
    int base = height - (height - radius) / 2;

    float angle = 0.5 * M_PI * (1 - value);

    if (valid)
    {
        cairo_set_source_rgb (cr, 1, 1, 1);
        cairo_set_line_width (cr, 8);
        cairo_move_to (cr, x + width / 2, base);
        cairo_line_to (cr, x + width / 2 + radius * cosf (angle), base - radius * sinf (angle));
        cairo_stroke (cr);
    }

    cairo_set_source_rgb (cr, 1, 1, 1);
    cairo_set_line_width (cr, 4);

    for (int i = -2; i <= 2; i ++)
    {
        angle = M_PI * (0.5 - 0.125 * i);

        cairo_move_to (cr, x + width / 2 + 1.1 * radius * cosf (angle),
         base - 1.1 * radius * sinf (angle));
        cairo_line_to (cr, x + width / 2 + 1.15 * radius * cosf (angle),
         base - 1.15 * radius * sinf (angle));
        cairo_stroke (cr);
    }
}

void draw_tuner (GtkWidget * widget, cairo_t * cr, const DetectedTone * tone,
 const DetectedPitch * pitch)
{
    GtkAllocation alloc;
    gtk_widget_get_allocation (widget, & alloc);

    cairo_set_source_rgb (cr, 0, 0, 0);
    cairo_rectangle (cr, 0, 0, alloc.width, alloc.height);
    cairo_fill (cr);

    char tone_str[16], stretch[32], note[16], off_by[16];

    if (pitch->state == DETECT_NONE)
    {
        sprintf (tone_str, "0.00 Hz");
        strcpy (stretch, "");
        strcpy (note, "—");
        strcpy (off_by, "—");
    }
    else
    {
        sprintf (tone_str, "%.02f Hz", tone->tone_hz);
        sprintf (stretch, "harmonics %+.02f", tone->harm_stretch);
        sprintf (note, "%s%d", note_names[pitch->pitch % 12], pitch->pitch / 12);
        sprintf (off_by, "%+.02f", pitch->off_by);
    }

    draw_text (widget, cr, 0, alloc.height / 4, alloc.width / 2, note, "Sans 48");
    draw_text (widget, cr, 0, alloc.height * 3 / 4, alloc.width / 2, tone_str, "Sans 24");
    draw_text (widget, cr, 0, alloc.height * 7 / 8, alloc.width / 2, stretch, "Sans 12");

    draw_dial (widget, cr, alloc.width / 2, 0, alloc.width / 2,
     alloc.height * 3 / 4, pitch->state != DETECT_NONE, pitch->off_by);
    draw_text (widget, cr, alloc.width / 2, alloc.height * 3 / 4,
     alloc.width / 2, off_by, "Sans 24");
}
