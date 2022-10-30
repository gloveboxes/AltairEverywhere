/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "dx_async.h"
#include <stddef.h>
#include <stdint.h>

DX_DECLARE_ASYNC_HANDLER(async_copyx_request_handler);

extern DX_ASYNC_BINDING async_copyx_request;

size_t file_output(int port, uint8_t data, char *buffer, size_t buffer_length);
uint8_t file_input(uint8_t port);