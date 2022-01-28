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

extern char msgBuffer[MSG_BUFFER_BYTES];

DX_DECLARE_DEVICE_TWIN_HANDLER(set_channel_id_handler);
DX_DECLARE_DEVICE_TWIN_HANDLER(set_led_brightness_handler);
void publish_telemetry(int temperature, int pressure);
