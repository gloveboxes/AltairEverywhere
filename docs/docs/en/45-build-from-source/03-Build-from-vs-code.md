## Install Visual Studio Code

1. Install [Visual Studio Code](https://code.visualstudio.com&azure-portal=true).

You need to pass the IoT Central and derived device key to your application.

1. Open a Terminal command prompt.
    If you are using Windows Subsystem for Linux then open a WSL command prompt.
1. Go to the **Altair-8800-Emulator/src** folder
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
1. Optionally update the [args] json property for the build configuration. Available options are:
    - `--MqttHost <host>`: MQTT broker hostname (required for MQTT)
    - `--MqttPort <port>`: MQTT broker port (default: 1883)
    - `--MqttClientId <client_id>`: MQTT client ID (default: AltairEmulator_<timestamp>)
    - `--MqttUsername <username>`: MQTT username (default: none)
    - `--MqttPassword <password>`: MQTT password (default: none)
    - `--NetworkInterface <iface>`: Network interface to use
    - `--FrontPanel <mode>`: Front panel selection: sensehat, kit, none (default: none)
    - `--OpenWeatherMapKey <key>`: OpenWeatherMap API key
    - `--OpenAIKey <key>`: OpenAI API key

    **Example: Connecting to a ThingsBoard MQTT broker**

    To connect to a ThingsBoard MQTT broker, set the `args` property in your `.vscode/launch.json` like this:

    ```json
    "args": [
        "--MqttHost", "my-thingsboard-host",
        "--MqttClientId", "vscode",
    ]
    ```

1. Save the launch.json file.
1. Select the **GCC** or **Clang** Kit.
1. Press <kbd>F5</kbd> to compile and launch the Altair emulator.
