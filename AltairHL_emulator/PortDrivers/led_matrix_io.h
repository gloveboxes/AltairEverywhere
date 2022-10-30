/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "dx_async.h"
#include <stddef.h>
#include <stdint.h>

DX_DECLARE_ASYNC_HANDLER(async_start_panel_io_handler);
DX_DECLARE_ASYNC_HANDLER(async_stop_panel_io_handler);

enum PANEL_MODE_T
{
    PANEL_BUS_MODE,
    PANEL_FONT_MODE,
    PANEL_BITMAP_MODE
};

extern enum PANEL_MODE_T panel_mode;

size_t led_matrix_output(int port_number, uint8_t data, char *buffer, size_t buffer_length);
