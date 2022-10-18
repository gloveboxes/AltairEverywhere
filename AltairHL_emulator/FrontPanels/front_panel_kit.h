/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#ifdef ALTAIR_FRONT_PANEL_KIT

#include <stdbool.h>
#include <stdint.h>
#include "altair_panel.h"

extern ALTAIR_COMMAND cmd_switches;
extern uint16_t bus_switches;

void front_panel_io(uint8_t status, uint8_t data, uint16_t bus, void (*process_control_panel_commands)(void));
bool init_altair_hardware(void);

#endif