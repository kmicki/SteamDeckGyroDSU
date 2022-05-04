# SteamDeckGyroDSU
Exploring ways to extract gyro data of Steam Deck without blocking the controller from other apps (Steam), and eventually setting up a DSU (cemuhook) server.

## Current status

Right now the code is reading values from Steam Deck controls and displays some of them in the console. **dev** branch contains the code.

#### Values displayed 

- **INC** : HID frame increment
  - the value increments by _1_ for every new data packet from HID device, i.e. every _4ms_
- **SPAN** : current difference between increments of consecutive acquired frames
- **MAX** : maximum difference between increments of consecutive acquired frames since start of program

- **A_RL** : raw acceleration value, direction right to left of Steam Deck
- **A_TB** : raw acceleration value, direction top to bottom of Steam Deck
- **A_FB** : raw acceleration value, direction front to back of Steam Deck

Raw acceleration values around _16500_ seems to equal 1g.

- **G_RL** : gyro value (angular velocity) around axis right to left of Steam Deck
  - positive value when topside of the Deck is pulled and bottomside of the Deck is pushed
- **G_TB** : gyro value (angular velocity) around axis top to bottom of Steam Deck
  - positive value when leftside of the Deck is pulled and rightside of the Deck is pushed
- **G_FB** : gyro value (angular velocity) around axis fron to back of Steam Deck
  - positive value when leftside of the Deck is moved down and rightside of the Deck is moved up

Raw gyro values around _16 (0x10)_ seem to be _1 deg/sec_ (not certain though).

Four values unknown right now but related to motion for sure (**U1**,**U2**,**U3**,**U4**). Looks like they are absolute angles around absolute axes (not relative to Steam Deck but relative to gravity maybe?).

For cemuhook-protocol (DSU) only acceleration and gyro values are needed. Status of remaining controls may also be used. To read them, see descriptions in _sdhidframe.h_.

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

### Non-root usage
  
If you don't want to give superuser permissions to the application, it is necessary to grant permissions to the user which runs the application.

Use following commands to create a new user group and add current user to the group and then grant that group permission to read from hiddev file of Steam Deck controls (run those in a repository's main directory):

    sudo group add usbaccess
    sudo gpasswd -a $USER usbaccess
    sudo cp pkg/51-deck-controls.rules /etc/udev/rules.d/
    
Then restart the system and you can use the program without **sudo**:

    ./bin/sdgyrodsu
    
- HID frame increment (INC)
  - the value increments by 1 for every new data packet from HID device, i.e. every 4ms
- current difference between increments of consecutive acquired frames (SPAN)
- maximum difference between increments of consecutive acquired frames since start of program (MAX)

- **A_RL** : raw acceleration value, direction right to left of Steam Deck
- **A_TB** : raw acceleration value, direction top to bottom of Steam Deck
- **A_FB** : raw acceleration value, direction front to back of Steam Deck

Raw acceleration values around _16500_ seems to equal 1g.

- **G_RL** : gyro value (angular velocity) around axis right to left of Steam Deck
  - positive value when topside of the Deck is pulled and bottomside of the Deck is pushed
- **G_TB** : gyro value (angular velocity) around axis top to bottom of Steam Deck
  - positive value when leftside of the Deck is pulled and rightside of the Deck is pushed
- **G_FB** : gyro value (angular velocity) around axis fron to back of Steam Deck
  - positive value when leftside of the Deck is moved down and rightside of the Deck is moved up

Raw gyro values around _16 (0x10)_ seem to be _1 deg/sec_ (not certain though).

Four values unknown right now but related to motion for sure (**U1**,**U2**,**U3**,**U4**). Looks like they are absolute angles around absolute axes (not relative to Steam Deck but relative to gravity maybe?).

For cemuhook-protocol (DSU) only acceleration and gyro values are needed. Status of remaining controls may also be used. To read them, see descriptions in _sdhidframe.h_.
