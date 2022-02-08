/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "iotc_manager.h"

static DX_MESSAGE_PROPERTY *messageProperties[] = {
      &(DX_MESSAGE_PROPERTY){.key = "appid", .value = "altair"}, 
      &(DX_MESSAGE_PROPERTY){.key = "type", .value = "weather"},
      &(DX_MESSAGE_PROPERTY){.key = "schema", .value = "1"}};

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



static void device_twin_update_location(double latitude, double longitude, DX_DEVICE_TWIN_BINDING *device_twin)
{
    char location_buffer[128];
    snprintf(location_buffer, sizeof(location_buffer), "{\"lat\":%f,\"lon\":%f,\"alt\":0}", latitude, longitude);
    dx_deviceTwinReportValue(device_twin, location_buffer);
}

static void update_geo_location(WEATHER_TELEMETRY *weather)
{
    static bool updated = false;

    if (!updated){
        updated = true;
        device_twin_update_location(weather->locationInfo->lat, weather->locationInfo->lng, &dt_location);
    }
}

void publish_telemetry(WEATHER_TELEMETRY *weather)
{
    if (!azure_connected){
        return;
    }

    if (dx_jsonSerialize(msgBuffer, sizeof(msgBuffer), 5, 
        DX_JSON_INT, "temperature", weather->latest.temperature, 
        DX_JSON_INT, "pressure", weather->latest.pressure,
        DX_JSON_INT, "humidity", weather->latest.humidity,
        DX_JSON_DOUBLE, "latitude", weather->locationInfo->lat,
        DX_JSON_DOUBLE, "longitude", weather->locationInfo->lng)) 
    {
        dx_azurePublish(msgBuffer, strlen(msgBuffer), messageProperties, NELEMS(messageProperties), NULL);
    }

    device_twin_update_int(&weather->latest.temperature, &weather->previous.temperature, &dt_temperature);
    device_twin_update_int(&weather->latest.pressure, &weather->previous.pressure, &dt_pressure);
    device_twin_update_int(&weather->latest.humidity, &weather->previous.humidity, &dt_humidity);

    device_twin_update_string(weather->latest.description, weather->previous.description, sizeof(((SENSOR_T*)0)->description), &dt_weather);
    update_geo_location(weather);
}
