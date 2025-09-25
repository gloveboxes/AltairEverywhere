# SenseHat driver
Basic driver for the sense-hat written in C using i2c.

Currently the driver can access the HTS221 temperature/humidity sensor and the LPS25H pressure sensor.

## Installing and Compiling
Installing the library
```
git clone https://github.com/mnk400/sensehat-driver
make
```
To build with the library you can use you can include ```sense_hat.h``` in your program and then link with the ```-lsense``` flag after installing.

Note: You need to have i2c enabled on the hardware for the compiled software to work, you might also need superuser access.
