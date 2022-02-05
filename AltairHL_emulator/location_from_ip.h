/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "httpGet.h"

struct location_info {
    char *countryCode;
    char *country;
    char *region;
    char *city;
    double lat;
    double lng;
    bool updated;
};

struct location_info *GetLocationData(void);
