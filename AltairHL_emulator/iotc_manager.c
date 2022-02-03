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

/// <summary>
/// Determine if environment value changed. If so, update it's device twin
/// </summary>
/// <param name="new_value"></param>
/// <param name="previous_value"></param>
/// <param name="device_twin"></param>
static void device_twin_update_int(int *latest_value, int *previous_value, DX_DEVICE_TWIN_BINDING *device_twin)
{
    if (*latest_value != *previous_value)
    {
        *previous_value = *latest_value;
        dx_deviceTwinReportValue(device_twin, latest_value);
    }
}

static void device_twin_update_double(double *latest_value, double *previous_value, DX_DEVICE_TWIN_BINDING *device_twin)
{
    if (*latest_value != *previous_value)
    {
        *previous_value = *latest_value;
        dx_deviceTwinReportValue(device_twin, latest_value);
    }
}

static void device_twin_update_string(char *latest_value, char *previous_value, size_t length, DX_DEVICE_TWIN_BINDING *device_twin)
{
    if (strncmp(latest_value, previous_value, length) != 0)
    {
        strncpy(previous_value, latest_value, length);
        dx_deviceTwinReportValue(device_twin, latest_value);
    }
}

void publish_telemetry(WEATHER_TELEMETRY *weather)
{
    if (!dx_isAzureConnected()){
        return;
    }

    if (dx_jsonSerialize(msgBuffer, sizeof(msgBuffer), 6, 
        DX_JSON_INT, "temperature", weather->latest.temperature, 
        DX_JSON_INT, "pressure", weather->latest.pressure,
        DX_JSON_INT, "humidity", weather->latest.humidity,
        DX_JSON_DOUBLE, "latitude", weather->latest.latitude,
        DX_JSON_DOUBLE, "longitude", weather->latest.longitude,
        DX_JSON_STRING, "countryCode", weather->latest.country_code)) 
    {
        dx_azurePublish(msgBuffer, strlen(msgBuffer), NULL, 0, NULL);
    }

    device_twin_update_int(&weather->latest.temperature, &weather->previous.temperature, &dt_temperature);
    device_twin_update_int(&weather->latest.pressure, &weather->previous.pressure, &dt_pressure);
    device_twin_update_int(&weather->latest.humidity, &weather->previous.humidity, &dt_humidity);

    device_twin_update_double(&weather->latest.latitude, &weather->previous.latitude, &dt_latitude);
    device_twin_update_double(&weather->latest.longitude, &weather->previous.longitude, &dt_longitude);

    device_twin_update_string(weather->latest.description, weather->previous.description, 80, &dt_weather);
    device_twin_update_string(weather->latest.country_code, weather->previous.country_code, 10, &dt_countryCode);
}
