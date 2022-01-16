/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "comms_manager_wolf.h"
#include "dx_azure_iot.h"
#include "dx_json_serializer.h"
#include "dx_utilities.h"
#include "sphere_panel.h"
#include "storage.h"

extern char msgBuffer[MSG_BUFFER_BYTES];
extern CPU_OPERATING_MODE cpu_operating_mode;
extern DX_TIMER_BINDING restartDeviceOneShotTimer;

void delay_restart_device_handler(EventLoopTimer* eventLoopTimer);
void device_twin_set_channel_id_handler(DX_DEVICE_TWIN_BINDING* deviceTwinBinding);
void device_twin_set_cpu_state_handler(DX_DEVICE_TWIN_BINDING* deviceTwinBinding);
void device_twin_set_led_brightness_handler(DX_DEVICE_TWIN_BINDING* deviceTwinBinding);
void publish_telemetry(int temperature, int pressure);
