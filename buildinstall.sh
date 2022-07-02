./build.sh

sudo groupadd usbaccess
sudo gpasswd -a $USER usbaccess
sudo cp pkg/51-deck-controls.rules /etc/udev/rules.d/

mkdir -p $HOME/sdgyrodsu
cp bin/sdgyrodsu $HOME/sdgyrodsu/
cp pkg/sdgyrodsu.service $HOME/.config/systemd/user/
systemctl --user enable --now sdgyrodsu.service

