/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "comms_manager_wolf.h"
#include "dx_azure_iot.h"
#include "dx_utilities.h"
#include "sphere_panel.h"
#include "storage.h"
#include <applibs/applications.h>
#include <applibs/powermanagement.h>

extern bool local_serial;
extern char msgBuffer[MSG_BUFFER_BYTES];
extern CPU_OPERATING_MODE cpu_operating_mode;
extern DX_TIMER_BINDING restartDeviceOneShotTimer;
extern uint16_t cpu_speed;

#ifdef ALTAIR_FRONT_PANEL_CLICK

#include "max7219.h"
extern matrix8x8_t panel8x8;

#endif // ALTAIR_FRONT_PANEL_CLICK

#ifdef ALTAIR_FRONT_PANEL_RETRO_CLICK
#include "as1115.h"
extern as1115_t retro_click;
#endif //  ALTAIR_FRONT_PANEL_RETRO_CLICK



DX_DIRECT_METHOD_RESPONSE_CODE RestartDeviceHandler(JSON_Value* json, DX_DIRECT_METHOD_BINDING* directMethodBinding, char** responseMsg);
void delay_restart_device_handler(EventLoopTimer* eventLoopTimer);
void device_twin_set_channel_id_handler(DX_DEVICE_TWIN_BINDING* deviceTwinBinding);
void device_twin_set_cpu_state_handler(DX_DEVICE_TWIN_BINDING* deviceTwinBinding);
void device_twin_set_led_brightness_handler(DX_DEVICE_TWIN_BINDING* deviceTwinBinding);
void device_twin_set_local_serial_handler(DX_DEVICE_TWIN_BINDING* deviceTwinBinding);
void memory_diagnostics_handler(EventLoopTimer* eventLoopTimer);
void publish_telemetry(int temperature, int pressure);
