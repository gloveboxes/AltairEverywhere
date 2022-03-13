/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "dx_utilities.h"
#include <stdbool.h>
#include <stdio.h>
#include <ws.h>

extern volatile bool send_partial_msg;
extern DX_TIMER_BINDING tmr_deferred_input;

void print_console_banner(void);

typedef struct {
    bool active;
    size_t length;
    char buffer[256];    
} WEB_SOCKET_INPUT_T;

WEB_SOCKET_INPUT_T web_socket_input_buffer;

DX_DECLARE_TIMER_HANDLER(deferred_input_handler);
DX_DECLARE_TIMER_HANDLER(partial_message_handler);

void init_web_socket_server(void (*client_connected_cb)(void));
void publish_character(char character);
void publish_message(const void *application_message, size_t application_message_length);
void send_partial_message(void);