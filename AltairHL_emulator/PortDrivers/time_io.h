/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "dx_async.h"
#include "dx_timer.h"
#include "stdint.h"
#include <stddef.h>

DX_DECLARE_ASYNC_HANDLER(async_set_timer_millisecond_handler);
DX_DECLARE_ASYNC_HANDLER(async_set_timer_seconds_handler);
DX_DECLARE_TIMER_HANDLER(tick_count_handler);
DX_DECLARE_TIMER_HANDLER(timer_millisecond_expired_handler);
DX_DECLARE_TIMER_HANDLER(timer_seconds_expired_handler);

extern DX_ASYNC_BINDING async_set_millisecond_timer;
extern DX_ASYNC_BINDING async_set_seconds_timer;
extern DX_TIMER_BINDING tmr_timer_millisecond_expired;
extern DX_TIMER_BINDING tmr_timer_seconds_expired;

size_t time_output(int port, uint8_t data, char *buffer, size_t buffer_length);
uint8_t time_input(uint8_t port);
