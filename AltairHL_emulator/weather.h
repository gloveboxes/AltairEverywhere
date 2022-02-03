/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "stdbool.h"
#include "location_from_ip.h"
#include <float.h>

typedef struct {
    int temperature;
    int humidity;
    int pressure;
    double latitude;
    double longitude;
    char country_code[10];
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
void GetCurrentWeather(struct location_info *locationInfo, WEATHER_TELEMETRY *telemetry);
