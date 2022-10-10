/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <stdint.h>
#include <stddef.h>

enum PANEL_MODE_T
{
    PANEL_BUS_MODE,
    PANEL_FONT_MODE,
    PANEL_BITMAP_MODE
};

extern enum PANEL_MODE_T panel_mode;

size_t led_matrix_output(int port_number, int data, char* buffer, size_t buffer_length);
