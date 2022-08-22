/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "altair_panel.h"
#include "dx_azure_iot.h"
#include "dx_device_twins.h"
#include "dx_json_serializer.h"
#include "dx_utilities.h"
#include "environment.h"
#include "web_socket_server.h"

extern DX_DEVICE_TWIN_BINDING dt_city;
extern DX_DEVICE_TWIN_BINDING dt_country;
extern DX_DEVICE_TWIN_BINDING dt_location;

extern bool azure_connected;
extern char msgBuffer[MSG_BUFFER_BYTES];

DX_DECLARE_DEVICE_TWIN_HANDLER(set_led_brightness_handler);

void publish_telemetry(ENVIRONMENT_TELEMETRY *environment);
void update_geo_location(ENVIRONMENT_TELEMETRY *environment);