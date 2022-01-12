#pragma once

/*
 *  C code to demonstrate control of the LED matrix for the
 *  Raspberry Pi Sense HAT add-on board.
 *
 *  Uses the mmap method to map the led device into memory
 *
 *  Build with:  gcc -Wall -O2 led_matrix.c -o led_matrix
 *               or just 'make'
 *
 *  Tested with:  Sense HAT v1.0 / Raspberry Pi 3 B+ / Raspbian GNU/Linux 10 (buster)
 *
 */

#ifdef ALTAIR_FRONT_PANEL_PI_SENSE

#include <fcntl.h>
#include <linux/fb.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "sphere_panel.h"

#define FILEPATH "/dev/fb0"
#define NUM_WORDS 64
#define FILESIZE (NUM_WORDS * sizeof(uint16_t))

#define RGB565_RED 0xF800

extern ALTAIR_COMMAND cmd_switches;
extern bool renderText;
extern char msgBuffer[MSG_BUFFER_BYTES];
extern CPU_OPERATING_MODE cpu_operating_mode;
extern uint16_t bus_switches;

void check_click_4x4key_mode_button(void);
void init_altair_hardware(void);
void read_altair_panel_switches(void (*process_control_panel_commands)(void));
void update_panel_status_leds(uint8_t status, uint8_t data, uint16_t bus);

#endif // ALTAIR_FRONT_PANEL_PI_SENSE