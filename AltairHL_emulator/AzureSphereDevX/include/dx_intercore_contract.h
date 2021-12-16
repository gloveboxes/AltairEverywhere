/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

// This is a sample Intercore contract only

#include <stdbool.h>
#include <stdint.h>

typedef enum __attribute__((packed)) {
	DX_IC_UNKNOWN,
	DX_IC_HEARTBEAT,
	DX_IC_ENVIRONMENT_SENSOR,
	DX_IC_SAMPLE_RATE
} DX_INTER_CORE_CMD;

typedef struct __attribute__((packed)) {
	DX_INTER_CORE_CMD cmd;
	float temperature;
	float pressure;
	float humidity;
	int sample_rate;
} DX_INTER_CORE_BLOCK;
