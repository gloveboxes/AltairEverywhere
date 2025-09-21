#pragma once

#ifdef ALTAIR_FRONT_PANEL_PI_SENSE

#include "led_panel.h"
#include "altair_panel.h"
#include "sense_hat.h"

#define NUM_OF_LEDS 64

extern ALTAIR_COMMAND cmd_switches;
extern uint16_t bus_switches;

void init_altair_hardware(void);
void front_panel_io(uint8_t status, uint8_t data, uint16_t bus, void (*process_control_panel_commands)(void));
void set_led_panel_color(int color);

#endif // ALTAIR_FRONT_PANEL_PI_SENSE