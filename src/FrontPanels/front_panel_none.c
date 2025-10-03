/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "front_panel_none.h"

#include <stddef.h>

bool none_front_panel_init(void)
{
    return true;
}

void none_front_panel_shutdown(void)
{
}

void none_front_panel_set_led_color(int color)
{
    (void)color;
}

bool none_front_panel_handle_led_matrix_output(int port_number, uint8_t data, char *buffer, size_t buffer_length, size_t *handled_length)
{
    (void)port_number;
    (void)data;
    (void)buffer;
    (void)buffer_length;
    if (handled_length)
    {
        *handled_length = 0;
    }
    return false;
}

void none_front_panel_io(uint8_t status, uint8_t data, uint16_t bus, void (*process_control_panel_commands)(void))
{
    (void)status;
    (void)data;
    (void)bus;
    (void)process_control_panel_commands;
}
