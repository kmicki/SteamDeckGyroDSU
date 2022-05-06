echo "Checking user group."
if cat /etc/group | grep -q 'usbaccess'; then
	echo "usbaccess group exits."
else
	echo "Creating usbacces group."
	sudo groupadd usbaccess
fi

echo "Checking if current user is in an 'usbaccess' group."
if groups $USER | grep -q 'usbaccess'; then
	echo "User is in an 'usbaccess' group."
else
	echo "Adding user to 'usbaccess' group."
	sudo gpasswd -a $USER usbaccess
fi

echo "Checking if there are permissions for 'usbaccess' group in place."
if test /etc/udev/rules.d/51-deck-controls.rules && cmp -s /etc/udev/rules.d/51-deck-controls.rules 51-deck-controls.rules; then
	echo "Permissions are present."
else
	echo "Removing old permissions if present."
	sudo rm /etc/udev/rules.d/51-deck-controls.rules
	echo "Adding permissions."
	sudo cp 51-deck-controls.rules /etc/udev/rules.d/
	echo "Refreshing rules."
	sudo udevadm control --reload-rules && sudo udevadm trigger
fi

echo "Stopping the service if it's running"
systemctl --user stop sdgyrodsu.service
systemctl --user disable sdgyrodsu.service
echo "Copying binary."
mkdir -p $HOME/sdgyrodsu
rm $HOME/sdgyrodsu/sdgyrodsu
cp sdgyrodsu $HOME/sdgyrodsu/
chmod +x $HOME/sdgyrodsu/sdgyrodsu
echo "Installing service"
rm $HOME/.config/systemd/user/sdgyrodsu.service
cp sdgyrodsu.service $HOME/.config/systemd/user/
if groups | grep -q 'usbaccess'; then
	systemctl --user enable --now sdgyrodsu.service
	echo "Installation done."
else
	systemctl --user enable sdgyrodsu.service
	echo "Installation done. Deck has to be restarted for server to run."
fi