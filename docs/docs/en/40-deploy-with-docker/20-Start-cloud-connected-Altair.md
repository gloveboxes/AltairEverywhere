## Stop the Altair docker container

First, stop the Altair Docker container. From a terminal window, run the following command.

```bash
docker stop altair8800
```

## Create an environment file

1. Using your text editor of your choice create a **altair.env** environment file.
2. Add the following keys to the file:

    ```env
    ID_SCOPE=
    DEVICE_ID=
    DERIVED_KEY=
    OPEN_WEATHER_MAP_API_KEY=
    TZ=Australia/Sydney
    ```

    The ID_SCOPE, DEVICE_ID, and DERIVED_KEY values are used to connect the Altair emulator to Azure IoT Central. The OPEN_WEATHER_MAP_API_KEY value is used to connect the Altair emulator to Open Weather Map. The TZ value is used to set the time zone for the Altair emulator.

3.  Update the values with your data you copied from the [Cloud sevices](../20-fundamentals/50-Cloud-services/01-Add-cloud-services.md) page.

    :::tip

    - If you don't use a service then leave the key-value blank.
    - Don't include any speech marks in key-values.
    - Replace the Australia/Sydney time zone with your local time zone.

    :::



3. Save the file as **~/altair.env**. The commands below assume you have saved the Altair environment file to your computer or device's home directory.


## Select the Altair Docker image

### General Linux, macOS, Windows, and Raspberry Pi users

1. For general use on 64-bit Linux, macOS, Windows, and Raspberry Pi operating systems. Run the following command.

    ```bash
    docker run -d --env-file ~/altair.env -p 8082:8082 -p 80:80 -v altair-disks:/Altairdocker/AltairHL_emulator/Disks --name altair8800 --rm glovebox/altair8800:latest
    ```

### Raspberry Pi with Pi Sense HAT users

1. For a Raspberry Pi running Raspberry Pi OS with a Pi Sense HAT. Run the following command.

    ```bash
    docker run -d --env-file ~/altair.env --privileged -p 8082:8082 -p 80:80 -v altair-disks:/Altairdocker/AltairHL_emulator/Disks --name altair8800 --rm glovebox/altair8800-pisense:latest
    ```

## Open the Web Terminal

Open the Web Terminal to access the Altair emulator. Open your web browser:

 * Navigate to `http://localhost` if you deployed the Altair emulator on your local computer.
 * Navigate to `http://hostname_or_ip_address` if you deployed the Altair emulator on a remote computer.

![The image shows the web terminal](../20-fundamentals/img/web_terminal_intro.png)
