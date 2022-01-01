/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#ifndef SOCKET_LOG
#define SOCKET_LOG

#include "utils.h"

// only define Log_Debug if USE_SOCKET_LOG is enabled.

int Log_Debug(const char *fmt, ...);

#endif