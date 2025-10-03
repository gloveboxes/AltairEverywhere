/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool none_front_panel_init(void);
void none_front_panel_shutdown(void);
void none_front_panel_io(uint8_t status, uint8_t data, uint16_t bus, void (*process_control_panel_commands)(void));
void none_front_panel_set_led_color(int color);
bool none_front_panel_handle_led_matrix_output(int port_number, uint8_t data, char *buffer, size_t buffer_length, size_t *handled_length);
