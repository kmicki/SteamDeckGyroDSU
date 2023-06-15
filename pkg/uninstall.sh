#!/bin/sh

echo "Uninstalling the service"
systemctl --user -q stop sdgyrodsu.service >/dev/null 2>&1
systemctl --user -q disable sdgyrodsu.service >/dev/null 2>&1
rm $HOME/.config/systemd/user/sdgyrodsu.service >/dev/null 2>&1

echo "Removing files"
rm $HOME/sdgyrodsu/sdgyrodsu >/dev/null 2>&1
rm $HOME/sdgyrodsu/update.sh >/dev/null 2>&1
rm $HOME/sdgyrodsu/uninstall.sh >/dev/null 2>&1
rm $HOME/sdgyrodsu/logcurrentrun.sh >/dev/null 2>&1
rm -d $HOME/sdgyrodsu >/dev/null 2>&1

echo "Removing USB permissions"
sudo rm /etc/udev/rules.d/51-deck-controls.rules >/dev/null 2>&1
echo "Refreshing rules"
sudo udevadm control --reload-rules && sudo udevadm trigger >/dev/null 2>&1

echo "Removing user from 'usbaccess' group"
sudo gpasswd -d $USER usbaccess >/dev/null 2>&1

echo "Removing 'usbaccess' group"
sudo groupdel usbaccess >/dev/null 2>&1

echo "Uninstalling complete."

