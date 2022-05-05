ssteamos-readonly disable
pacman-key --init
pacman-key --populate
pacman -S --noconfirm gcc
pacman -S --noconfirm glibc
pacman -S --noconfirm linux-api-headers
pacman -S --noconfirm ncurses
steamos-readonly enable

mkdir -p bin
g++ $(find inc -type d -printf '-I %p\n') -g $(find src -type f -iregex '.*\.cpp' -printf '%p\n') -pthread -lncurses -o bin/sdgyrodsu

group add usbaccess
gpasswd -a $USER usbaccess
cp pkg/51-deck-controls.rules /etc/udev/rules.d/

mkdir -p $HOME/sdgyrodsu
cp bin/sdgyrodsu $HOME/sdgyrodsu/
cp pkg/sdgyrodsu.service $HOME/.config/systemd/user/
systemctl --user enable sdgyrodsu.service