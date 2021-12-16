/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "dx_terminate.h"
#include "dx_utilities.h"
#include <applibs/log.h>
#include <ctype.h>
#include <getopt.h>
#include <string.h>

/// <summary>
/// Connection types to use when connecting to the Azure IoT Hub.
/// </summary>
typedef enum {
    DX_CONNECTION_TYPE_NOT_DEFINED = 0,
    DX_CONNECTION_TYPE_DPS = 1,
    DX_CONNECTION_TYPE_HOSTNAME = 2
} ConnectionType;

typedef struct {
    const char* idScope;
    const char* hostname;
    ConnectionType connectionType;
} DX_USER_CONFIG;


bool dx_configParseCmdLineArguments(int argc, char* argv[], DX_USER_CONFIG *userConfig);
