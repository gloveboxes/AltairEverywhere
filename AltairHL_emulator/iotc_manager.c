/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "iotc_manager.h"

/// <summary>
/// Device Twin Handler to set the brightness of panel LEDs
/// </summary>
DX_DEVICE_TWIN_HANDLER(set_led_brightness_handler, deviceTwinBinding)
{
    if (!IN_RANGE(*(int *)deviceTwinBinding->propertyValue, 0, 15)) {
        dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue, DX_DEVICE_TWIN_RESPONSE_ERROR);
    } else {

#ifdef ALTAIR_FRONT_PANEL_PI_SENSE
		set_led_panel_color(*(int *)deviceTwinBinding->propertyValue);
#endif	// ALTAIR_FRONT_PANEL_PI_SENSE

        dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue, DX_DEVICE_TWIN_RESPONSE_COMPLETED);
    }
}
DX_DEVICE_TWIN_HANDLER_END

/// <summary>
/// Device Twin Handler to set the mqtt channel id
/// </summary>
DX_DEVICE_TWIN_HANDLER(set_channel_id_handler, deviceTwinBinding)
{
    write_channel_id_to_storage(*(int *)deviceTwinBinding->propertyValue);
    dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue, DX_DEVICE_TWIN_RESPONSE_COMPLETED);
}
DX_DEVICE_TWIN_HANDLER_END

void publish_telemetry(int temperature, int pressure)
{
    if (dx_jsonSerialize(msgBuffer, sizeof(msgBuffer), 2, DX_JSON_INT, "Temperature", temperature, DX_JSON_INT, "Pressure", pressure)) {
        dx_azurePublish(msgBuffer, strlen(msgBuffer), NULL, 0, NULL);
    }
}

