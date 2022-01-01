/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#ifdef ALTAIR_FRONT_PANEL_KIT

#include "app_exit_codes.h"
#include "dx_gpio.h"
#include "dx_utilities.h"
#include "hw/azure_sphere_learning_path.h"
#include "sphere_panel.h"
#include <applibs/spi.h>
#include <applibs/uart.h>
#include <applibs/uart.h>
#include <errno.h>
#include <soc/mt3620_uarts.h>

extern ALTAIR_COMMAND cmd_switches;
extern int altair_spi_fd;
extern int console_fd;
extern DX_GPIO_BINDING led_store;
extern DX_GPIO_BINDING switches_chip_select;
extern DX_GPIO_BINDING switches_load;
extern uint16_t bus_switches;
extern const uint8_t reverse_lut[16];


bool init_altair_hardware(void);
int init_console(void);
void read_altair_panel_switches(void (*process_control_panel_commands)(void));
void read_switches(uint16_t* address, uint8_t* cmd);
void update_panel_status_leds(uint8_t status, uint8_t data, uint16_t bus);

#endif // ALTAIR_FRONT_PANEL_KIT