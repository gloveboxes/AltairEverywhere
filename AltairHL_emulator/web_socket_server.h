/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "88dcdd.h"
#include "cpu_monitor.h"
#include "dx_async.h"
#include "dx_device_twins.h"
#include "dx_timer.h"
#include "dx_utilities.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <ws.h>

extern DX_ASYNC_BINDING async_expire_session;
extern DX_ASYNC_BINDING async_terminal;
extern DX_TIMER_BINDING tmr_deferred_input;
extern bool send_partial_msg;

void print_console_banner(void);

typedef struct
{
	size_t length;
	char buffer[512];
	pthread_mutex_t block_lock;
} WS_INPUT_BLOCK_T;

extern DX_DEVICE_TWIN_BINDING dt_new_sessions;
extern CPU_OPERATING_MODE cpu_operating_mode;
extern DX_TIMER_BINDING tmr_partial_message;

DX_DECLARE_ASYNC_HANDLER(async_expire_session_handler);
DX_DECLARE_ASYNC_HANDLER(async_terminal_handler);
DX_DECLARE_TIMER_HANDLER(partial_message_handler);
DX_DECLARE_TIMER_HANDLER(ws_ping_pong_handler);

void init_web_socket_server(void (*client_connected_cb)(void));
void publish_character(char character);
void publish_message(const void *application_message, size_t application_message_length);
void send_partial_message(void);