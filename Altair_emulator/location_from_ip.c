/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "curldefs.h"
#include "dx_utilities.h"
#include "location_from_ip.h"
#include "parson.h"
#include "utils.h"
#include <applibs/log.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


struct location_info locationInfo;
struct url_data Location_Data;

static size_t location_write_data(void* ptr, size_t size, size_t nmemb, struct url_data* data)
{
	size_t index = data->size;
	size_t n = (size * nmemb);
	char* tmp;

	data->size += (size * nmemb);

	tmp = realloc(data->data, data->size + 1); /* +1 for '\0' */

	if (tmp) {
		data->data = tmp;
	}
	else {
		if (data->data) {
			free(data->data);
		}
		return 0;
	}

	memcpy((data->data + index), ptr, n);
	data->data[data->size] = '\0';

	return size * nmemb;
}

static const char* geoIfyURL = "https://get.geojs.io/v1/ip/geo.json";



struct location_info* GetLocationData(void)
{
	JSON_Value* rootProperties = NULL;

	memset(&locationInfo, 0x00, sizeof(locationInfo));
	// if we don't have network, then return NULL.
	if (!dx_isNetworkReady())
	{
		return NULL;
	}

	Location_Data.size = 0;
	Location_Data.data = malloc(2048); /* reasonable size initial buffer */
	if (Location_Data.data == NULL) {
		goto cleanup;
	}

	memset(Location_Data.data, 0x00, 2048);

	CURLcode res = CURLE_OK;

	CURL* curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, geoIfyURL);
	/* use a GET to fetch this */
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, location_write_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &Location_Data);
	/* Perform the request */
	res = curl_easy_perform(curl);


	if (res == CURLE_OK)
	{

		Log_Debug("%s\n", Location_Data.data);
		
		rootProperties = json_parse_string(Location_Data.data);
		if (rootProperties == NULL) {
			goto cleanup;
		}

		JSON_Object* rootObject = json_value_get_object(rootProperties);
		if (rootObject == NULL) {
			goto cleanup;
		}

		const char* countryCode = json_object_get_string(rootObject, "country_code");
		const char* latitude = json_object_get_string(rootObject, "latitude");
		const char* longitude = json_object_get_string(rootObject, "longitude");

		double lat = atof(latitude);
		double lng = atof(longitude);
		Log_Debug("Country Code %s\n", countryCode);
		Log_Debug("Lat %f\n", lat);
		Log_Debug("Lng %f\n", lng);

		snprintf(locationInfo.countryCode, 10, countryCode);
		locationInfo.lat = lat;
		locationInfo.lng = lng;
	}

cleanup:

	if (rootProperties != NULL) {
		json_value_free(rootProperties);
	}

	if (Location_Data.data != NULL) {
		free(Location_Data.data);
	}

	curl_easy_cleanup(curl);

	return &locationInfo;
}
