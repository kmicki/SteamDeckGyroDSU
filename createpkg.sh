sudo steamos-readonly disable
sudo pacman-key --init
sudo pacman-key --populate
sudo pacman -S --noconfirm gcc
sudo pacman -S --noconfirm glibc
sudo pacman -S --noconfirm linux-api-headers
sudo pacman -S --noconfirm ncurses
sudo steamos-readonly enable

mkdir -p bin

rm bin/*
cp pkg/* bin/

g++ $(find inc -type d -printf '-I %p\n') -g $(find src -type f -iregex '.*\.cpp' -printf '%p\n') -pthread -lncurses -o bin/sdgyrodsu

mkdir -p pkgbin/
rm pkgbin/*
mkdir -p pkgbin/SteamDeckGyroDSUSetup
cp bin/* pkgbin/SteamDeckGyroDSUSetup/

cd pkgbin
zip -r SteamDeckGyroDSUSetup.zip SteamDeckGyroDSUSetup
rm -r SteamDeckGyroDSUSetup