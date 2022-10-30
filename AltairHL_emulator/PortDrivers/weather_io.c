/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "weather_io.h"

#include "dx_utilities.h"
#include "environment.h"
#include "environment_types.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static int format_double4(const void *value, char *buffer, size_t buffer_length);
static int format_float0(const void *value, char *buffer, size_t buffer_length);
static int format_float2(const void *value, char *buffer, size_t buffer_length);
static int format_int(const void *value, char *buffer, size_t buffer_length);
static int format_string(const void *value, char *buffer, size_t buffer_length);

// Weather definitions
static const void *w_key[] = {
    "Celsius",
    "Millibar",
    "Humidity %",
    "Wind km/h",
    "Wind degrees",
    "Observation",
};

static const void *w_value[] = {
    &environment.latest.weather.temperature,
    &environment.latest.weather.pressure,
    &environment.latest.weather.humidity,
    &environment.latest.weather.wind_speed,
    &environment.latest.weather.wind_direction,
    &environment.latest.weather.description,
};

static int (*w_formatter[])(const void *value, char *buffer, size_t buffer_length) = {
    format_int,
    format_int,
    format_int,
    format_float2,
    format_int,
    format_string,
};

// Location definitions
static const void *l_key[] = {
    "Latitude",
    "Longitude",
    "Country",
    "City",
};

static const void *l_value[] = {
    &environment.locationInfo.lat,
    &environment.locationInfo.lng,
    &environment.locationInfo.country,
    &environment.locationInfo.city,
};

static int (*l_formatter[])(const void *value, char *buffer, size_t buffer_length) = {
    format_double4,
    format_double4,
    format_string,
    format_string,
};

// Pollution defintions
static const void *p_key[] = {
    "AQI(CAQI)",
    "CO",
    "NO",
    "NO2",
    "O3",
    "SO2",
    "NH3",
    "PM2.5",
    "PM1.0",
};

static const void *p_value[] = {
    &environment.latest.pollution.air_quality_index,
    &environment.latest.pollution.carbon_monoxide,
    &environment.latest.pollution.nitrogen_monoxide,
    &environment.latest.pollution.nitrogen_dioxide,
    &environment.latest.pollution.ozone,
    &environment.latest.pollution.sulphur_dioxide,
    &environment.latest.pollution.ammonia,
    &environment.latest.pollution.pm2_5,
    &environment.latest.pollution.pm10,
};

static int (*p_formatter[])(const void *value, char *buffer, size_t buffer_length) = {
    format_float0,
    format_float2,
    format_float2,
    format_float2,
    format_float2,
    format_float2,
    format_float2,
    format_float2,
    format_float2,
};

static int format_float0(const void *value, char *buffer, size_t buffer_length)
{
    return snprintf(buffer, buffer_length, "%.0f", *(float *)value);
}

// static void format_float1(const void *value, char *buffer, size_t buffer_length)
// {
//     len = snprintf(buffer, buffer_length, "%.0f", *(float *)value);
// }

static int format_float2(const void *value, char *buffer, size_t buffer_length)
{
    return snprintf(buffer, buffer_length, "%.2f", *(float *)value);
}

static int format_double4(const void *value, char *buffer, size_t buffer_length)
{
    return snprintf(buffer, buffer_length, "%.4f", *(double *)value);
}

static int format_int(const void *value, char *buffer, size_t buffer_length)
{
    return snprintf(buffer, buffer_length, "%d", *(int *)value);
}

static int format_string(const void *value, char *buffer, size_t buffer_length)
{
    return snprintf(buffer, buffer_length, "%s", (char *)value);
}

size_t weather_output(int port_number, uint8_t data, char *buffer, size_t buffer_length)
{
    int len = 0;

    switch (port_number)
    {
        case 34: // Weather key
            if (data < NELEMS(w_key))
            {
                len = format_string(w_key[data], buffer, buffer_length);
            }
            break;
        case 35: // weather value
            if (environment.latest.weather.updated && data < NELEMS(w_value))
            {
                len = w_formatter[data](w_value[data], buffer, buffer_length);
            }
            break;
        case 36: // Location key
            if (data < NELEMS(l_key))
            {
                len = format_string(l_key[data], buffer, buffer_length);
            }
            break;
        case 37: // Location value
            if (environment.locationInfo.updated && data < NELEMS(l_value))
            {
                len = l_formatter[data](l_value[data], buffer, buffer_length);
            }
            break;
        case 38: // Pollution key
            if (data < NELEMS(p_key))
            {
                len = format_string(p_key[data], buffer, buffer_length);
            }
            break;
        case 39: // Pollution value
            if (environment.latest.pollution.updated && data < NELEMS(p_value))
            {
                len = p_formatter[data](p_value[data], buffer, buffer_length);
            }
            break;
        default:
            len = 0;
            break;
    }

    return (size_t)len;
}
