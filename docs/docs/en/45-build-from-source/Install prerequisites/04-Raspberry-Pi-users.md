# Raspberry Pi OS

## Install Raspberry Pi OS

The easiest way to install Raspberry Pi OS is to use the latest version of the [Raspberry Pi Imager](https://www.raspberrypi.com/software/).

1. Check which version of Raspberry Pi OS your Raspberry Pi is [compatible](https://www.raspberrypi.com/software/operating-systems/) with.
1. If compatible, then install Raspberry Pi OS 64 bit **no desktop** to maximize memory for the Altair emulator build process.
1. Learn how to use the [Raspberry Pi Imager](https://youtu.be/ntaXWS8Lk34).
1. Select the Raspberry Pi Imager Settings icon to configure the operating system installation, wifi settings and more.

### Connect to your Raspberry Pi

1. From your desktop computer, start an SSH session to your Raspberry Pi.
1. From the SSH session, run the following command to install the required packages

    ```bash
    sudo apt-get install -y libuv1-dev cmake build-essential gdb curl libcurl4-openssl-dev libssl-dev uuid-dev ca-certificates git libi2c-dev libgpiod-dev gpiod
    ```

## Raspberry Pi Sense HAT support

If you are building the Altair project on a Raspberry Pi with a [Raspberry Pi PI Sense HAT](https://www.raspberrypi.com/products/sense-hat/) then you need to enable the I2C bus.

From the command prompt, run the following command to enable I2C support for the Pi Sense HAT.

```bash
sudo raspi-config nonint do_i2c 0
```
