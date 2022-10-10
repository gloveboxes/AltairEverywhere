/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "weather_io.h"

#include "dx_utilities.h"
#include "environment_types.h"
#include <stdio.h>
#include <string.h>

DX_DECLARE_ASYNC_HANDLER(async_publish_json_handler);
DX_DECLARE_ASYNC_HANDLER(async_publish_weather_handler);

typedef struct
{
    char buffer[256];
    int index;
} JSON_UNIT_T;

// clang-format off
DX_MESSAGE_PROPERTY *json_msg_properties[] = {
    &(DX_MESSAGE_PROPERTY){.key = "appid", .value = "altair"},
    &(DX_MESSAGE_PROPERTY){.key = "type", .value = "custom"},
    &(DX_MESSAGE_PROPERTY){.key = "schema", .value = "1"}};

DX_MESSAGE_CONTENT_PROPERTIES json_content_properties = {
    .contentEncoding = "utf-8", .contentType = "application/json"};
// clang-format on

extern bool azure_connected;
extern ENVIRONMENT_TELEMETRY environment;

static JSON_UNIT_T ju;
static volatile bool publish_json_pending    = false;
static volatile bool publish_weather_pending = false;
static int len                               = 0;

static void format_double4(const void *value, char *buffer, size_t buffer_length);
static void format_float0(const void *value, char *buffer, size_t buffer_length);
static void format_float2(const void *value, char *buffer, size_t buffer_length);
static void format_int(const void *value, char *buffer, size_t buffer_length);
static void format_string(const void *value, char *buffer, size_t buffer_length);

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

static void (*w_formatter[])(const void *value, char *buffer, size_t buffer_length) = {
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

static void (*l_formatter[])(const void *value, char *buffer, size_t buffer_length) = {
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

static void (*p_formatter[])(const void *value, char *buffer, size_t buffer_length) = {
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

DX_ASYNC_HANDLER(async_publish_weather_handler, handle)
{
    if (environment.valid && azure_connected)
    {
#ifndef ALTAIR_CLOUD
        publish_telemetry(&environment);
#endif
    }
    publish_weather_pending = false;
}
DX_ASYNC_HANDLER_END

DX_ASYNC_HANDLER(async_publish_json_handler, handle)
{
    if (azure_connected)
    {
#ifndef ALTAIR_CLOUD
        dx_azurePublish(ju.buffer, strlen(ju.buffer), json_msg_properties, NELEMS(json_msg_properties), &json_content_properties);
#endif
    }
    publish_json_pending = false;
}
DX_ASYNC_HANDLER_END

static void format_float0(const void *value, char *buffer, size_t buffer_length)
{
    len = snprintf(buffer, buffer_length, "%.0f", *(float *)value);
}

static void format_float1(const void *value, char *buffer, size_t buffer_length)
{
    len = (size_t)snprintf(buffer, buffer_length, "%.0f", *(float *)value);
}

static void format_float2(const void *value, char *buffer, size_t buffer_length)
{
    len = (size_t)snprintf(buffer, buffer_length, "%.2f", *(float *)value);
}

static void format_double4(const void *value, char *buffer, size_t buffer_length)
{
    len = (size_t)snprintf(buffer, buffer_length, "%.4f", *(double *)value);
}

static void format_int(const void *value, char *buffer, size_t buffer_length)
{
    len = (size_t)snprintf(buffer, buffer_length, "%d", *(int *)value);
}

static void format_string(const void *value, char *buffer, size_t buffer_length)
{
    len = (size_t)snprintf(buffer, buffer_length, "%s", (char *)value);
}

int weather_output(int port_number, int data, char *buffer, size_t buffer_length)
{
    len = 0;

    switch (port_number)
    {
        case 31: // publish weather json
            if (!publish_json_pending)
            {
                if (ju.index == 0)
                {
                    memset((void *)ju.buffer, 0x00, sizeof(ju.buffer));
                }

                if (data != 0 && ju.index < sizeof(ju.buffer))
                {
                    ju.buffer[ju.index++] = data;
                }

                if (data == 0)
                {
                    publish_json_pending = true;
                    ju.index             = 0;
                    dx_asyncSend(&async_publish_json, NULL);
                }
            }
            break;
        case 32: // publish weather
            if (!publish_weather_pending)
            {
                publish_weather_pending = true;
                dx_asyncSend(&async_publish_weather, NULL);
            }
            break;
        case 34: // Weather key
            if (data < NELEMS(w_key))
            {
                format_string(w_key[data], buffer, buffer_length);
            }
            break;
        case 35: // weather value
            if (environment.latest.weather.updated && data < NELEMS(w_value))
            {
                w_formatter[data](w_value[data], buffer, buffer_length);
            }
            break;
        case 36: // Location key
            if (data < NELEMS(l_key))
            {
                format_string(l_key[data], buffer, buffer_length);
            }
            break;
        case 37: // Location value
            if (environment.locationInfo.updated && data < NELEMS(l_value))
            {
                l_formatter[data](l_value[data], buffer, buffer_length);
            }
            break;
        case 38: // Pollution key
            if (data < NELEMS(p_key))
            {
                format_string(p_key[data], buffer, buffer_length);
            }
            break;
        case 39: // Pollution value
            if (environment.latest.pollution.updated && data < NELEMS(p_value))
            {
                p_formatter[data](p_value[data], buffer, buffer_length);
            }
            break;
        default:
            len = 0;
            break;
    }

    return len;
}

uint8_t weather_input(uint8_t port_number)
{
    int retVal = 0;

    switch (port_number)
    {
        case 31: // publish weather json pending
            retVal = (uint8_t)publish_json_pending;
            break;
        case 32: // publish weather pending
            retVal = (uint8_t)publish_weather_pending;
            break;
    }
    return retVal;
}