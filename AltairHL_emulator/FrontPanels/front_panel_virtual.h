/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "altair_panel.h"
#include "mqtt_manager.h"
#include "utils.h"
#include <stdbool.h>
#include <string.h>
#include "intel8080.h"

extern volatile ALTAIR_COMMAND cmd_switches;
extern volatile uint16_t bus_switches;

void disassemble(intel8080_t *cpu);
void process_virtual_input(const char* command, void (*process_control_panel_commands)(void));
void publish_cpu_state(char* command, uint16_t address_bus, uint8_t data_bus);
void trace(intel8080_t *cpu);