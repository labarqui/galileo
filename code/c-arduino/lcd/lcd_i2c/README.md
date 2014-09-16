## Arduino LCD I2C example

Steps to install it:

1. Replace the LiquidCrystal library with the one on libraries/LiquidCrystal, based on [this one](https://bitbucket.org/fmalpartida/new-liquidcrystal/wiki/Home). This library is located in `~/arduino-1.5.3/libraries`
2. Add the LiquidCrystalI2C library in the same folder pointed above.
3. Open the sketch in arduino and run or run `make` and `sudo make flash`

## Utils

[i2c-tools repo](https://github.com/groeck/i2c-tools)
[i2c-tools docs](http://www.lm-sensors.org/wiki/i2cToolsDocumentation)
Probing a range of addresses on /dev/i2c-0:	`i2cdetect -r 0`
[Interfacing with I2C devices on linux](http://elinux.org/Interfacing_with_I2C_Devices)

