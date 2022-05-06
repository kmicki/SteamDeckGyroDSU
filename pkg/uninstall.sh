echo "Uninstalling the service"
systemctl --user stop sdgyrodsu.service
systemctl --user disable sdgyrodsu.service
rm $HOME/.config/systemd/user/sdgyrodsu.service

echo "Removing binary"
rm $HOME/sdgyrodsu/sdgyrodsu
rm -d $HOME/sdgyrodsu

echo "Removing USB permissions"
sudo rm /etc/udev/rules.d/51-deck-controls.rules
echo "Refreshing rules"
sudo udevadm control --reload-rules && sudo udevadm trigger

echo "Removing user from 'usbaccess' group"
sudo gpasswd -d $USER usbaccess

echo "Removing 'usbaccess' group"
sudo groupdel usbaccess

echo "Uninstalling complete."