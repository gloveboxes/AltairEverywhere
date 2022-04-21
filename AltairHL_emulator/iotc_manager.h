/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "altair_panel.h"
#include "dx_azure_iot.h"
#include "dx_device_twins.h"
#include "dx_json_serializer.h"
#include "dx_utilities.h"
#include "environment.h"
#include "front_panel_pi_sense_hat.h"
#include "web_socket_server.h"

extern DX_DEVICE_TWIN_BINDING dt_air_quality_index;
extern DX_DEVICE_TWIN_BINDING dt_carbon_monoxide;
extern DX_DEVICE_TWIN_BINDING dt_nitrogen_monoxide;
extern DX_DEVICE_TWIN_BINDING dt_nitrogen_dioxide;
extern DX_DEVICE_TWIN_BINDING dt_ozone;
extern DX_DEVICE_TWIN_BINDING dt_sulphur_dioxide;
extern DX_DEVICE_TWIN_BINDING dt_ammonia;
extern DX_DEVICE_TWIN_BINDING dt_pm2_5;
extern DX_DEVICE_TWIN_BINDING dt_pm10;

extern DX_DEVICE_TWIN_BINDING dt_humidity;
extern DX_DEVICE_TWIN_BINDING dt_location;
extern DX_DEVICE_TWIN_BINDING dt_country;
extern DX_DEVICE_TWIN_BINDING dt_city;
extern DX_DEVICE_TWIN_BINDING dt_mainPollutant;
extern DX_DEVICE_TWIN_BINDING dt_pressure;
extern DX_DEVICE_TWIN_BINDING dt_temperature;
extern DX_DEVICE_TWIN_BINDING dt_wind_speed;
extern DX_DEVICE_TWIN_BINDING dt_wind_direction;
extern DX_DEVICE_TWIN_BINDING dt_weather;
extern bool azure_connected;
extern char msgBuffer[MSG_BUFFER_BYTES];

// void publish_properties(ENVIRONMENT_TELEMETRY *environment);
DX_DECLARE_DEVICE_TWIN_HANDLER(set_led_brightness_handler);
void publish_telemetry(ENVIRONMENT_TELEMETRY *environment);
void update_geo_properties(ENVIRONMENT_TELEMETRY *environment);