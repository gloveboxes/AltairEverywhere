There are two options to configure the WiFi of the device. You can configure the WiFi with an SD Card or the Azure Sphere CLI. If you use a Mikroe Retro 8800 Click, then the IP address will be displayed on the LED panel when the device successfully connects to a WiFi network.

## WiFi provisioning with an SD card

If you deploy the Altair emulator with an Mikroe SD card Click then you can [provision the Wi-Fi from the SD card](05-Create-boot-SD-card.md).

## macOS users WiFi provisioning with an SD card

If you deploy the Altair emulator with an Mikroe SD card Click then you can [provision the Wi-Fi from the SD card](05-Create-boot-SD-card.md).

## Windows and Linux users WiFi provisioning with Azure Sphere CLI

1. Install the Azure Sphere SDK.
    - The [Quickstart: Install the Azure Sphere SDK for Windows](https://docs.microsoft.com/en-us/azure-sphere/install/install-sdk?pivots=visual-studio) will step you through the process.
    - The [Quickstart: Install the Azure Sphere SDK for Linux](https://docs.microsoft.com/en-us/azure-sphere/install/install-sdk-linux?pivots=vs-code-linux) will step you through the process.
1. Review the [device WiFi configuration section](https://docs.microsoft.com/azure-sphere/reference/azsphere-device?tabs=cliv2beta) for more information on setting up Wi-Fi with the Azure Sphere CLI.
1. List existing WiFi configurations

    ```bash
    azsphere device wifi list
    ```

1. Forget a WiFi configuration

    ```bash
    azsphere device wifi forget -i NETWORK_ID
    ```

1. Add a WiFi configuration

    ```bash
    azsphere device wifi add -s YOUR_SSID -p YOUR_NETWORK_PASSWORD
    ```
1. Test the Wi-Fi has connected. From a command prompt, issue the following command.

    ```bash
    azsphere device wifi show-status
    ```
