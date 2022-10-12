/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#ifdef ALTAIR_FRONT_PANEL_KIT

#include <stdbool.h>
#include <stdint.h>
#include "altair_panel.h"

extern ALTAIR_COMMAND cmd_switches;
extern uint16_t bus_switches;

void update_panel_status_leds(uint8_t status, uint8_t data, uint16_t bus);
void read_altair_panel_switches(void (*process_control_panel_commands)(void));
void read_switches(uint16_t *address, uint8_t *cmd);
bool init_altair_hardware(void);

#endif