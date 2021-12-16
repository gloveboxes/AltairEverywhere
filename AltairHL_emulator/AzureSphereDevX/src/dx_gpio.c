/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "dx_gpio.h"

bool dx_gpioOpen(DX_GPIO_BINDING *peripheral)
{
    if (peripheral == NULL || peripheral->pin < 0) {
        return false;
    }

    if (peripheral->opened) {
        return true;
    }

    if (peripheral->invertPin) {
        if (peripheral->initialState == GPIO_Value_High) {
            peripheral->initialState = GPIO_Value_Low;
        } else {
            peripheral->initialState = GPIO_Value_High;
        }
    }

    switch (peripheral->direction) {
    case DX_OUTPUT:
        peripheral->fd =
            GPIO_OpenAsOutput(peripheral->pin, GPIO_OutputMode_PushPull, peripheral->initialState);
        if (peripheral->fd < 0) {
            Log_Debug(
                "Error opening GPIO: %s (%d). Check that app_manifest.json includes the GPIO "
                "used.\n",
                strerror(errno), errno);
            dx_terminate(DX_ExitCode_Gpio_Open_Output_Failed);
            return false;
        }
        break;
    case DX_INPUT:
        peripheral->fd = GPIO_OpenAsInput(peripheral->pin);
        if (peripheral->fd < 0) {
            Log_Debug(
                "Error opening GPIO: %s (%d). Check that app_manifest.json includes the GPIO "
                "used.\n",
                strerror(errno), errno);
            dx_terminate(DX_ExitCode_Gpio_Open_Input_Failed);
            return false;
        }
        break;
    case DX_DIRECTION_UNKNOWN:
        Log_Debug("Unknown direction for peripheral %s", peripheral->name);
        dx_terminate(DX_ExitCode_Gpio_Open_Direction_Unknown);
        return false;
        break;
    }

    peripheral->opened = true;
    return true;
}

void dx_gpioSetOpen(DX_GPIO_BINDING **gpioSet, size_t gpioSetCount)
{
    for (int i = 0; i < gpioSetCount; i++) {
        if (!dx_gpioOpen(gpioSet[i])) {
            break;
        }
    }
}

/// <summary>
///     Closes a file descriptor and prints an error on failure.
/// </summary>
/// <param name="fd">File descriptor to close</param>
/// <param name="fdName">File descriptor name to use in error message</param>
void dx_gpioClose(DX_GPIO_BINDING *peripheral)
{
    if (peripheral->opened && peripheral->fd >= 0) {
        int result = close(peripheral->fd);
        if (result != 0) {
            Log_Debug("ERROR: Could not close peripheral %s: %s (%d).\n",
                      peripheral->name == NULL ? "No name" : peripheral->name, strerror(errno),
                      errno);
        }
    }
    peripheral->fd = -1;
    peripheral->opened = false;
}

void dx_gpioSetClose(DX_GPIO_BINDING **gpioSet, size_t gpioSetCount)
{
    for (int i = 0; i < gpioSetCount; i++) {
        dx_gpioClose(gpioSet[i]);
    }
}

void dx_gpioOn(DX_GPIO_BINDING *peripheral)
{
    if (peripheral == NULL || !peripheral->opened) {
        dx_terminate(DX_ExitCode_Gpio_Not_Initialized);
        return;
    }

    GPIO_SetValue(peripheral->fd, peripheral->invertPin ? GPIO_Value_Low : GPIO_Value_High);
}

void dx_gpioOff(DX_GPIO_BINDING *peripheral)
{
    if (peripheral == NULL || !peripheral->opened) {
        dx_terminate(DX_ExitCode_Gpio_Not_Initialized);
        return;
    }

    GPIO_SetValue(peripheral->fd, peripheral->invertPin ? GPIO_Value_High : GPIO_Value_Low);
}

void dx_gpioStateSet(DX_GPIO_BINDING *peripheral, bool state)
{
    if (state) {
        dx_gpioOn(peripheral);
    } else {
        dx_gpioOff(peripheral);
    }
}

/// <summary>
/// Read Button DX_GPIO_BINDING returns state
/// </summary>
bool dx_gpioStateGet(DX_GPIO_BINDING *peripheral, GPIO_Value_Type *oldState)
{
    bool isGpioOn = false;
    GPIO_Value_Type newState;

    if (peripheral == NULL || !peripheral->opened) {
        dx_terminate(DX_ExitCode_Gpio_Not_Initialized);
        return false;
    }

    if (peripheral->direction != DX_INPUT) {
        dx_terminate(DX_ExitCode_Gpio_Wrong_Direction);
        return false;
    }

    if (GPIO_GetValue(peripheral->fd, &newState) != 0) {
        dx_terminate(DX_ExitCode_Gpio_Read);
    } else {
        switch (peripheral->detect) {
        case DX_GPIO_DETECT_LOW:
            isGpioOn = (newState != *oldState) && (newState == GPIO_Value_Low);
            break;
        case DX_GPIO_DETECT_HIGH:
            isGpioOn = (newState != *oldState) && (newState == GPIO_Value_High);
            break;
        case DX_GPIO_DETECT_BOTH:
            isGpioOn = (newState != *oldState);
            break;
        }

        *oldState = newState;
    }
    return isGpioOn;
}
