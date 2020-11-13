# SAM

<img src="EMF2014/Dog/noun_Dog_79221.svg" width=64 alt="logo">

SAM is a Simple Abstract Machine intended as a teaching tool and toy.

Sam is a Stack–Array Machine. It has no memory or registers; instead it has
a nested stack of stacks. Each stack is an array of items that can be
executed.

SAM was designed for the TiLDA Mke badge (whose hardware seems to be codenamed “sam”), an Arduino Due-compatible device designed for Electromagnetic Field 2014. It can also be run on a GNU-compatible system with SDL2.

SAM was invented as a tenth birthday present for my nephew, Sam. It was named after my first computer, a Sinclair ZX81, “Super Advanced Micro”. Sam’s logo is a simpatico astute mongrel called Sam.


## The code

The SAM-specific files are in `Mk2-Firmware/EMF2014`. SAM itself is in the source files `sam*`. The logo files are in the directory `Dog`. (Some of the C files end in `.x` so they are not automatically built by the Arduino IDE.)


## Building SAM

To build on a GNU-compatible system you will need GCC, Python (for the `samc` assembler) and SDL2 (on Ubuntu-compatible systems, install package `libsdl2-gfx-dev:i386`). Run the script `mksam` with the desired code, and then run the resulting `sam` binary; for example:

```
./mksam graphics.yaml
./sam
```

Debugging output is shown in the terminal. Press Return in the terminal to
close the window and end the program.

To install on a TiLDA, use Arduino IDE v1.5.7 (versions >= 1.6.0 do not work), and follow the [EMFCamp instructions](https://wiki-archive.emfcamp.org/2014/w/index.php/TiLDA_MKe#Set_up_your_environment) to set up your environment, build `sam` as above to set the program, then compile and upload the sketch as normal.

The original TiLDA `README` text follows.

---

TiLDA v2 Firmware
=================

#### Firmware on the TiLDA v2 Badge
* To use this code you need Arduino IDE version 1.5.7 or later which can be downloaded [here](http://arduino.cc/en/Main/Software#toc3).
* The easiest way to work with this repo is to set it as the sketchbook folder in the IDE's File->Preferences menu (restart the IDE after changing this setting).
* This way you will not have any conflicts with your existing sketchbook and library folders.
* N.B. Arduino 1.5.7 stores its preferences in a separate location to that of the Arduino 1.0.x IDE).
* If you're using git or your own editor, you'll need to restart the IDE after touching any file that's open. To avoid this, go to File->Preferences, and tick "Use external editor"


* The TiLDA code base is split in 3 distinct sections:
  * The custom TiLDA board definition is kept in **hardware/emfcamp/sam/**.
  * The custom TiLDA hardware libraries are kept in **hardware/emfcamp/sam/libraries** and will only be used if a TiLDA board is selected from the _Tools->Board_ menu.
  * Third is the main firmware sketch which is kept in **EMF2014/**, this has the _.ino_, _.c[pp]_ and _.h_ files for each task.
  * There may also be other examples or test sketches either in their own directory or as examples in TiLDA library folders.


#### hardware/
* This folder contains _TiLDA Mk2_ board definition for use with the Arduino IDE (version -> 1.5.7).
* To use: copy the **emfcamp** folder to your **~/sketchbook/hardware/** folder.
* You can then select _TiLDA Mk2_ from the _Tools->Board_ menu.
* To use the **FTDI** upload port you will need to manually erase the board before uploading a new sketch (short the **Erase** pins for 1 second).

#### EMF2014/
* This is the main sketch for the EMF 2014 firmware.
* It is complied using the **TiLDA Mk2** board form the the _Tools->Board_ menu.
* void setup() and void loop() can be found in EMF2014.ino
* There are several _.c[pp]_ and _.h_ files that make up the _FreeRTOS_ tasks used by the main app.


#### frRGBTask/
* This is the first fully functional test of the RGB Task.
* Some of the buttons are set up to test the different RGB modes.


#### Contributing
* If you wish to work on code please fork our repo, do your work in a branch and submit a pull request for review
* Please do not commit to master
* For discussing changes, please point your IRC client at #tilda on Freenode

