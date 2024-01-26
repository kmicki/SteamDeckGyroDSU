# SteamDeckGyroDSU
**DSU** (*cemuhook protocol*) server for motion data for **Steam Deck**.

## Install/Update

Open this page in the browser in **Steam Deck**'s desktop mode.

Download [Update File](https://github.com/kmicki/SteamDeckGyroDSU/releases/latest/download/update-sdgyrodsu.desktop), save it to Desktop and run it by touching or double-clicking *Update GyroDSU*.

This Desktop shortcut may be used also to update the *SteamDeckGyroDSU* to the most recent version.

To uninstall, run *Uninstall GyroDSU* from Desktop by touching or double-clicking.
    
## Usage

Server is running as a service. It provides motion data for cemuhook at Deck's IP address and UDP port *26760*.

Optionally, another UDP server port may be specified in an environment variable **SDGYRO_SERVER_PORT**.

**Remark:** The server provides only motion data. Remaining controls (buttons/axes) are not provided.

### Client (emulator) Configuration

See [Client Configuration](https://github.com/kmicki/SteamDeckGyroDSU/wiki/Client-Configuration) wiki page for instructions on how to configure client applications (emulators).

## Reporting problems

Before reporting problems make sure you are running the most recent version of **SteamDeckGyroDSU** (see *Install/Update* section above).

When reporting a problem or an issue with the server, please generate a log file with following command:

    $HOME/sdgyrodsu/logcurrentrun.sh > sdgyrodsu.log
    
File `sdgyrodsu.log` will be generated in current directory. Attach it to the issue describing the problem.

## Alternative installation

To install the server using a binary package provided in a release, see [wiki page](https://github.com/kmicki/SteamDeckGyroDSU/wiki/Alternative-installation-instructions).

To build the server from source on Deck and install it, see [wiki page](https://github.com/kmicki/SteamDeckGyroDSU/wiki/Build-and-install-from-source).
