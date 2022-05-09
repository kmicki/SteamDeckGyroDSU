# SteamDeckGyroDSU
DSU (cemuhook protocol) server for motion data.

## Quick installation/update

In Steam Deck's desktop mode, open Konsole and execute command:

    bash <(curl -s https://raw.githubusercontent.com/kmicki/SteamDeckGyroDSU/master/pkg/update.sh)
    
Above command will download the binary package and install it.

## Alternative installation instructions

Download the SteamDeckGyroDSUSetup.zip from the most recent release. Unzip and run install script.

    cd
    wget https://github.com/kmicki/SteamDeckGyroDSU/releases/latest/download/SteamDeckGyroDSUSetup.zip
    unzip -o SteamDeckGyroDSUSetup.zip
    cd SteamDeckGyroDSUSetup
    ./install.sh
    
Package contains update script that downloads the most recent release from GitHub and installs it.

    ./update.sh
    
System restart is necessary in case of first install. Script will inform about that.
    
### Uninstall

The SteamDeckGyroDSUSetup.zip contains also uninstall script.

    ./uninstall.sh
    
## Usage

Server is running as a service. It provides motion data for cemuhook at Deck's IP address and UDP port 26760.

### Configuring Cemu

1. Download [Cemu](https://cemu.info/) and extract files.
2. Download [cemuhook](https://cemuhook.sshnuke.net/) and extract files to Cemu folder.
3. Run Cemu at least once.
4. If the server and Cemu are both running on Deck, the motion source should be selectable in Options -> Gamepad Motion Source -> DSU1 -> By Slot.
5. Make sure that in _Input settings_ in Cemu _WiiU Gamepad_ is selected as an emulated controller.
6. If Cemu is running on a separate PC, open cemuhook.ini file and insert IP of the Deck under \[Input\] section as _serverIP_ similar to below:
<pre>
[Graphics]
ignorePrecompiledShaderCache = false
[CPU]
customTimerMode = QPC
customTimerMultiplier = 1
[Input]
motionSource = DSU1
<b>serverIP = X.X.X.X</b>
[Debug]
mmTimerAccuracy = 1ms
</pre>
where **X.X.X.X** is Deck's IP.

## Build and install from source

To build the server from source on Deck and install it, see [wiki page](https://github.com/kmicki/SteamDeckGyroDSU/wiki/Build-and-install-from-source).
