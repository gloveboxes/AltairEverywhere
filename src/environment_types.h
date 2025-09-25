/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <stdbool.h>

typedef struct
{
	double lat;
	double lng;
	char city[60];
	char country[60];
	bool updated;
} LOCATION_T;

typedef struct
{
	float air_quality_index;
	float carbon_monoxide;
	float nitrogen_monoxide;
	float nitrogen_dioxide;
	float ozone;
	float sulphur_dioxide;
	float ammonia;
	float pm2_5;
	float pm10;
	bool updated;
} POLLUTION_T;

typedef struct
{
	int temperature;
	int humidity;
	int pressure;
	char description[80];
	float wind_speed;
	int wind_direction;
	bool updated;
} WEATHER_T;

typedef struct
{
	WEATHER_T weather;
	POLLUTION_T pollution;
} SENSOR_T;

typedef struct
{
	SENSOR_T latest;
	SENSOR_T previous;
	LOCATION_T locationInfo;
	bool valid;
} ENVIRONMENT_TELEMETRY;
