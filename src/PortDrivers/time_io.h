/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "dx_async.h"
#include "dx_timer.h"
#include "stdint.h"
#include <stddef.h>



size_t time_output(int port, uint8_t data, char *buffer, size_t buffer_length);
uint8_t time_input(uint8_t port);
