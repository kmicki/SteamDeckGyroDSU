@echo off
echo "Grabbing latest .zip file..."
curl -L -O https://github.com/kmicki/SteamDeckGyroDSU/releases/latest/download/SteamDeckGyroDSUSetup.zip
unzip -o SteamDeckGyroDSUSetup.zip -d /home/deck
echo "Stopping service..."
systemctl --user stop sdgyrodsu.service
systemctl --user disable sdgyrodsu.service
echo "Copying files..."
rm $HOME/sdgyrodsu/sdgyrodsu
cp sdgyrodsu $HOME/sdgyrodsu/
chmod +x $HOME/sdgyrodsu/sdgyrodsu
echo "Installing service..."
rm $HOME/.config/systemd/user/sdgyrodsu.service
cp sdgyrodsu.service $HOME/.config/systemd/user/
systemctl --user enable --now sdgyrodsu.service
echo "Installation done."
systemctl --user status sdgyrodsu.service
