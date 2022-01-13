#pragma once

#ifdef ALTAIR_FRONT_PANEL_PI_SENSE

#include "led_panel.h"
#include "sphere_panel.h"

#define NUM_OF_LEDS 64
#define RGB565_RED 0xF800

extern ALTAIR_COMMAND cmd_switches;
extern CPU_OPERATING_MODE cpu_operating_mode;
extern uint16_t bus_switches;

void check_click_4x4key_mode_button(void);
void init_altair_hardware(void);
void read_altair_panel_switches(void (*process_control_panel_commands)(void));
void update_panel_status_leds(uint8_t status, uint8_t data, uint16_t bus);

#endif // ALTAIR_FRONT_PANEL_PI_SENSE