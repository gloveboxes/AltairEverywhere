/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "dx_timer.h"
#include "dx_utilities.h"
#include "environment_types.h"
#include "iotc_manager.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define COPYX_FOLDER_NAME "CopyX"

#define LOAD_PORT_DATA(name, format)                 \
	if (!reading_data)                               \
	{                                                \
		readPtr = 0;                                 \
		snprintf(data, sizeof(data), #format, name); \
		reading_data = true;                         \
	}                                                \
	if (readPtr < sizeof(data))                      \
	{                                                \
		retVal = data[readPtr++];                    \
	}                                                \
	else                                             \
	{                                                \
		retVal = 0x00;                               \
	}

#define LOAD_PORT_DATA_FROM_STRING(name) \
	if (!reading_data)                   \
	{                                    \
		string_len   = strlen(name);     \
		readPtr      = 0;                \
		reading_data = true;             \
	}                                    \
	if (readPtr < string_len)            \
	{                                    \
		retVal = name[readPtr++];        \
	}                                    \
	else                                 \
	{                                    \
		retVal = 0x00;                   \
	}

DX_DECLARE_TIMER_HANDLER(port_timer_expired_handler);
DX_DECLARE_TIMER_HANDLER(port_out_json_handler);
DX_DECLARE_TIMER_HANDLER(port_out_weather_handler);
DX_DECLARE_TIMER_HANDLER(tick_count_handler);

extern DX_TIMER_BINDING tmr_port_timer_expired;
extern DX_TIMER_BINDING tmr_deferred_port_out_weather;
extern DX_TIMER_BINDING tmr_deferred_port_out_json;

uint8_t io_port_in(uint8_t port);
void io_port_out(uint8_t port, uint8_t data);