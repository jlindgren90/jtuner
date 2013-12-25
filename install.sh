#!/bin/sh

cp jtuner /usr/bin
chmod 0755 /usr/bin/reflect
cp jtuner.png /usr/share/icons/hicolor/16x16/apps
chmod 0644 /usr/share/icons/hicolor/16x16/apps/jtuner.png
cp jtuner.svg /usr/share/icons/hicolor/scalable/apps
chmod 0644 /usr/share/icons/hicolor/scalable/apps/jtuner.svg
gtk-update-icon-cache /usr/share/icons/hicolor
cp jtuner.desktop /usr/share/applications
chmod 0644 /usr/share/applications/jtuner.desktop
update-desktop-database
