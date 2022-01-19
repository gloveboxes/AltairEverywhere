# Cloud Enabled Altair 8800 on Azure Sphere

[Azure Sphere Cloud Enabled Altair 8880 Emulator Documentation](https://github.com/AzureSphereCloudEnabledAltair8800/AzureSphereCloudEnabledAltair8800.emulator/wiki)

## Hardware

1. Raspberry Pi plus optional Raspberry Pi Sense HAT
1. Desktop computer running Windows, Linux, or macOS

## Operating Systems

The Altair project has been tested on the following *NIX style operating systems:

## Windows computers

On Windows, you can build and run the Altair 8800 project from Windows Subsystem for Linux (WSL2).

1. If you have not done so already, then [install WSL2](https://docs.microsoft.com/windows/wsl/install).
1. Optional, but recommended, install the [Windows Terminal](https://docs.microsoft.com/windows/terminal/install)

You **must** run all commands listed below from the **WSL terminal window**.

## Linux computers

1. Raspberry Pi OS 32-bit (Bullseye)
1. Desktop Linux (Ubuntu 20.04)

## Apple computers 

1. macOS Monterey 12.1 (Intel and Apple Silicon)

## Terminal window



## Required packages

The Altair project requires the following packages:

1. [libuv](http://docs.libuv.org/en/v1.x/design.html) event loop library.
1. SSL Development.
1. [OSSP uuid](http://www.ossp.org/pkg/lib/uuid/)
1. [Mosquitto](https://mosquitto.org/) MQTT broker.
1. C build, compiler, and debugger tools.

### Linux systems

For Linux based systems, including Raspberry Pi OS, Windows Subsystem for Linux, and desktop Linux, install the following packages. 

1. Open a Terminal window and run the following command.

    ```bash
    sudo apt-get install -y libuv1.dev cmake build-essential gdb curl libcurl4-openssl-dev libssl-dev uuid-dev ca-certificates git mosquitto libi2c-dev
    ```

### macOS

1. Install [Homebrew](https://brew.sh/)

    ```bash
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    ```

1. Open a Terminal window.
1. Run the following command to install the required packages.

    ```bash
    brew install mosquitto libuv openssl ossp-uuid 
    ```

## Clone Altair 8800 project repo

Clone the Altair 8800 project repo to your desktop computer.  

> For Windows Subsystem for Linux users. You must clone the Altair repo into the Linux file system (NOT the Windows file system).

1. Open a Terminal window.
2. Run the following command to clone the Altair repo to your computer.

    ```bash
    git clone --recurse-submodules https://github.com/gloveboxes/Altair8800Linux.git
    ```

## Configure the Altair project

If you are building the project on a Raspberry Pi with a Pi Sense HAT then you must enable Pi Sense HAT support.

```bash
tbd
```

## Build the Altair project

Next, build the Altair project.

1. Open a Terminal window
1. Navigate to the Altair8800Linux folder.

    ```bash
    cd Altair8800Linux
    ```
2. Run the following commands to build the project.

    ```bash
    cd AltairHL_emulator && \
    mkdir -p build && \
    cmake -B build && \
    cmake --build build --config release --target all 
    ```

## Raspberry Pi Sense HAT support

If you are building the Altair project in a Raspberry Pi with a PI Sense HAT then you need to enable I2C support.

1. Open a Terminal window
1. Run the following command to start the Raspberry Pi config utility.

    ```bash
    sudo raspi-config
    ```
1. Select **Interface options**
1. Select **I2C**
1. Select **Yes**
1. Press <kbd>ENTER</kbd>
1. Press <kbd>ENTER</kbd>
1. Select **Finish**
1. Press <kbd>ENTER</kbd>




## Mosquitto MQTT broker configuration

Create a Mosquitto configuration file. The configuration file defines the ports and protocols the Mosquitto MQTT broker will use.

### Linux systems

1. Open a Terminal window
1. Run the following command to create a Mosquitto configuration file with the **nano** text editor

    ```bash
    sudo nano /etc/mosquitto/conf.d/altair.conf
    ```

### macOS

1. Open a Terminal window
1. Run the following command to create a Mosquitto configuration file with the **nano** text editor

    ```bash
    sudo nano /opt/homebrew/etc/mosquitto/altair.conf
    ```

### Set the Mosquitto broker config

1. Paste the following text into the Nano text editor.

    ```text
    listener 1883 localhost
    allow_anonymous true

    listener 8080
    allow_anonymous true
    protocol websockets
    ```

1. Save the file changes

    1. Press <kbd>ctrl+x</kbd>
    1. Confirm you want to save modified buffer. Press **y**
    1. Confirm file name. Press <kbd>ENTER</kbd>


## Restart the Mosquitto MQTT broker

Restart the Mosquitto broker.

## Windows Subsystem for Linux

1. Open a WSL Terminal window
1. Run the following command to start Mosquitto as a background process.

    ```bash
    mosquitto -c /etc/mosquitto/mosquitto.conf &
    ```

## Linux systems

```bash
sudo systemctl restart mosquitto.service
```

#### macOS

```bash
mosquitto -c /opt/homebrew/etc/mosquitto/altair.conf &
```
<!-- 
#### Auto start Mosquitto Broker on macOS



##### Apple Silicon -->

<!-- 1. Open org.user.mosquitto.plist

    ```bash
    sudo nano ~/Library/LaunchAgents/org.user.mosquitto.plist
    ```
1. Paste the following text into the org.user.mosquitto.plist file

    ```text
    <?xml version="1.0" encoding="UTF-8"?>
    <!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
    <plist version="1.0">
    <dict>
    <key>Label</key>
    <string>homebrew.mxcl.mosquitto</string>
    <key>ProgramArguments</key>
    <array>
        <string>/opt/homebrew/sbin/mosquitto</string>
        <string>-c</string>
        <string>/opt/homebrew/etc/mosquitto/altair.conf</string>
    </array>
    <key>RunAtLoad</key>
    <true/>
    <key>KeepAlive</key>
    <false/>
    <key>WorkingDirectory</key>
    <string>/opt/homebrew/sbin/mosquitto</string>
    </dict>
    </plist>
    ```
1. Save the file -->


## Start Web Terminal HTTP Server

1. Open a Terminal window
1. Navigate to the Altair8800Linux folder.

    ```bash
    cd Altair8800Linux
    ```
1. Run the follow command to start a HTTP web server.

    ```bash
    cd Altair8800Linux/Terminal
    python3 -m http.server 8081 &
    ```


## IoT Central

### Load PnP Template

### Create a device

### Create IoT Central device connection string

The initial release of the Altair on Linux project does not support Azure Device Provisioning which is a requirement for IoT Plug and Play and IoT Central.

Instead you will manually create a device connection string for an IoT Central device.



<!-- ## Appendix

## Autostart Web Terminal Service on Raspberry Pi

1. Open Services/altair_terminal.service with the nano editor.

    ```bash
    nano Services/altair_terminal.service
    ```

    ```text
    [Unit]
    Description=Altair Web Terminal daemon
    After=multi-user.target

    [Service]
    Type=idle
    User=pi
    WorkingDirectory=/home/pi/Altair8800Linux/Terminal
    ExecStart=/usr/bin/python3 -m http.server 8081
    Restart=always


    [Install]
    WantedBy=multi-user.target
    ```

2. Validate the following items are correct.
    - **User** - defaults to pi
    - **WorkingDirectory** - defaults to /home/pi/Altair8800Linux/Terminal

3. Save any changes

    1. Press <kbd>ctrl+x</kbd>
    1. Confirm you want to save modified buffer. Press **y**
    1. Confirm file name. Press <kbd>ENTER</kbd>

### Copy the altair_terminal.service

```bash
cp Services/altair_terminal.service /etc/systemd/system/altair_terminal.service
```

### Reload Systemd

```bash
sudo systemctl daemon-reload
``` -->




