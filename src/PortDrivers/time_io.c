/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "time_io.h"

#include "dx_utilities.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static volatile bool delay_milliseconds_enabled = false;
static volatile bool delay_seconds_enabled      = false;
static int timer_milliseconds_delay             = 0;
static int timer_delay                          = 0;
// set tick_count to 1 as the tick count timer doesn't kick in until 1 second after startup
static uint32_t tick_count = 1;

DX_TIMER_HANDLER(tick_count_handler)
{
    tick_count++;
}
DX_TIMER_HANDLER_END

DX_TIMER_HANDLER(timer_seconds_expired_handler)
{
    delay_seconds_enabled = false;
}
DX_TIMER_HANDLER_END

DX_TIMER_HANDLER(timer_millisecond_expired_handler)
{
    delay_milliseconds_enabled = false;
}
DX_TIMER_HANDLER_END

DX_ASYNC_HANDLER(async_set_timer_seconds_handler, handle)
{
    int data = *((int *)handle->data);
    dx_timerOneShotSet(&tmr_timer_seconds_expired, &(struct timespec){data, 0});
}
DX_ASYNC_HANDLER_END

DX_ASYNC_HANDLER(async_set_timer_millisecond_handler, handle)
{
    int data = *((int *)handle->data);
    dx_timerOneShotSet(&tmr_timer_millisecond_expired, &(struct timespec){0, data * ONE_MS});
}
DX_ASYNC_HANDLER_END

size_t time_output(int port, uint8_t data, char *buffer, size_t buffer_length)
{
    size_t len = 0;

    switch (port)
    {
        case 29: // Set milliseconds timer
            if (!delay_milliseconds_enabled)
            {
                delay_milliseconds_enabled = true;
                timer_milliseconds_delay   = data;
                dx_asyncSend(&async_set_millisecond_timer, (void *)&timer_milliseconds_delay);
            }
            break;
        case 30: // set seconds timer
            if (!delay_seconds_enabled)
            {
                delay_seconds_enabled = true;
                timer_delay           = data;
                dx_asyncSend(&async_set_seconds_timer, (void *)&timer_delay);
            }
            break;
        case 41: // System tick count
            len = (size_t)snprintf(buffer, buffer_length, "%u", tick_count);
            break;
        case 42: // get utc date and time
            dx_getCurrentUtc(buffer, buffer_length);
            len = strnlen(buffer, buffer_length);
            break;
        case 43: // get local date and time
#ifdef AZURE_SPHERE
            dx_getCurrentUtc(buffer, buffer_length);
#else
            dx_getLocalTime(buffer, buffer_length);
#endif
            len = strnlen(buffer, buffer_length);
            break;
    }

    return len;
}

uint8_t time_input(uint8_t port)
{
    uint8_t retVal = 0;
    switch (port)
    {
        case 29: // Has milliseconds timer expired
            retVal = delay_milliseconds_enabled;
            break;
        case 30: // Has seconds timer expired
            retVal = delay_seconds_enabled;
            break;
    }
    return retVal;
}