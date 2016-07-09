SRCS=draw.c fft.c io.c jtuner.c pitch.c tone.c
FLAGS=-std=gnu99 -Wall -O2 -ffast-math
LIBS=-lm -lasound `pkg-config --cflags --libs gtk+-3.0`

jtuner : ${SRCS}
	gcc ${FLAGS} ${SRCS} ${LIBS} -o jtuner

install :
	cp jtuner /usr/bin
	chmod 0755 /usr/bin/jtuner
	cp jtuner.png /usr/share/icons/hicolor/16x16/apps
	chmod 0644 /usr/share/icons/hicolor/16x16/apps/jtuner.png
	cp jtuner.svg /usr/share/icons/hicolor/scalable/apps
	chmod 0644 /usr/share/icons/hicolor/scalable/apps/jtuner.svg
	gtk-update-icon-cache /usr/share/icons/hicolor
	cp jtuner.desktop /usr/share/applications
	chmod 0644 /usr/share/applications/jtuner.desktop
	update-desktop-database

uninstall :
	rm -f /usr/bin/jtuner
	rm -f /usr/share/icons/hicolor/16x16/apps/jtuner.png
	rm -f /usr/share/icons/hicolor/scalable/apps/jtuner.svg
	gtk-update-icon-cache /usr/share/icons/hicolor
	rm -f /usr/share/applications/jtuner.desktop
	update-desktop-database

clean :
	rm -f jtuner
