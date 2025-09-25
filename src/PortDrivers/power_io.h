/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "dx_async.h"
#include "dx_timer.h"
#include <stddef.h>
#include <stdint.h>

DX_DECLARE_ASYNC_HANDLER(async_power_management_disable_handler);
DX_DECLARE_ASYNC_HANDLER(async_power_management_enable_handler);
DX_DECLARE_ASYNC_HANDLER(async_power_management_sleep_handler);
DX_DECLARE_ASYNC_HANDLER(async_power_management_wake_handler);

DX_DECLARE_TIMER_HANDLER(tmr_i8080_wakeup_handler);

extern DX_ASYNC_BINDING async_power_management_disable;
extern DX_ASYNC_BINDING async_power_management_enable;
extern DX_ASYNC_BINDING async_power_management_sleep;
extern DX_ASYNC_BINDING async_power_management_wake;

extern DX_TIMER_BINDING tmr_i8080_wakeup;
extern DX_TIMER_BINDING tmr_terminal_io_monitor;

size_t power_output(int port, uint8_t data, char *buffer, size_t buffer_length);
uint8_t power_input(uint8_t port);