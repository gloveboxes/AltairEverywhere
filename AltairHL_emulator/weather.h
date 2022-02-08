/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "location_from_ip.h"
#include "stdbool.h"
#include <ctype.h>
#include <float.h>
#include <getopt.h>
#include <string.h>

typedef struct {
    int temperature;
    int humidity;
    int pressure;
    char description[80];
    
} SENSOR_T;

typedef struct {
    SENSOR_T latest;
    SENSOR_T previous;
    bool validated;
} ENVIRONMENT_T;

typedef struct {
    SENSOR_T latest;
    SENSOR_T previous;
    struct location_info *locationInfo;
    bool valid;
} WEATHER_TELEMETRY;

// /*************************************************************
// * Description
// *    Get the current weather
// * Parameter
// *    location_info * - contains country code, city code
// *                      can determine degC/F from country code.
// * Return
// *    char *: current weather description
// *************************************************************/
// TODO: allow for location customization.
void GetCurrentWeather(WEATHER_TELEMETRY *telemetry);
void init_open_weather_map_api_key(int argc, char *argv[]);