SRCS=draw.c fft.c io.c jtuner.c pitch.c tone.c
OFFLINE_SRCS=fft.c jtuner-offline.c pitch.c tone.c
FLAGS=-std=gnu99 -Wall -O2 -g -ffast-math
LIBS=-lm -lasound `pkg-config --cflags --libs gtk+-2.0`

all : jtuner jtuner-offline

jtuner : ${SRCS}
	gcc ${FLAGS} ${SRCS} ${LIBS} -o jtuner

jtuner-offline : ${OFFLINE_SRCS}
	gcc ${FLAGS} ${OFFLINE_SRCS} -lm -o jtuner-offline

install :
	mkdir -p $(DESTDIR)/usr/bin
	cp jtuner jtuner-offline $(DESTDIR)/usr/bin
	chmod 0755 $(DESTDIR)/usr/bin/jtuner $(DESTDIR)/usr/bin/jtuner-offline
	mkdir -p $(DESTDIR)/usr/share/icons/hicolor/16x16/apps
	cp jtuner.png $(DESTDIR)/usr/share/icons/hicolor/16x16/apps
	chmod 0644 $(DESTDIR)/usr/share/icons/hicolor/16x16/apps/jtuner.png
	mkdir -p $(DESTDIR)/usr/share/icons/hicolor/scalable/apps
	cp jtuner.svg $(DESTDIR)/usr/share/icons/hicolor/scalable/apps
	chmod 0644 $(DESTDIR)/usr/share/icons/hicolor/scalable/apps/jtuner.svg
	mkdir -p $(DESTDIR)/usr/share/applications
	cp jtuner.desktop $(DESTDIR)/usr/share/applications
	chmod 0644 $(DESTDIR)/usr/share/applications/jtuner.desktop

uninstall :
	rm -f $(DESTDIR)/usr/bin/jtuner $(DESTDIR)/usr/bin/jtuner-offline
	rm -f $(DESTDIR)/usr/share/icons/hicolor/16x16/apps/jtuner.png
	rm -f $(DESTDIR)/usr/share/icons/hicolor/scalable/apps/jtuner.svg
	rm -f $(DESTDIR)/usr/share/applications/jtuner.desktop

clean :
	rm -f jtuner jtuner-offline
