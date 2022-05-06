sudo steamos-readonly disable
sudo pacman-key --init
sudo pacman-key --populate
sudo pacman -S --noconfirm gcc
sudo pacman -S --noconfirm glibc
sudo pacman -S --noconfirm linux-api-headers
sudo pacman -S --noconfirm ncurses
sudo steamos-readonly enable

mkdir -p bin
g++ $(find inc -type d -printf '-I %p\n') -g $(find src -type f -iregex '.*\.cpp' -printf '%p\n') -pthread -lncurses -o bin/sdgyrodsu

systemctl --user stop sdgyrodsu.service
rm $HOME/sdgyrodsu/sdgyrodsu
cp bin/sdgyrodsu $HOME/sdgyrodsu/
systemctl --user enable --now sdgyrodsu.service