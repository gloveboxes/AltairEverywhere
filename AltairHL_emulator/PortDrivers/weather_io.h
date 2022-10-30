/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <stddef.h>
#include <stdint.h>

size_t weather_output(int port_number, uint8_t data, char *buffer, size_t buffer_length);
