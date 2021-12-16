/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

void delay(int ms);
void delayMicroseconds(int us);
void ListI2CDevices(int isuFd);
