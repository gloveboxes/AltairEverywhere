# Cloud Enabled Altair 8800 on Azure Sphere

[Azure Sphere Clould Enabled Altair 8880 Emulator Documentation](https://github.com/AzureSphereCloudEnabledAltair8800/AzureSphereCloudEnabledAltair8800.emulator/wiki)

## Install required libraries

### Raspberry Pi running Raspberry Pi OS (Bullseye)

```bash
sudo apt-get install -y libuv1.dev unzip cmake build-essential gdb curl libcurl4-openssl-dev libssl-dev uuid-dev ca-certificates git mosquitto libi2c-dev
```

### macOS Monterey 12.1 (Intel and Apple Silicon)

```bash
brew install mosquitto libuv openssl ossp-uuid 
```

### Desktop Linux (tested on Ubuntu 20.04)

```bash
sudo apt-get install -y libuv1.dev unzip cmake build-essential gdb curl libcurl4-openssl-dev libssl-dev uuid-dev ca-certificates git mosquitto libi2c-dev
```


### Windows 10 & 11 with Windows Subsystem for Linux (WSL) 2

```bash
sudo apt-get install -y libuv1.dev unzip cmake build-essential gdb curl libcurl4-openssl-dev libssl-dev uuid-dev ca-certificates git mosquitto libi2c-dev
```

### Desktop Linux

## Clone Altair 8800 repo

```bash
git clone --recurse-submodules https://github.com/gloveboxes/Altair8800Linux.git
```


## Build project

```bash
cd AltairHL_emulator && \
mkdir build && cmake -B build && \
cmake --build build --config release --target all 
```

## Enable Raspberry I2C Interface

```bash
sudo raspi-config --> interfacing options --> enable i2c
```

## Mosquitto MQTT broker configuration

### Raspberry Pi and Linux

```bash
sudo nano /etc/mosquitto/conf.d/altair.conf
```

### macOS

```bash
sudo nano /opt/homebrew/etc/mosquitto/altair.conf
```

Paste in the following text.

```text
listener 1883 localhost
allow_anonymous true

listener 8080
allow_anonymous true
protocol websockets
```


### Restart Mosquitto MQTT broker

#### Raspberry Pi and Desktop Linux

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

```bash
cd Altair8800Linux/Terminal
python3 -m http.server 8081 &
```

## Autostart Web Terminal Service on Raspberry Pi and Desktop Linux

### Validate the altair_terminal.service

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
```

## Replacing char in c

https://stackoverflow.com/questions/32496497/standard-function-to-replace-character-or-substring-in-a-char-array/32496721


