# SteamDeckGyroDSU
DSU (cemuhook protocol) server for motion data for Steam Deck.

## Install/Update

In Steam Deck's desktop mode, open Konsole.

First-time install will require superuser access to set USB permissions. Therefore it will ask to input deck user's password.

If you've never set the password on your Deck, use `passwd` instruction and set it. Be aware: Konsole will not show the password you're typing in as a security measure.

When deck user has a password set, execute command:

    bash <(curl -sL https://raw.githubusercontent.com/kmicki/SteamDeckGyroDSU/master/pkg/update.sh)
    
Above command will download the binary package and install it. It can be also used to update to the newest version at any time.

When **SteamDeckGyroDSU** is already installed, it can also be updated by running following command:

    $HOME/sdgyrodsu/update.sh

In case of first-time install, system restart is required. The install script will inform about that.

To uninstall:

    $HOME/sdgyrodsu/uninstall.sh
    
## Usage

Server is running as a service. It provides motion data for cemuhook at Deck's IP address and UDP port 26760.

**Remark:** The server provides only motion data. Remaining controls (buttons/axes) are not provided.

### Client Configuration

See [Client Configuration](https://github.com/kmicki/SteamDeckGyroDSU/wiki/Client-Configuration) wiki page.

## Reporting problems

Before reporting problems make sure you are running the most recent version of **SteamDeckGyroDSU** (see *Install/Update* section above).

When reporting a problem or an issue with the server, please generate a log file with following command:

    $HOME/sdgyrodsu/logcurrentrun.sh > sdgyrodsu.log
    
File `sdgyrodsu.log` will be generated in current directory. Attach it to the issue describing the problem.

## Alternative installation

To install the server using a binary package provided in a release, see [wiki page](https://github.com/kmicki/SteamDeckGyroDSU/wiki/Alternative-installation-instructions).

To build the server from source on Deck and install it, see [wiki page](https://github.com/kmicki/SteamDeckGyroDSU/wiki/Build-and-install-from-source).
