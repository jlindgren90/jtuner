#!/bin/sh

SRCS="draw.c fft.c io.c jtuner.c pitch.c tone.c"
FLAGS="-std=gnu99 -Wall -O2 -ffast-math"
LIBS="-lm -lasound `pkg-config --cflags --libs gtk+-3.0`"

gcc ${FLAGS} ${SRCS} ${LIBS} -o jtuner
