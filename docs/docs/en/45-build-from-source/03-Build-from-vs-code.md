---
sidebar_position: 4
---

# Build from Visual Studio Code

## Install Visual Studio Code

1. Install [Visual Studio Code](https://code.visualstudio.com&azure-portal=true).

You need to pass the IoT Central and derived device key to your application.

1. Open a Terminal command prompt.
    If you are using Windows Subsystem for Linux then open a WSL command prompt.
1. Go to the **AltairEverywhere/AltairHL_emulator** folder
1. Run the following command to open the folder with VS Code.

    ```bash
    code .
    ```

1. Install the following Visual Studio Code extensions:
    - If using WSL, then install the [Visual Studio Code Remote-WSL extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-wsl&azure-portal=true).
    - [Visual Studio Code CodeLLDB extension](https://marketplace.visualstudio.com/items?itemName=vadimcn.vscode-lldb)
    - [CMake Tools extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools)
    - [C/C++ extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)

1. Open the .vscode/launch.json file in VS Code.
1. Update the [args] json property for the build configuration.
1. Add the following information.

    - DPS or IoT Central ID Scope.
    - Device ID.
    - Derived device key.
    - Network interface, eg wlan0, eth0, en1.
        Use the **ifconfig** command to determine your active network interface.

    **"args": ["-s", "<YOUR_SCOPE_ID>", "-d", "<YOUR_DEVICE_ID>", "-k", "<YOUR_DERIVED_DEVICE_KEY>", "-n", "<YOUR_NETWORK_INTERFACE>", "-o", "<YOUR_OPEN_WEATHER_MAP_API_KEY>"],**

1. Save the launch.json file.
1. If you are have a Raspberry Pi Sense HAT, then enable the Pi Sense HAT in the CMakeLists.txt file. Uncomment:

    ```cmake
    # set(ALTAIR_FRONT_PI_SENSE_HAT TRUE "Enable Raspberry Pi Sense HAT")
    ```

1. Select the **GCC** or **Clang** Kit.
1. Press <kbd>F5</kbd> to compile and launch the Altair emulator.
