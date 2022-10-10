/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "power_io.h"

#ifdef AZURE_SPHERE

void altair_sleep(void);
void altair_wake(void);

#endif

static int wake_delay;

DX_TIMER_HANDLER(tmr_i8080_wakeup_handler)
{
#ifdef AZURE_SPHERE
    altair_wake();
#endif
}
DX_TIMER_HANDLER_END

DX_ASYNC_HANDLER(async_power_management_enable_handler, handle)
{
#ifdef OEM_AVNET
    dx_timerStart(&tmr_terminal_io_monitor);
#endif // OEM_AVNET
}
DX_ASYNC_HANDLER_END

DX_ASYNC_HANDLER(async_power_management_disable_handler, handle)
{
#ifdef OEM_AVNET
    dx_timerStop(&tmr_terminal_io_monitor);
#endif // OEM_AVNET
}
DX_ASYNC_HANDLER_END

DX_ASYNC_HANDLER(async_power_management_sleep_handler, handle)
{
#ifdef OEM_AVNET
    altair_sleep();
#endif // OEM_AVNET
}
DX_ASYNC_HANDLER_END

DX_ASYNC_HANDLER(async_power_management_wake_handler, handle)
{
#ifdef OEM_AVNET
    dx_timerOneShotSet(&tmr_i8080_wakeup, &(struct timespec){*((int *)handle->data), 0});
#endif // OEM_AVNET
}
DX_ASYNC_HANDLER_END

size_t power_output(int port, int data, char *buffer, size_t buffer_length)
{
    size_t len = 0;

    switch (port)
    {
#ifdef AZURE_SPHERE
        case 66: // enable/disable/sleep power management

            switch (data)
            {
                case 0:
                    dx_asyncSend(&async_power_management_disable, NULL);
                    break;
                case 1:
                    dx_asyncSend(&async_power_management_enable, NULL);
                    break;
                case 2:
                    dx_asyncSend(&async_power_management_sleep, NULL);
                    break;
                default:
                    break;
            }

            break;

        case 67: // wake from sleep in X seconds
            if (data > 0)
            {
                wake_delay = data;
                dx_asyncSend(&async_power_management_wake, (void *)&wake_delay);
            }
            break;

#endif // AZURE SPHERE
    }

    return len;
}

uint8_t power_input(uint8_t port)
{
    uint8_t retVal = 0;

    switch (port)
    {
    }

    return retVal;
}