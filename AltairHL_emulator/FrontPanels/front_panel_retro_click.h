/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#ifdef ALTAIR_FRONT_PANEL_RETRO_CLICK

#include "app_exit_codes.h"
#include "comms_manager_wolf.h"
#include "dx_gpio.h"
#include "dx_timer.h"
#include "dx_i2c.h"
#include "graphics.h"
#include "hw/azure_sphere_learning_path.h"
#include "as1115.h"
#include "sphere_panel.h"
#include "utils.h"
#include <applibs/gpio.h>
#include <stdio.h>


extern ALTAIR_COMMAND cmd_switches;
extern bool renderText;
extern char msgBuffer[MSG_BUFFER_BYTES];
extern CLICK_4X4_BUTTON_MODE click_4x4_key_mode;
extern CPU_OPERATING_MODE cpu_operating_mode;
extern int console_fd;
as1115_t retro_click;
extern DX_GPIO_BINDING buttonB;
extern DX_TIMER_BINDING turnOffNotificationsTimer;
extern uint16_t bus_switches;
extern DX_I2C_BINDING i2c_as1115_retro;


void check_click_4x4key_mode_button(void);
void init_altair_hardware(void);
void read_altair_panel_switches(void (*process_control_panel_commands)(void));
void turn_off_notifications_handler(EventLoopTimer* eventLoopTimer);
void update_panel_status_leds(uint8_t status, uint8_t data, uint16_t bus);

#endif // ALTAIR_FRONT_PANEL_RETRO_CLICK