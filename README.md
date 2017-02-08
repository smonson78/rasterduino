# rasterduino

I don't know that I'm going to call it *rasterduino*, it's just a working title.

*Rasterduino* is an Arduino firmware for performing raster engraving on a CNC laser. I wrote it to use with my Full Spectrum 40W Hobby Laser (resold Chinese K40 machine) to replace the very limited non-upgradeable RetinaEngrave controller that it came with.

#### *Rasterduino* is:
* Compatible with grbl 1.1 pinout (in laser mode)
* Smooth - accelerates into and out of each movement
* Able to vary the laser power as an 8 bit quantity per pixel
* Open-source
* Compatible with source images up to 1500 pixels wide (for now; I will remove this limitation in the future)

*Rasterduino* is very rough, but it does basically work, and I have plans to make it better in the future.

### Important Disclaimer
Using lasers is extremely dangerous, blah blah blah, you know how this works. This software isn't bullet-proof - if you download *rasterduino* and then laser-beam yourself in the face because it turned on when you weren't wearing goggles, then you will have yourself to blame. There are no promises, guarantees or implicit safety assurances with this thing. Unless you understand what you're doing, **do not download anything from this page**.

### How to use

#### Building

1. Get a local copy from the [releases page|http://github.com/smonson78/rasterduino/releases/tag/v0.6] or straight from github with `
git clone https://github.com/smonson78/rasterduino.git`
2. Install the prerequisites (these are the Ubuntu package names):
    * libfreeimage-dev
    * libserialport-dev
    * avr-libc
    * avrdude
3. Run `make` and then `make flash` to install the firmware onto the target board (it must be connected, duh). You might have to edit `Makefile` to select the right port if yours isn't `/dev/ttyUSB0`. Probably a good idea **not** to have the laser connected to it and powered up while you're doing this.
4. Build the image-sending utility in the `sender` directory: change to that directory and run `make` there to build the image-sending utility, which is called `raster`.

#### Lasering

You should prepare your source image so that one horizontal scanline equals one laser scanline. Horizontal length should be less than 1500 pixels. There's no limit on vertical size. If you use colour, it will be converted to greyscale before sending to the laser. The darker the colour, the higher the laser intensity, so if you're engraving on a thing where more burning equals a lighter colour, such as clear acrylic or anodised aluminium, you should invert the colour of the picture.

Then begin blasting laser beams at your chosen object:
```
./raster -r ramp-distance -v velocity -s scanline-separation-distance -w final-width image-filename.png
```
Each switch takes a numerical argument:

*ramp-distance* (-r): number of steps before and after each lasered scanline reserved for speeding up and slowing down. How many you need is determined by how fast you want to go.

*velocity* (-v): velocity of the laser process, given as x/2,000,000th of a second between each stepper motor increment (sorry about that). For my laser, a velocity of 400 is pretty decent. If I go below about 350 then it starts missing steps.

*scanline-separation-distance* (-s): simply the number of steps in between each scanline (I use 5, giving me a density of 200 lines per inch).

*final-width* (-w): the number of steps each scanline will be. This is separate from the image's width in pixels - it will be scaled to the size given.

It'll print out a bunch of crap; it's just for debugging.

As of the current version, if there are any dropped characters when sending data over the serial port, the program will probably just freeze up and ruin whatever you're drawing. So right now don't go engraving any priceless Ming vases or irreplaceable heirlooms.









