/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "iotc_manager.h"

static DX_MESSAGE_PROPERTY *weather_msg_properties[] = {
    &(DX_MESSAGE_PROPERTY){.key = "appid", .value = "altair"},
    &(DX_MESSAGE_PROPERTY){.key = "type", .value = "weather"},
    &(DX_MESSAGE_PROPERTY){.key = "schema", .value = "1"}};

static DX_MESSAGE_CONTENT_PROPERTIES weather_content_properties = {
    .contentEncoding = "utf-8", .contentType = "application/json"};Í

static void device_twin_update_location(
    double latitude, double longitude, DX_DEVICE_TWIN_BINDING *device_twin)
{
    char location_buffer[128];
    snprintf(
        location_buffer, sizeof(location_buffer), "{\"lat\":%f,\"lon\":%f,\"alt\":0}", latitude, longitude);
    dx_deviceTwinReportValue(device_twin, location_buffer);
}

void update_geo_location(ENVIRONMENT_TELEMETRY *environment)
{
    static bool updated = false;

    if (!updated)
    {
        updated = true;
        device_twin_update_location(
            environment->locationInfo.lat, environment->locationInfo.lng, &dt_location);
        dx_deviceTwinReportValue(&dt_country, environment->locationInfo.country);
        dx_deviceTwinReportValue(&dt_city, environment->locationInfo.city);
    }
}

void publish_telemetry(ENVIRONMENT_TELEMETRY *environment)
{
    if (!azure_connected || !environment->valid)
    {
        return;
    }

    const char *publish_template = "{"
                                   "\"temperature\":%d,"
                                   "\"pressure\":%d,"
                                   "\"humidity\":%d,"
                                   "\"windspeed\":%.2f,"
                                   "\"winddirection\":%d,"
                                   "\"aqi\":%.2f,"
                                   "\"co\":%.2f,"
                                   "\"no\":%.2f,"
                                   "\"no2\":%.2f,"
                                   "\"o3\":%.2f,"
                                   "\"so2\":%.2f,"
                                   "\"nh3\":%.2f,"
                                   "\"pm2_5\":%.2f,"
                                   "\"pm10\":%.2f,"
                                   "\"latitude\":%.6f,"
                                   "\"longitude\":%.6f"
                                   "}";

    size_t msg_len = (size_t)snprintf(msgBuffer, sizeof(msgBuffer), publish_template,
        environment->latest.weather.temperature, environment->latest.weather.pressure,
        environment->latest.weather.humidity, environment->latest.weather.wind_speed,
        environment->latest.weather.wind_direction, environment->latest.pollution.air_quality_index,
        environment->latest.pollution.carbon_monoxide, environment->latest.pollution.nitrogen_monoxide,
        environment->latest.pollution.nitrogen_dioxide, environment->latest.pollution.ozone,
        environment->latest.pollution.sulphur_dioxide, environment->latest.pollution.ammonia,
        environment->latest.pollution.pm2_5, environment->latest.pollution.pm10,
        environment->locationInfo.lat, environment->locationInfo.lng);

    if (msg_len < sizeof(msgBuffer))
    {
        dx_azurePublish(msgBuffer, msg_len, weather_msg_properties, NELEMS(weather_msg_properties),
            &weather_content_properties);
    }
    else
    {
        Log_Debug("MsgBuffer too small. Msg not sent.\n");
    }
}