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
static const char *weatherURLTemplate = "http://api.openweathermap.org/data/2.5/weather?lat=%3.2f&lon=%3.2f&appid=b9f4fa1afb274c817ccd909027bd049d";
static int64_t previous_request_milliseconds;

static char weatherUrl[128];

void GetCurrentWeather(struct location_info *locationInfo, WEATHER_TELEMETRY *telemetry)
{
	int64_t now = dx_getNowMilliseconds();

    // throttle weather requests to no more than once every 10 minutes = 60000 milliseconds * 10
	if (now < previous_request_milliseconds + (60000 * 10)){
		return;
	}

    JSON_Value *rootProperties = NULL;
    double temp = 0.0;

    telemetry->valid = false;

    snprintf(weatherUrl, sizeof(weatherUrl), weatherURLTemplate, locationInfo->lat, locationInfo->lng);

    char *data = getHttpData(weatherUrl);

    if (data != NULL) {

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

        if (!json_object_has_value_of_type(mainProperties, "temp", JSONNumber) || 
			!json_object_has_value_of_type(mainProperties, "pressure", JSONNumber) ||
            !json_object_has_value_of_type(mainProperties, "humidity", JSONNumber)) {
            goto cleanup;
        }

        temp = json_object_get_number(mainProperties, "temp"); // need to do some calc on the returned temperature value to get F or C.

        temp = temp - 273.15;

        telemetry->latest.temperature = FLOAT_TO_INT(temp);
        telemetry->latest.pressure = FLOAT_TO_INT(json_object_get_number(mainProperties, "pressure"));
        telemetry->latest.humidity = FLOAT_TO_INT(json_object_get_number(mainProperties, "humidity"));
		telemetry->latest.latitude = locationInfo->lat;
		telemetry->latest.longitude = locationInfo->lng;
		strncpy(telemetry->latest.country_code, locationInfo->countryCode, 10);

        telemetry->valid = true;

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
