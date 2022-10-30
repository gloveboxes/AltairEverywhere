/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "dx_async.h"
#include "dx_gpio.h"
#include "dx_timer.h"
#include <stddef.h>
#include <stdint.h>

#ifdef AZURE_SPHERE
#include "dx_intercore.h"
#include "intercore_contract.h"
#endif

DX_DECLARE_ASYNC_HANDLER(async_accelerometer_start_handler);
DX_DECLARE_ASYNC_HANDLER(async_accelerometer_stop_handler);
DX_DECLARE_TIMER_HANDLER(read_accelerometer_handler);

extern DX_ASYNC_BINDING async_accelerometer_start;
extern DX_ASYNC_BINDING async_accelerometer_stop;
extern DX_TIMER_BINDING tmr_read_accelerometer;

#ifdef AZURE_SPHERE

extern DX_GPIO_BINDING gpioRed;
extern DX_GPIO_BINDING gpioGreen;
extern DX_GPIO_BINDING gpioBlue;
extern INTERCORE_ML_CLASSIFY_BLOCK_T intercore_ml_classify_block;
extern DX_INTERCORE_BINDING intercore_ml_classify_ctx;

void intercore_classify_response_handler(void *data_block, ssize_t message_length);

#endif

size_t onboard_output(int port, uint8_t data, char *buffer, size_t buffer_length);
uint8_t onboard_input(uint8_t port);