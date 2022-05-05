# SteamDeckGyroDSU
DSU (cemuhook protocol) server for motion data.

## Current status

The code runs DSU server that can be used with Cemu (cemuhook). To use it, modify cemuhook.ini with IP address of the Deck (set it in \[Input\] section as _serverIP_).

 **dev** branch contains the code.

## Build instructions

The program is for Steam Deck specifically so instructions are for building on Steam Deck.

Use vscode task or just build using following commands in a project's directory:

    mkdir -p bin
    g++ $(find inc -type d -printf '-I %p\n') -g $(find src -type f -iregex '.*\.cpp' -printf '%p\n') -pthread -lncurses -o bin/sdgyrodsu
    
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
  
## Usage instructions

Run:

    sudo ./bin/sdgyrodsu

When program is running, the DSU (cemuhook protocol) server providing motion data is available at Deck's IP address and UDP port 26760.

### Non-root usage
  
If you don't want to give superuser permissions to the application, it is necessary to grant specific permissions to the user which runs the application.

Use following commands to create a new user group and add current user to the group and then grant that group permission to read from hiddev file of Steam Deck controls (run those in a repository's main directory):

    sudo group add usbaccess
    sudo gpasswd -a $USER usbaccess
    sudo cp pkg/51-deck-controls.rules /etc/udev/rules.d/
    
Then restart the system and you can use the program without **sudo**:

    ./bin/sdgyrodsu

## TODO

- prepare a daemon configuration
- prepare a package for pacman so that building the application (and disabling readonly filesystem) won't be necessary.
- prepare a plugin for gaming mode to enable/disable deamon