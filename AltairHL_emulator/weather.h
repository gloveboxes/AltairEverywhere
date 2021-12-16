/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "stdbool.h"
#include "location_from_ip.h"
#include <float.h>

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
char* GetCurrentWeather(struct location_info* locationInfo, float* Temperature);
