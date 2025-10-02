# Add Open Weather Map Key

## Open Weather Map weather and pollution service

To connect the Altair emulator to the Open Weather Map weather and pollution APIs you'll need to declare the Open Weather Map API key.

Update the **cmdArgs** section of the app_manifest.json file.

```json
"CmdArgs": [
    "--ScopeID",
    "",
    "-o",
    "REPLACE_WITH_YOUR_OPEN_WEATHER_MAP_API_KEY"
],
```

### Default app_manifest.json

The following is the default project app_manifest.json file. Open the Altair emulator app_manifest.json file and update.

```json
{
    "SchemaVersion": 1,
    "Name": "AltairHL_emulator",
    "ComponentId": "ac8d863a-4424-11eb-b378-0242ac130002",
    "EntryPoint": "/bin/app",
    "CmdArgs": [
        "--ScopeID",
        "",
        "-o",
        ""
    ],
    "Capabilities": {
        "MutableStorage": {
            "SizeKB": 64
        },
        "Gpio": [
            "$AZURE_CONNECTED_LED",
            "$BUTTON_A",
            "$BUTTON_B",
            "$LED_RED",
            "$LED_GREEN",
            "$LED_BLUE"
        ],
        "PowerControls": [
            "SetPowerProfile"
        ],
        "WifiConfig": true,
        "CertStore": true,
        "EnterpriseWifiConfig": true,
        "NetworkConfig": true,
        "AllowedTcpServerPorts": [
            8082
        ],
        "I2cMaster": [
            "$ISU2"
        ],
        "Adc": [
            "$AVNET_LIGHT_SENSOR"
        ],
        "AllowedConnections": [
            "global.azure-devices-provisioning.net",
            "api.openweathermap.org",
            "get.geojs.io",
            "raw.githubusercontent.com"
        ],
        "DeviceAuthentication ": null,
        "AllowedApplicationConnections": [
            "2e319eae-7be5-4a0c-ba47-9353aa6ca96a",
            "9b684af8-21b9-42aa-91e4-621d5428e497",
            "005180bc-402f-4cb3-a662-72937dbcde47",
            "6583cf17-d321-4d72-8283-0b7c5b56442b",
            "AF8B26DB-355E-405C-BBDE-3B851668EE23"
        ]
    },
    "ApplicationType": "Default"
}
```
