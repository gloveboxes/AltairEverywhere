/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <applibs/applications.h>
#include <stdio.h>

// #define SHOW_DEBUG_MSGS 1

#define FLOAT_TO_INT(x) ((x) >= 0 ? (int)((x) + 0.5) : (int)((x)-0.5))

char *uint8_to_binary(uint8_t bitmap, char *buffer, size_t buffer_length);
void delay(int ms);
void DumpBuffer(uint8_t *buffer, uint16_t length);
char *get_i8080_instruction_name(uint8_t opcode, uint8_t *i8080_opcode_size);
char *log_memory_usage(char *buffer, size_t buffer_size, const char *message);

