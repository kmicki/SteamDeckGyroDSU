#!/bin/sh

echo "Grabbing latest release..."
if curl -L -O -s https://github.com/kmicki/SteamDeckGyroDSU/releases/latest/download/SteamDeckGyroDSUSetup.zip >/dev/null; then
	echo "Latest release downloaded."
else
	echo -e "\e[1mFailed to grab latest .zip file...\e[0m"
	exit 10
fi
echo "Extracting files..."
if unzip -o SteamDeckGyroDSUSetup.zip -d $HOME >/dev/null; then
	echo "Files extracted."
else
	echo -e "\e[1mFailed to extract files from downloaded .zip release...\e[0m"
	exit 11
fi

rm -f SteamDeckGyroDSUSetup.zip

cd $HOME/SteamDeckGyroDSUSetup
echo "Running install script..."
echo " "
./install.sh
code=$?

cd
rm -rf $HOME/SteamDeckGyroDSUSetup

exit $code
