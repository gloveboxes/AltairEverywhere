/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "iotc_manager.h"
#include "dx_mqtt.h"

// External reference to global MQTT configuration
extern DX_MQTT_CONFIG mqtt_config;

void update_geo_location(ENVIRONMENT_TELEMETRY *environment)
{
    static bool updated = false;

    if (!updated && environment->locationInfo.updated)
    {
        updated = true;
        // Device twin reporting removed - using MQTT only
        dx_Log_Debug("Country: %s, City: %s\n", environment->locationInfo.country, environment->locationInfo.city);
    }
}

void publish_telemetry(ENVIRONMENT_TELEMETRY *environment)
{
    if (!dx_isMqttConnected() || !environment->valid)
    {
        return;
    }

    const char *publish_template = "{"
                                   "\"device\":\"%s\","
                                   "\"timestamp\":%ld,"
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

    size_t msg_len = (size_t)snprintf(msgBuffer, sizeof(msgBuffer), publish_template, mqtt_config.client_id, time(NULL), environment->latest.weather.temperature,
        environment->latest.weather.pressure, environment->latest.weather.humidity, environment->latest.weather.wind_speed,
        environment->latest.weather.wind_direction, environment->latest.pollution.air_quality_index,
        environment->latest.pollution.carbon_monoxide, environment->latest.pollution.nitrogen_monoxide,
        environment->latest.pollution.nitrogen_dioxide, environment->latest.pollution.ozone, environment->latest.pollution.sulphur_dioxide,
        environment->latest.pollution.ammonia, environment->latest.pollution.pm2_5, environment->latest.pollution.pm10,
        environment->locationInfo.lat, environment->locationInfo.lng);

    if (msg_len < sizeof(msgBuffer))
    {
        // Publish telemetry via MQTT instead of Azure IoT Hub
        DX_MQTT_MESSAGE mqtt_msg = {
            .topic = "altair/telemetry/weather", 
            .payload = msgBuffer, 
            .payload_length = msg_len, 
            .qos = 0, 
            .retain = false
        };
        dx_mqttPublish(&mqtt_msg);
    }
    else
    {
        Log_Debug("MsgBuffer too small. Msg not sent.\n");
    }
}