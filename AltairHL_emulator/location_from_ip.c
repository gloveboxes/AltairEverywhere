/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "dx_utilities.h"
#include "location_from_ip.h"
#include "parson.h"
#include "utils.h"
#include <applibs/log.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static location_info locationInfo;
static const char *geoIfyURL = "https://get.geojs.io/v1/ip/geo.json";

static void generate_fake_location(LOCATION_T *locationInfo){
        // greenwich = {.lat = 51.477928, .lng = -0.001545};  // Blackheath
    if (!locationInfo->updated) {
        locationInfo->lat = 51.477928;
        locationInfo->lng = -0.001545;
        DX_SAFE_STRING_COPY(locationInfo->city, "Blackheath", sizeof(locationInfo->city));
        DX_SAFE_STRING_COPY(locationInfo->country, "England", sizeof(locationInfo->country));
        locationInfo->updated = true;
    }
}

void get_geolocation(LOCATION_T *locationInfo)
{
    JSON_Value *rootProperties = NULL;

    if (locationInfo->updated) {
        return;
    }

    char *data = dx_getHttpData(geoIfyURL);

    if (data == NULL) {
        generate_fake_location(locationInfo);
    } else {

        rootProperties = json_parse_string(data);
        if (rootProperties == NULL) {
            goto cleanup;
        }

        JSON_Object *rootObject = json_value_get_object(rootProperties);
        if (rootObject == NULL) {
            goto cleanup;
        }

        if (json_object_has_value_of_type(rootObject, "latitude", JSONString)) {
            const char *latitude = json_object_get_string(rootObject, "latitude");
            locationInfo->lat = strtod(latitude, NULL);
        }

        if (json_object_has_value_of_type(rootObject, "longitude", JSONString)) {
            const char *longitude = json_object_get_string(rootObject, "longitude");
            locationInfo->lng = strtod(longitude, NULL);
        }

        if (json_object_has_value_of_type(rootObject, "city", JSONString)) {
            DX_SAFE_STRING_COPY(locationInfo->city, json_object_get_string(rootObject, "city"), sizeof(locationInfo->city));
        }

        if (json_object_has_value_of_type(rootObject, "country", JSONString)) {
            DX_SAFE_STRING_COPY(locationInfo->country, json_object_get_string(rootObject, "country"), sizeof(locationInfo->country));
        }

        locationInfo->updated = true;
    }

cleanup:

    if (rootProperties != NULL) {
        json_value_free(rootProperties);
    }

    if (data != NULL) {
        free(data);
        data = NULL;
    }

    return;
}