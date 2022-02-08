/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <curl/curl.h>
#include <curl/easy.h>
#include "dx_utilities.h"
#include "parson.h"
#include "utils.h"
#include "weather.h"
#include <applibs/log.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// api.openweathermap.org/data/2.5/weather?lat={lat}&lon={lon}
static const char *weatherURLTemplate = "http://api.openweathermap.org/data/2.5/weather?lat=%3.2f&lon=%3.2f&appid=%s";
static int64_t previous_request_milliseconds;
static char *owm_api_key = NULL;
static char weatherUrl[128];
static struct location_info *locationInfo = NULL;

static void generate_fake_telemetry(WEATHER_TELEMETRY *telemetry)
{
    static struct location_info greenwich = {.lat = 51.477928, .lng = -0.001545};
    // So create some random weather data with geo location on Greenwich
    int rnd = (rand() % 10) - 5;

    telemetry->latest.temperature = 25 + rnd;

    rnd = (rand() % 50) - 25;
    telemetry->latest.pressure = 1000 + rnd;
    telemetry->latest.humidity = 20 + (rand() % 60);

    if (locationInfo != NULL && locationInfo->updated) {
        telemetry->locationInfo = locationInfo;
    } else {
        telemetry->locationInfo = &greenwich;
    }

    telemetry->valid = true;
    telemetry->fake = true;
}

void GetCurrentWeather(WEATHER_TELEMETRY *telemetry)
{
    int64_t now = dx_getNowMilliseconds();

    // throttle weather requests to no more than once every 15 minutes = 60000 milliseconds * 15
    if (now < previous_request_milliseconds + (60000 * 15)) {
        return;
    }

    locationInfo = GetLocationData();
    if (!locationInfo->updated) {
        dx_Log_Debug("Geo location request failed\n");
        generate_fake_telemetry(telemetry);
        previous_request_milliseconds = now;
        return;
    }

    if (dx_isStringNullOrEmpty(owm_api_key)) {
        dx_Log_Debug("Missing Open Weather Map API Key\n");
        generate_fake_telemetry(telemetry);
        previous_request_milliseconds = now;
        return;
    }

    JSON_Value *rootProperties = NULL;
    double temp = 0.0;

    telemetry->valid = false;

    snprintf(weatherUrl, sizeof(weatherUrl), weatherURLTemplate, locationInfo->lat, locationInfo->lng, owm_api_key);

    char *data = getHttpData(weatherUrl);

    if (data == NULL) {
        generate_fake_telemetry(telemetry);
    } else {

        rootProperties = json_parse_string(data);
        if (rootProperties == NULL) {
            goto cleanup;
        }

        JSON_Object *weather = NULL;
        JSON_Object *rootObject = json_value_get_object(rootProperties);

        JSON_Array *weatherArray = json_object_dotget_array(rootObject, "weather");
        if (weatherArray == NULL) {
            goto cleanup;
        }

        size_t numWeatherItems = json_array_get_count(weatherArray); // needs to be more than 0.
        if (numWeatherItems > 0) {
            // get the first item in the array
            weather = json_array_get_object(weatherArray, 0);
        }

        JSON_Object *mainProperties = json_object_dotget_object(rootObject, "main");
        if (mainProperties == NULL) {
            goto cleanup;
        }

        if (!json_object_has_value_of_type(mainProperties, "temp", JSONNumber) || !json_object_has_value_of_type(mainProperties, "pressure", JSONNumber) ||
            !json_object_has_value_of_type(mainProperties, "humidity", JSONNumber)) {
            goto cleanup;
        }

        temp = json_object_get_number(mainProperties, "temp"); // need to do some calc on the returned temperature value to get F or C.

        temp = temp - 273.15;

        telemetry->latest.temperature = FLOAT_TO_INT(temp);
        telemetry->latest.pressure = FLOAT_TO_INT(json_object_get_number(mainProperties, "pressure"));
        telemetry->latest.humidity = FLOAT_TO_INT(json_object_get_number(mainProperties, "humidity"));
        telemetry->locationInfo = locationInfo;

        telemetry->valid = true;
        telemetry->fake = false;

        // if we have a description...
        if (weather != NULL && json_object_has_value_of_type(weather, "description", JSONString)) {
            const char *description = json_object_get_string(weather, "description");
            snprintf(telemetry->latest.description, 80, "%s %d%c", description, telemetry->latest.temperature, 'C');
        } else {
            snprintf(telemetry->latest.description, 80, "%d%c", telemetry->latest.temperature, 'F');
        }

        previous_request_milliseconds = now;
    }

cleanup:

    if (rootProperties != NULL) {
        json_value_free(rootProperties);
        rootProperties = NULL;
    }

    if (data != NULL) {
        free(data);
        data = NULL;
    }
}

void init_open_weather_map_api_key(int argc, char *argv[])
{
    static bool initialized = false;
    extern int optind;

    if (!initialized) {

        initialized = true;

        int option = 0;
        static const struct option cmdLineOptions[] = {
            {.name = "OpenWeatherMapKey", .has_arg = required_argument, .flag = NULL, .val = 'o'}
        };

        // reset the options index to 1
        // https://pubs.opengroup.org/onlinepubs/9699919799/functions/getopt.html
        optind = 1;

        // option = getopt_long(argc, argv, "o:", cmdLineOptions, NULL);

        // Loop over all of the options.
        while ((option = getopt_long(argc, argv, "o:", cmdLineOptions, NULL)) != -1) {
            // Check if arguments are missing. Every option requires an argument.
            if (optarg != NULL && optarg[0] == '-') {
                printf("WARNING: Option %c requires an argument\n", option);
                continue;
            }
            switch (option) {
            case 'o':
                owm_api_key = optarg;
                break;
            default:
                // Unknown options are ignored.
                break;
            }
        }
    }
}