#pragma once

#ifdef ALTAIR_FRONT_PANEL_PI_SENSE

#include "led_panel.h"
#include "altair_panel.h"

#define NUM_OF_LEDS 64

extern volatile ALTAIR_COMMAND cmd_switches;
extern volatile CPU_OPERATING_MODE cpu_operating_mode;
extern volatile uint16_t bus_switches;

void init_altair_hardware(void);
void update_panel_status_leds(uint8_t status, uint8_t data, uint16_t bus);
void set_led_panel_color(int color);

#endif // ALTAIR_FRONT_PANEL_PI_SENSE