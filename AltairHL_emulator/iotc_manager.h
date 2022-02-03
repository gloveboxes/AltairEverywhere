/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "comms_manager_wolf.h"
#include "dx_azure_iot.h"
#include "dx_device_twins.h"
#include "dx_json_serializer.h"
#include "dx_utilities.h"
#include "front_panel_pi_sense_hat.h"
#include "sphere_panel.h"
#include "storage.h"
#include "weather.h"

extern char msgBuffer[MSG_BUFFER_BYTES];
extern DX_DEVICE_TWIN_BINDING dt_temperature;
extern DX_DEVICE_TWIN_BINDING dt_pressure;
extern DX_DEVICE_TWIN_BINDING dt_humidity;
extern DX_DEVICE_TWIN_BINDING dt_weather;
extern DX_DEVICE_TWIN_BINDING dt_latitude;
extern DX_DEVICE_TWIN_BINDING dt_longitude;
extern DX_DEVICE_TWIN_BINDING dt_countryCode;

DX_DECLARE_DEVICE_TWIN_HANDLER(set_channel_id_handler);
DX_DECLARE_DEVICE_TWIN_HANDLER(set_led_brightness_handler);
void publish_telemetry(WEATHER_TELEMETRY *weather);
