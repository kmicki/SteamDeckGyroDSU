#!/bin/sh

echo "Cleaning up after SteamDeckGyroDSU v1.*"

echo "Removing deprecated USB permissions"
sudo rm /etc/udev/rules.d/51-deck-controls.rules >/dev/null 2>&1
echo "Refreshing rules"
sudo udevadm control --reload-rules && sudo udevadm trigger >/dev/null 2>&1

echo "Removing user from 'usbaccess' group"
sudo gpasswd -d $USER usbaccess >/dev/null 2>&1

echo "Removing 'usbaccess' group"
sudo groupdel usbaccess >/dev/null 2>&1

echo "Cleanup complete."

