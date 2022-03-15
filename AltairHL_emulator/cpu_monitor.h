/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "altair_panel.h"
#include "dx_timer.h"
#include "intel8080.h"
#include "utils.h"
#include "web_socket_server.h"
#include <applibs/log.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#define DISK_A           "Disks/cpm63k.dsk"
#define DISK_B           "Disks/blank.dsk"
#define DISK_LOADER      "Disks/88dskrom.bin"
#define ALTAIR_BASIC_ROM "Disks/altair_basic.bin"

extern DX_TIMER_BINDING tmr_deferred_command;
extern intel8080_t cpu;
extern uint8_t memory[64 * 1024];
extern volatile ALTAIR_COMMAND cmd_switches;
extern volatile CPU_OPERATING_MODE cpu_operating_mode;
extern volatile uint16_t bus_switches;

DX_DECLARE_TIMER_HANDLER(deferred_command_handler);

void disassemble(intel8080_t *cpu);
void process_virtual_input(const char *command, void (*process_control_panel_commands)(void));
void publish_cpu_state(char *command, uint16_t address_bus, uint8_t data_bus);
void trace(intel8080_t *cpu);
void process_control_panel_commands(void);
bool loadRomImage(char *romImageName, uint16_t loadAddress);