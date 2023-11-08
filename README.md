# SteamDeckGyroDSU
**DSU** (*cemuhook protocol*) server for motion data for **Steam Deck**.

## Install/Update

In **Steam Deck**'s desktop mode, open *Konsole*.

Execute command:

    bash <(curl -sL https://raw.githubusercontent.com/kmicki/SteamDeckGyroDSU/master/pkg/update.sh)
    
Above command will download the binary package and install it. It can be also used to update to the newest version at any time.

When **SteamDeckGyroDSU** is already installed, it can also be updated by running following command:

    $HOME/sdgyrodsu/update.sh

To uninstall:

    $HOME/sdgyrodsu/uninstall.sh

### Remarks about update from version 1.*

Versions **1.\*** of **SteamDeckGyroDSU** required superuser access to be installed.

Versions **2.0** and above don't need it.

Still, after update from **1.\*** to **2.0** or higher, the changes done with superuser access will be left in the system.
Uninstall script will not remove them anymore since version **2.0**.

Reverting those changes is not necessary but can be done by executing following command (requires superuser access):

    $HOME/sdgyrodsu/v1cleanup.sh

after version **2.0** or above is installed.
    
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
