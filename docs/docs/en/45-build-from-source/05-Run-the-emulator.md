# Start the emulator

1. From a terminal window, change to the **AltairEverywhere/AltairHL_emulator** folder that you cloned to your computer.
1. You can run the Altair emultaor in standalone mode.

    ```bash
    ./build/Altair_emulator
    ```

1. Alternatively, you start the emulator in cloud connected mode, you need the following information.

    1. ID_SCOPE: Azure IoT Central ID Scope
    1. DEVICE_ID: Azure IoT Central Device ID
    1. DERIVED_KEY: Azure IoT Central Device Derived Key
    1. OPEN_WEATHER_MAP_API_KEY: Open Weather Map API Key
    1. NETWORK_INTERFACE: Network interface name. eg wlan0, eth0

1. To start the emulator in cloud connected mode, run the following command.

    ```bash
    ./build/Altair_emulator -s ID_SCOPE -d DEVICE_ID -k DERIVED_KEY -o OPEN_WEATHER_MAP_API_KEY -n NETWORK_INTERFACE
    ```

    To run the emulator as a background task.

    ```bash
    nohup ./build/Altair_emulator -s ID_SCOPE -d DEVICE_ID -k DERIVED_KEY -o OPEN_WEATHER_MAP_API_KEY -n NETWORK_INTERFACE &
    ```
