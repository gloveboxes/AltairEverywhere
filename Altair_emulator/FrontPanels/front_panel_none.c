/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "front_panel_none.h"

bool init_altair_hardware(void) {
	return true;
}

void read_altair_panel_switches(void (*process_control_panel_commands)(void)) {

}

void update_panel_status_leds(uint8_t status, uint8_t data, uint16_t bus) {

}