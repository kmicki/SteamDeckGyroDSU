# SteamDeckGyroDSU
DSU (cemuhook protocol) server for motion data.

## Quick build and install instructions

To build the server and install it as a service for the first time switch to desktop mode, open terminal and enter following commands in order:

    cd
    git clone https://github.com/kmicki/SteamDeckGyroDSU.git
    cd SteamDeckGyroDSU
    
    ./install.sh
    
If you haven't used **sudo** before, first run `passwd` to set a password.
    
After that, restart the system and the DSU server should be active.

To disable the server, use command:

    systemctl --user disable sdgyrodsu.service
    
To enable it again:

    systemctl --user disable --now sdgyrodsu.service

## Current status

The code runs DSU server that can be used with Cemu (cemuhook). To use it, modify cemuhook.ini with IP address of the Deck (set it in \[Input\] section as _serverIP_).

## Build instructions

The program is for Steam Deck specifically so instructions are for building on Steam Deck.

### Dependencies

Repository depends on libraries that are already installed in the Steam Deck's system. Unfortunately, even though the libraries are there, the header files are not, so they have to be reinstalled.

Those packages are:
- **gcc**
- **glibc**
- **linux-api-headers**
- **ncurses**

To do that:

    sudo steamos-readonly disable
    sudo pacman-key --init
    sudo pacman-key --populate
    sudo pacman -S --noconfirm gcc
    sudo pacman -S --noconfirm glibc
    sudo pacman -S --noconfirm linux-api-headers
    sudo pacman -S --noconfirm ncurses
    sudo steamos-readonly enable
    
As you see above, this requires disabling read only filesystem. 

Alternatively you can use overlayfs, see [SteamDeckPersistentRootFs](https://github.com/Chloe-ko/SteamDeckPersistentRootFs).

Be aware: when overlayfs is enabled Steam will fail to apply updates. When update is available, follow uninstallation instructions in a mentioned repository.

### Build

Use vscode task or just build using following commands in a project's directory:

    mkdir -p bin
    g++ $(find inc -type d -printf '-I %p\n') -g $(find src -type f -iregex '.*\.cpp' -printf '%p\n') -pthread -lncurses -o bin/sdgyrodsu
  
## Usage instructions

If the server was installed using `install.sh` script, it should be running as a service after system restart. Otherwise, see below.
When program is running, the DSU (cemuhook protocol) server providing motion data is available at Deck's IP address and UDP port 26760.

### Grant permissions

Use following commands to create a new user group and add current user to the group and then grant that group permission to read from hiddev file of Steam Deck controls (run those in a repository's main directory):

    sudo groupadd usbaccess
    sudo gpasswd -a $USER usbaccess
    sudo cp pkg/51-deck-controls.rules /etc/udev/rules.d/
    
    Then restart the system.
    
### Install user service

If you want to run server automatically when Steam Deck is ON, install it as a service.

First prepare permissions as described in a previous section (service will be non-root).

Then run those commands from repository directory:

    mkdir -p $HOME/sdgyrodsu
    cp bin/sdgyrodsu $HOME/sdgyrodsu/
    cp pkg/sdgyrodsu.service $HOME/.config/systemd/user/
    systemctl --user enable --now sdgyrodsu.service

### Run without service

You can use the server without installing a user service. Granting permissions is still necessary. Without granting permissions it will work only when run as root.

    ./bin/sdgyrodsu

## TODO

- optimize CPU usage, the program uses 3% of CPU when a client (cemuhook) is connected
- prepare a package for pacman so that building the application (and disabling readonly filesystem) won't be necessary.
- prepare a plugin for gaming mode to enable/disable deamon
