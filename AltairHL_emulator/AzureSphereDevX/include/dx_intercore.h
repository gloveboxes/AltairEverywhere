/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "dx_terminate.h"
#include "dx_timer.h"
#include "eventloop_timer_utilities.h"
#include <applibs/application.h>
#include <applibs/eventloop.h>
#include <applibs/log.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

typedef struct {
	char* rtAppComponentId;
    int sockFd;
	bool initialized;
	bool nonblocking_io;
	void (*interCoreCallback)(void*, ssize_t message_size);
	void* intercore_recv_block;
	size_t intercore_recv_block_length;
} DX_INTERCORE_BINDING;

bool dx_intercorePublish(DX_INTERCORE_BINDING* intercore_binding, void* control_block, size_t message_length);
ssize_t dx_intercorePublishThenRead(DX_INTERCORE_BINDING *intercore_binding, void *control_block, size_t message_length);
bool dx_intercoreConnect(DX_INTERCORE_BINDING *intercore_binding);
bool dx_intercorePublishThenReadTimeout(DX_INTERCORE_BINDING *intercore_binding, suseconds_t timeoutInMicroseconds);
