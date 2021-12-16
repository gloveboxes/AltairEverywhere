/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "curldefs.h"
#include "dx_utilities.h"
#include "parson.h"
#include "utils.h"
#include "weather.h"
#include <applibs/log.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct url_data Weather_Data;

char weatherInfo[80];

static size_t weather_write_data(void* ptr, size_t size, size_t nmemb, struct url_data* data)
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

// api.openweathermap.org/data/2.5/weather?lat={lat}&lon={lon}
static const char* weatherURLTemplate = "http://api.openweathermap.org/data/2.5/weather?lat=%3.2f&lon=%3.2f&appid=b9f4fa1afb274c817ccd909027bd049d";

static char weatherUrl[128];

char* GetCurrentWeather(struct location_info* locationInfo, float *Temperature)
{
	JSON_Value* rootProperties = NULL;
	*Temperature = 0.0;
    double temp = 0.0;

	memset(weatherInfo, 0x00, 80);
	// if we don't have network, then return empty string.
	if (!dx_isNetworkReady())
	{
		return &weatherInfo[0];
	}

	Weather_Data.size = 0;
	Weather_Data.data = malloc(2048); /* reasonable size initial buffer */	

	if (Weather_Data.data == NULL) {
		goto cleanup;
	}

	memset(Weather_Data.data, 0, 2048);

	snprintf(weatherUrl, sizeof(weatherUrl), weatherURLTemplate, locationInfo->lat, locationInfo->lng);
	Log_Debug("%s\n", weatherUrl);

	// used for all countries/regions except US.
	bool TempInC = true;

	if (strncmp(locationInfo->countryCode, "US", 2) == 0)
	{
		TempInC = false;
	}
	Log_Debug("Curl starting\n");


	CURLcode res = CURLE_OK;

	CURL* curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, weatherUrl);
	/* use a GET to fetch this */
	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, weather_write_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &Weather_Data);
	/* Perform the request */
	res = curl_easy_perform(curl);

	Log_Debug("Curl completed\n");

	if (res == CURLE_OK)
	{
		memset(weatherInfo, 0x00, sizeof(weatherInfo));

		Log_Debug("%s\n", Weather_Data.data);		

		rootProperties = json_parse_string(Weather_Data.data);
		if (rootProperties == NULL) {
			goto cleanup;
		}

		JSON_Object* weather = NULL;
		JSON_Object* rootObject = json_value_get_object(rootProperties);

		JSON_Array* weatherArray = json_object_dotget_array(rootObject, "weather");
		if (weatherArray == NULL) {
			goto cleanup;
		}

		size_t numWeatherItems = json_array_get_count(weatherArray);	// needs to be more than 0.
		if (numWeatherItems > 0)
		{
			// get the first item in the array
			weather = json_array_get_object(weatherArray, 0);
		}

		JSON_Object* mainProperties = json_object_dotget_object(rootObject, "main");
		if (mainProperties == NULL) {
			goto cleanup;
		}

		int iTemp = 0;

		temp = json_object_get_number(mainProperties, "temp");	// need to do some calc on the returned temperature value to get F or C.
		if (TempInC)
		{
			temp = temp - 273.15;
			iTemp = FLOAT_TO_INT(temp);
			Log_Debug("Temperature: %dC\n", iTemp);
		}
		else
		{
			temp = (1.8 * (temp - 273.15)) + 32;
			iTemp = FLOAT_TO_INT(temp);
			Log_Debug("Temperature: %dF\n", iTemp);
		}

		// if we have a description...
		if (weather != NULL)
		{
			const char* description = json_object_get_string(weather, "description");
			Log_Debug("%s\n", description);
			snprintf(weatherInfo, 80, "%s %d%c", description, iTemp, TempInC ? 'C' : 'F');
		}
		else
		{
			snprintf(weatherInfo, 80, "%d%c", iTemp, TempInC ? 'C' : 'F');
		}
	}

cleanup:

	if (rootProperties != NULL) {
		json_value_free(rootProperties);
	}

	if (Weather_Data.data != NULL) {
		free(Weather_Data.data);
	}

	curl_easy_cleanup(curl);

	*Temperature = (float)temp;
	return &weatherInfo[0];
}
