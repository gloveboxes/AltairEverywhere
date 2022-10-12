/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <stdbool.h>

#define IN  0
#define OUT 1

#define LOW  0
#define HIGH 1

bool dx_gpioOpenOutput(int pin, bool state);
bool dx_gpioClose(int pin);
bool dx_gpioStateSet(int pin, bool state);
