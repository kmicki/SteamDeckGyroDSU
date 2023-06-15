#!/bin/sh

cd "$(dirname "$(readlink -f "$0")")"

echo "Checking 'usbaccess' user group..."
if cat /etc/group | grep -q 'usbaccess'; then
	echo "'usbaccess' group exits."
else
	echo "No 'usbacces' group exists..."
	echo "Creating 'usbacces' group..."
	if sudo groupadd usbaccess >/dev/null; then
		echo "Created 'usbacces' group..."
	else
		echo -e "\e[1mFailed creating 'usbaccess' group.\e[0m"
		exit 20
	fi
fi

echo "Checking if current user is in an 'usbaccess' group..."
if groups $USER | grep -q 'usbaccess'; then
	echo "User is in an 'usbaccess' group."
else
	echo "Adding user to 'usbaccess' group..."
	if sudo gpasswd -a $USER usbaccess >/dev/null; then
		echo "Added user to 'usbaccess' group."
	else
		echo -e "\e[1mFailed adding user to 'usbaccess' group.\e[0m"
		exit 21
	fi
fi

RESTART=false

echo "Checking if there are permissions for 'usbaccess' group in place..."
if test /etc/udev/rules.d/51-deck-controls.rules && cmp -s /etc/udev/rules.d/51-deck-controls.rules 51-deck-controls.rules; then
	echo "Permissions are present."
else
	echo "Removing old permissions if present..."
	sudo rm /etc/udev/rules.d/51-deck-controls.rules >/dev/null 2>&1 
	echo "Adding permissions..."
	if sudo cp 51-deck-controls.rules /etc/udev/rules.d/ >/dev/null; then
		echo "Added permissions."
	else
		echo -e "\e[1mFailed to copy permissions file.\e[0m"
		exit 23
	fi
	echo "Refreshing rules..."
	if sudo udevadm control --reload-rules && sudo udevadm trigger >/dev/null; then
		echo "Rules refreshed."
	else
		echo -e "\e[1mRefreshing rules failed.\e[0m"
		RESTART=true
	fi
fi

echo "Stopping the service if it's running..."
systemctl --user -q stop sdgyrodsu.service >/dev/null 2>&1
systemctl --user -q disable sdgyrodsu.service >/dev/null 2>&1 
echo "Copying binary..."
if mkdir -p $HOME/sdgyrodsu >/dev/null; then
	:
else
	echo -e "\e[1mFailed to create a directory.\e[0m"
	exit 24
fi
rm $HOME/sdgyrodsu/sdgyrodsu >/dev/null 2>&1
if cp sdgyrodsu $HOME/sdgyrodsu/ >/dev/null; then
	:
else
	echo -e "\e[1mFailed to copy binary file.\e[0m"
	exit 25
fi
if chmod +x $HOME/sdgyrodsu/sdgyrodsu >/dev/null; then
	echo "Binary copied."
else
	echo -e "\e[1mFailed to set binary as executable.\e[0m"
	exit 26
fi

rm $HOME/sdgyrodsu/update.sh >/dev/null 2>&1
cp update.sh $HOME/sdgyrodsu/ >/dev/null
if chmod +x $HOME/sdgyrodsu/update.sh >/dev/null; then
	echo "Update script copied."
fi

rm $HOME/sdgyrodsu/uninstall.sh >/dev/null 2>&1
cp uninstall.sh $HOME/sdgyrodsu/ >/dev/null
if chmod +x $HOME/sdgyrodsu/uninstall.sh >/dev/null; then
	echo "Uninstall script copied."
fi

rm $HOME/sdgyrodsu/logcurrentrun.sh >/dev/null 2>&1
cp logcurrentrun.sh $HOME/sdgyrodsu/ >/dev/null
if chmod +x $HOME/sdgyrodsu/logcurrentrun.sh >/dev/null; then
	echo "Log script copied."
fi

echo "Installing service..."
rm $HOME/.config/systemd/user/sdgyrodsu.service >/dev/null 2>&1 
if cp sdgyrodsu.service $HOME/.config/systemd/user/; then
	:
else
	echo -e "\e[1mFailed to copy service file into user systemd location.\e[0m"
	exit 27
fi

groups | grep -q 'usbaccess'

if [ "$?" == 0 ] && [ "$RESTART" == false ]; then
	if systemctl --user -q enable --now sdgyrodsu.service >/dev/null; then
		echo "Installation done."
	else
		echo -e "\e[1mFailed enabling the service.\e[0m"
		exit 28
	fi
else
	if systemctl --user -q enable sdgyrodsu.service >/dev/null; then
		echo "Installation done."
		echo -e "\e[1mDeck has to be restarted for server to run.\e[0m"
		exit 0
	else
		echo -e "\e[1mFailed enabling the service.\e[0m"
		exit 28
	fi
fi

