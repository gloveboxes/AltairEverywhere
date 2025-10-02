# Windows

Complete the following steps:

1. If you have not done so already, then install Windows Subsystem for Linux [(WSL2)](https://docs.microsoft.com/windows/wsl/install) and Ubuntu 20.04.
1. Optional, but recommended, install the [Windows Terminal](https://docs.microsoft.com/windows/terminal/install)

## Install the required packages

The Altair project requires the following packages:

1. [libuv1](https://man7.org/linux/man-pages/man3/event.3.html) event loop library.
1. SSL Development.
1. [OSSP uuid](http://www.ossp.org/pkg/lib/uuid/)
1. C compiler and debugging tools.

## Windows Subsystem for Linux users

Follow these steps to install the required packages.

1. Open an WSL Ubuntu Terminal window.
1. Run the following command to install the required packages

    ```bash
    sudo apt-get install -y libuv1-dev cmake build-essential gdb curl libcurl4-openssl-dev libssl-dev uuid-dev ca-certificates git libi2c-dev libgpiod-dev gpiod
    ```
