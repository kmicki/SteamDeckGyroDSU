# SteamDeckGyroDSU
**DSU** (*cemuhook protocol*) server for motion data for **Steam Deck**.

Also works on other handhelds running **SteamOS** whose motion sensor is exposed by the kernel as an IIO device (e.g. **ROG Xbox Ally X** with `bmi323-imu`). See [Other devices](#other-devices).

## Install/Update

Open this page in the browser in **Steam Deck**'s desktop mode.

Download [Installation File](https://github.com/kmicki/SteamDeckGyroDSU/releases/latest/download/update-sdgyrodsu.desktop), save it to Desktop and run it by touching or double-clicking *Update GyroDSU*.

This Desktop shortcut may be used also to update the *SteamDeckGyroDSU* to the most recent version.

To uninstall, run *Uninstall GyroDSU* from Desktop by touching or double-clicking.
    
## Usage

Server is running as a service. It provides motion data for cemuhook at Deck's IP address and UDP port *26760*.

Optionally, another UDP server port may be specified in an environment variable **SDGYRO_SERVER_PORT**.

**Remark:** The server provides only motion data. Remaining controls (buttons/axes) are not provided.

### Other devices

When the Steam Deck's controller is not found, the server reads the accelerometer and gyroscope from the first IIO device in `/sys/bus/iio/devices` that provides both (no root access needed). Log line at startup shows which source is used:

    journalctl --user -u sdgyrodsu | grep "Using"

Mapping of sensor axes was verified on **ROG Xbox Ally X**. On other devices, if motion axes are inverted or swapped, the mapping can be overridden with environment variables in the service (`systemctl --user edit sdgyrodsu`):

    [Service]
    Environment=SDGYRO_IIO_ACCEL_MAP=x,-y,-z
    Environment=SDGYRO_IIO_GYRO_MAP=x,-y,-z

Each value lists sensor axes (optionally negated) for the Steam Deck frame axes, which point to: the right side, the top edge and out of the screen. Accelerometer and gyroscope should use the same mapping, with an even number of negations; otherwise motion drifts in clients that combine both sensors.

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
