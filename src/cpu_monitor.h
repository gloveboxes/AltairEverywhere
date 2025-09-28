/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "altair_panel.h"
#include "dx_timer.h"
#include "intel8080.h"
#include "utils.h"
#include "web_console.h"
#include <applibs/log.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#define DISK_A_RO        "Disks/azsphere_cpm63k.dsk"
#define DISK_A           "Disks/cpm63k.dsk"
#define DISK_B           "Disks/bdsc-v1.60.dsk"
#define DISK_C           "Disks/escape-posix.dsk"
#define DISK_D           "Disks/blank.dsk"
#define DISK_LOADER      "Disks/88dskrom.bin"
#define ALTAIR_BASIC_ROM "Disks/altair_basic.bin"

extern DX_TIMER_BINDING tmr_deferred_command;
extern intel8080_t cpu;
extern uint8_t memory[64 * 1024];
extern ALTAIR_COMMAND cmd_switches;
extern uint16_t bus_switches;

// Thread-safe CPU operating mode accessors (defined in main.c)
CPU_OPERATING_MODE get_cpu_operating_mode_fast(void);
void clear_terminal_input_queue(void);
void set_cpu_operating_mode(CPU_OPERATING_MODE new_mode);
CPU_OPERATING_MODE toggle_cpu_operating_mode(void);

bool loadRomImage(char *romImageName, uint16_t loadAddress);
void disassemble(intel8080_t *cpu);
void load_boot_disk(void);
void process_control_panel_commands(void);
void process_virtual_input(const char *command);
void publish_cpu_state(char *command, uint16_t address_bus, uint8_t data_bus);
void trace(intel8080_t *cpu);
