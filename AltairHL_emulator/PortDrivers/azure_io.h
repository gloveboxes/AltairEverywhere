#pragma once

#include "dx_async.h"
#include <stddef.h>
#include <stdint.h>

DX_DECLARE_ASYNC_HANDLER(async_publish_json_handler);
DX_DECLARE_ASYNC_HANDLER(async_publish_weather_handler);

extern DX_ASYNC_BINDING async_publish_json;
extern DX_ASYNC_BINDING async_publish_weather;

uint8_t azure_input(uint8_t port_number);
size_t azure_output(int port_number, uint8_t data, char *buffer, size_t buffer_length);
