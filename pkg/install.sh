#!/bin/sh

cd "$(dirname "$(readlink -f "$0")")"

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

rm $HOME/sdgyrodsu/v1cleanup.sh >/dev/null 2>&1
cp v1cleanup.sh $HOME/sdgyrodsu/ >/dev/null
if chmod +x $HOME/sdgyrodsu/v1cleanup.sh >/dev/null; then
	echo "V1-Cleanup script copied."
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

if systemctl --user -q enable --now sdgyrodsu.service >/dev/null; then
	echo "Installation done."
else
	echo -e "\e[1mFailed enabling the service.\e[0m"
	exit 28
fi