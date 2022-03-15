/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "dx_utilities.h"
#include "environment_types.h"

typedef struct
{
	double lat;
	double lng;
	bool updated;
} location_info;

void get_geolocation(LOCATION_T *locationInfo);
