/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "comms_manager_wolf.h"
#include "sphere_panel.h"
#include <string.h>
#include <stdbool.h>
#include "utils.h"

extern ALTAIR_COMMAND cmd_switches;
extern char msgBuffer[MSG_BUFFER_BYTES];
extern uint16_t bus_switches;
//extern pub_topic_data;

void process_virtual_input(const char* command, void (*process_control_panel_commands)(void));
void publish_cpu_state(char* command, uint16_t address_bus, uint8_t data_bus);