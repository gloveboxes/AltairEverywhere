---
sidebar_position: 7
---

# Add network endpoints

1. Open the **app_manifest.json** file.
1. Update the **CmdArgs** property with your IoT Central **ID Scope**.
1. Get your Azure Sphere Tenant ID. From a **command prompt**, run the following command.

    ```powershell
    azsphere tenant show-selected -o yaml
    ```

    The output of this command will be similar to the following. The **id** property value is your Azure Sphere Tenant ID.

    ```yaml
    id: 9abc79eb-9999-43ce-9999-fa8888888894
    name: myAzureSphereTenant
    roles:
    - Administrator
    ```

1. Update the **DeviceAuthentication** property with your **Azure Sphere Tenant ID**.
1. Update the **AllowedConnections** property with the IoT Central Application endpoint URLs you copied to Notepad.
1. **Review** your updated manifest_app.json file. It should be similar to the following.

    ```json
    {
      "SchemaVersion": 1,
      "Name": "co2monitor",
      "ComponentId": "25025d2c-66da-4448-bae1-ac26fcdd3627",
      "EntryPoint": "/bin/app",
      "CmdArgs": [ "--ScopeID", "0ne0099999D" ],
      "Capabilities": {
        "SystemEventNotifications": true,
        "SoftwareUpdateDeferral": true,
        "Gpio": [ "$AZURE_CONNECTED_LED", "$BUTTON_B" ],
        "Pwm": [ "$PWM_CLICK_CONTROLLER", "$PWM_RGB_CONTROLLER" ],
        "I2cMaster": [ "$I2C_ISU2" ],
        "Adc": [ "$AVNET_MT3620_SK_ADC_CONTROLLER0" ],
        "AllowedConnections": [
            "global.azure-devices-provisioning.net",
            "iotc-9999bc-3305-99ba-885e-6573fc4cf701.azure-devices.net",
            "iotc-789999fa-8306-4994-b70a-399c46501044.azure-devices.net",
            "iotc-7a099966-a8c1-4f33-b803-bf29998713787.azure-devices.net",
            "iotc-97299997-05ab-4988-8142-e299995acdb7.azure-devices.net",
            "iotc-d099995-7fec-460c-b717-e99999bf4551.azure-devices.net",
            "iotc-789999dd-3bf5-49d7-9e12-f6999991df8c.azure-devices.net",
            "iotc-29999917-7344-49e4-9344-5e0cc9999d9b.azure-devices.net",
            "iotc-99999e59-df2a-41d8-bacd-ebb9999143ab.azure-devices.net",
            "iotc-c0a9999b-d256-4aaf-aa06-e90e999902b3.azure-devices.net",
            "iotc-f9199991-ceb1-4f38-9f1c-13199992570e.azure-devices.net"
        ],
        "DeviceAuthentication": "9abc79eb-9999-43ce-9999-fa8888888894"
      },
      "ApplicationType": "Default"
    }
    ```
