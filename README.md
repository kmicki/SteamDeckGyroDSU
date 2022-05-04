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
