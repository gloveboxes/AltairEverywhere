/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "88dcdd.h"
#include "cpu_monitor.h"
#include "dx_device_twins.h"
#include "dx_utilities.h"
#include <stdbool.h>
#include <stdio.h>
#include <ws.h>

extern volatile bool send_partial_msg;
extern DX_TIMER_BINDING tmr_deferred_input;

void print_console_banner(void);

typedef struct
{
	bool active;
	size_t length;
	char buffer[256];
} WS_INPUT_BLOCK_T;

extern DX_DEVICE_TWIN_BINDING dt_new_sessions;
extern volatile CPU_OPERATING_MODE cpu_operating_mode;
extern WS_INPUT_BLOCK_T ws_input_block;
extern DX_TIMER_BINDING tmr_partial_message;

DX_DECLARE_TIMER_HANDLER(deferred_input_handler);
DX_DECLARE_TIMER_HANDLER(partial_message_handler);

void init_web_socket_server(void (*client_connected_cb)(void));
void publish_character(char character);
void publish_message(const void *application_message, size_t application_message_length);
void send_partial_message(void);