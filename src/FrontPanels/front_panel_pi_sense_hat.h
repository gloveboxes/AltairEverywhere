#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "altair_panel.h"

#define NUM_OF_LEDS 64

extern ALTAIR_COMMAND cmd_switches;
extern uint16_t bus_switches;

bool sense_hat_front_panel_init(void);
void sense_hat_front_panel_shutdown(void);
void sense_hat_front_panel_io(uint8_t status, uint8_t data, uint16_t bus, void (*process_control_panel_commands)(void));
void sense_hat_set_led_panel_color(int color);
bool sense_hat_handle_led_matrix_output(int port_number, uint8_t data, char *buffer, size_t buffer_length, size_t *handled_length);
