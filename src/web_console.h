/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "88dcdd.h"
#include "cpu_monitor.h"
#include "dx_async.h"
#include "dx_timer.h"
#include "dx_utilities.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <ws.h>

extern DX_ASYNC_BINDING async_expire_session;
extern DX_TIMER_BINDING tmr_deferred_input;

void print_console_banner(void);
void terminal_handler(char *data, size_t length);

DX_DECLARE_ASYNC_HANDLER(async_expire_session_handler);
DX_DECLARE_TIMER_HANDLER(ws_ping_pong_handler);

// Terminal input queue structure
typedef struct
{
    char buffer[256];
    size_t head;
    size_t tail;
    pthread_mutex_t mutex;
} TERMINAL_INPUT_QUEUE;

void init_web_socket_server(void (*client_connected_cb)(void));
void publish_character(char character);
void publish_message(const void *application_message, size_t application_message_length);
void set_cpu_operating_mode(CPU_OPERATING_MODE new_mode);

// Terminal input queue functions
void enqueue_terminal_input_character(char character);
char dequeue_terminal_input_character(void);
void clear_terminal_input_queue(void);
