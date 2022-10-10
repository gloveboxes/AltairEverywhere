/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "dx_async.h"
#include "iotc_manager.h"
#include <stddef.h>
#include <stdint.h>

DX_DECLARE_ASYNC_HANDLER(async_publish_json_handler);
DX_DECLARE_ASYNC_HANDLER(async_publish_weather_handler);

extern DX_ASYNC_BINDING async_publish_json;
extern DX_ASYNC_BINDING async_publish_weather;

uint8_t weather_input(uint8_t port_number);
int weather_output(int port_number, int data, char *buffer, size_t buffer_length);
