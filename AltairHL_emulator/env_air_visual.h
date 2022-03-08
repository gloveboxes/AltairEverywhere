#pragma once

/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "altair_config.h"
#include "dx_utilities.h"
#include "parson.h"
#include "utils.h"
#include <applibs/log.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "environment_types.h"



void update_air_visual(ENVIRONMENT_TELEMETRY *environment);
void init_air_visual_service(ALTAIR_CONFIG_T *altair_config, ENVIRONMENT_TELEMETRY *environment);