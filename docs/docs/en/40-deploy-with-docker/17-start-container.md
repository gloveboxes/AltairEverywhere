# Start the Altair emulator

There are two classes of docker images you can use to run the Altair emulator.

1. The first is for general use on 64-bit [Linux, macOS, Windows, and Raspberry Pi operating systems](#general-linux-macos-windows-and-raspberry-pi-users).
2. The second image is for a Raspberry Pi running [Raspberry Pi OS with a Pi Sense HAT](#raspberry-pi-with-pi-sense-hat-users). The Pi Sense HAT 8x8 LED panel can display the Altair address and data bus information can also be switched between *Font* and *bitmap* modes for games.

    | Raspberry Pi with Pi Sense HAT  | Raspberry Pi Sense HAT |
    |--|--|
    | ![The image shows the address and data bus LEDs](img/raspberry_pi_sense_hat_map.png) | ![The gif shows the address and data bus LEDs in action](img/raspberry_pi_sense_hat.gif) |

## Altair disk storage

The Altair emulator disks are stored in a Docker persistent storage volume. This ensures any changes made to the contents of the Altair disks are saved if the Docker container is stopped or deleted.

## Start the docker container

You need to follow these steps to start the Altair emulator Docker container.

1. Enable I2C hardware access. This is only required if you are running the Altair emulator on a Raspberry Pi with a Pi Sense HAT.
1. Create a Docker persistent storage volume.
1. Select and start a Docker container.

## Enable the Pi Sense HAT

If you are running the Altair emulator on a Raspberry Pi with a Pi Sense HAT, you need to enable I2C hardware access with the following command.

```bash
sudo raspi-config nonint do_i2c 0
```

## Select the Altair Docker image

Select the Altair Docker image that matches your system. Be sure to replace the Australia/Sydney time zone with your local time zone.

### General Linux, macOS, Windows, and Raspberry Pi users

1. For general use on 64-bit Linux, macOS, Windows, and Raspberry Pi operating systems. Run the following command.

    Note, MQTT and Open Weather Map environment variables are optional. If you do not want to use these features, you can remove the `-e` options from the command.

    For MacOs and Linux, run the following command.

    ```bash
    docker run -e TZ=Australia/Sydney \
    -e MQTT_HOST=YOUR_MQTT_HOST -e MQTT_PORT=YOUR_MQTT_PORT -e MQTT_CLIENT_ID=YOUR_MQTT_CLIENT_ID \
    -e OPEN_WEATHER_MAP_API_KEY=YOUR_OPEN_WEATHER_MAP_API_KEY \
    -d --privileged --user root \
    -p 8082:8082 -p 80:80 \
    --name altair8800 \
    -v altair-disks:/app/Disks \
    --rm glovebox/altair8800:latest
    ```

    For Windows, run the following command.

    ```powershell
    docker run -e TZ=Australia/Sydney `
    -e MQTT_HOST=YOUR_MQTT_HOST -e MQTT_PORT=YOUR_MQTT_PORT -e MQTT_CLIENT_ID=YOUR_MQTT_CLIENT_ID `
    -e OPEN_WEATHER_MAP_API_KEY=YOUR_OPEN_WEATHER_MAP_API_KEY `
    -d --privileged --user root `
    -p 8082:8082 -p 80:80 `
    --name altair8800 `
    -v altair-disks:/app/Disks `
    --rm glovebox/altair8800:latest
    ```

### Raspberry Pi with Pi Sense HAT users

1. For a Raspberry Pi running Raspberry Pi OS with a Pi Sense HAT. Run the following command.

    ```bash
    docker run -e TZ=Australia/Sydney \
    -e MQTT_HOST=YOUR_MQTT_HOST -e MQTT_PORT=YOUR_MQTT_PORT -e MQTT_CLIENT_ID=YOUR_MQTT_CLIENT_ID \
    -e OPEN_WEATHER_MAP_API_KEY=YOUR_OPEN_WEATHER_MAP_API_KEY \
    -d --privileged --user root \
    -p 8082:8082 -p 80:80 \
    --name altair8800 \
    -v altair-disks:/app/Disks \
    --device=/dev/i2c-1 \
    --rm glovebox/altair8800-pisense:latest
    ```

## Open the Web Terminal

Open the Web Terminal to access the Altair emulator. Follow these steps.

1. Familiarize yourself with the [Web Terminal](../20-fundamentals/25-Web-Terminal.md) and the CP/M operating system.
2. Open your web browser:
    * Navigate to `http://localhost` if you deployed the Altair emulator on your local computer.
    * Navigate to `http://hostname_or_ip_address` if you deployed the Altair emulator on a remote computer.

    ![The following image is of the web terminal command prompt](../20-fundamentals/img/web_terminal_intro.png)

## Docker tips and tricks

### Stop the Altair emulator Docker container

Use the following command to stop the Altair emulator Docker container.

```bash
docker stop altair8800
```

### Restart the Altair emulator Docker container

Use the following command to start the Altair emulator Docker container that you previously stopped.

```bash
docker start altair8800
```

### Delete the Altair emulator Docker container

First, stop the Altair emulator Docker container, then delete the Altair emulator container.

Use the following command to delete the Altair Docker container.

```bash
docker container rm altair8800
```

### Inspect the persistent storage volume

```bash
docker volume inspect altair-disks
```

### Check the data in the persistent storage volume

```bash
sudo ls /var/lib/docker/volumes/altair-disks/_data -all
```

### To remove the persistent storage volume

```bash
docker volume rm altair-disks
```

<!-- ## Trouble shooting Raspberry Pi issues

1. Ensure strong WiFi connection
1. Disabling the WiFi power management can improve stability

    ```bash
    sudo iw wlan0 set power_save off
    ``` -->
