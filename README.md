# SteamDeckGyroDSU
DSU (cemuhook protocol) server for motion data.

## Install/Update

In Steam Deck's desktop mode, open Konsole.

First-time install will require superuser access to set USB permissions. Therefore it will ask to input deck user's password.

If you've never set the password on your Deck, use `passwd` instruction and set it. Be aware: Konsole will not show the password you're typing in as a security measure.

When deck user has a password set, execute command:

    bash <(curl -sL https://github.com/kmicki/SteamDeckGyroDSU/raw/master/pkg/update.sh)
    
Above command will download the binary package and install it.

In case of first-time install, system restart is required. The install script will inform about that.

To uninstall:

    bash <(curl -sL https://github.com/kmicki/SteamDeckGyroDSU/raw/master/pkg/uninstall.sh)
    
## Usage

Server is running as a service. It provides motion data for cemuhook at Deck's IP address and UDP port 26760.

**Remark:** The server provides only motion data. Remaining controls (buttons/axes) are not provided.

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

## Alternative installation

To install the server using a binary package provided in a release, see [wiki page](https://github.com/kmicki/SteamDeckGyroDSU/wiki/Alternative-installation-instructions).

To build the server from source on Deck and install it, see [wiki page](https://github.com/kmicki/SteamDeckGyroDSU/wiki/Build-and-install-from-source).
