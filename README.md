# Cloud Enabled Altair 8800 on Azure Sphere

[Azure Sphere Clould Enabled Altair 8880 Emulator Documentation](https://github.com/AzureSphereCloudEnabledAltair8800/AzureSphereCloudEnabledAltair8800.emulator/wiki)

```bash
git clone --recurse-submodules https://github.com/gloveboxes/Altair8800Linux.git
```


```bash
sudo apt-get install -y libuv1.dev unzip cmake build-essential gdb curl libcurl4-openssl-dev libssl-dev uuid-dev ca-certificates git mosquitto libi2c-dev
```bash

*  sudo raspi-config --> interfacing options --> enable i2c
 *
 *  sudo apt install libi2c-dev




## Mosquitto broker configuration

```bash
sudo nano /etc/mosquitto/conf.d/altair.conf
```

```text
listener 1883 localhost
allow_anonymous true

listener 8080
allow_anonymous true
protocol websockets

```

```bash
sudo systemctl restart mosquitto.service
```


## Start Web Server on Raspberry Pi

```bash
python3 -m http.server --cgi 8081
```