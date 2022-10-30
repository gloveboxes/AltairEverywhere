/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <stddef.h>
#include <stdint.h>

extern const char ALTAIR_EMULATOR_VERSION[];

size_t utility_output(int port, uint8_t data, char *buffer, size_t buffer_length);
uint8_t utility_input(uint8_t port);