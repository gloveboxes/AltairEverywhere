---
sidebar_position: 2
---

# IoT Plug and Play

This solution uses IoT Plug and Play with IoT Central. IoT Plug and Play (PnP) defines a model that a device uses to advertise its capabilities to a PnP-enabled application like IoT Central. PnP is an open specification, to learn more, refer to [What is IoT Plug and Play](https://docs.microsoft.com/azure/iot-pnp/overview-iot-plug-and-play).

An IoT Plug and Play CO2 monitor model has been published to the [public repository of IoT Plug and Play](https://github.com/Azure/iot-plugandplay-models) models. A local copy of the IoT Plug and Play model is located in the *iot_plug_and_play* directory.

When your device first connects to IoT Central, the IoT Plug and Play model is retrieved from the public repository of models. IoT Central then creates default views using the Plug and Play model.

The IoT Plug and Play model for the CO2 monitor project is declared in main.h.

```c
#define IOT_PLUG_AND_PLAY_MODEL_ID "dtmi:com:example:azuresphere:co2monitor;2"
```

