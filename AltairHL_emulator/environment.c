/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "environment.h"

static bool initialized = false;

void init_environment(ALTAIR_CONFIG_T *altair_config)
{
	if (!initialized)
	{
		memset(&environment, 0x00, sizeof(environment));

		get_geolocation(&environment.locationInfo);
		// init_air_visual_service(altair_config, &environment);
		init_open_weather_map_api_key(altair_config, &environment);

		initialized = true;
	}
}

void update_weather()
{
	// update_air_visual(&environment);
	update_owm(&environment);

	environment.valid = environment.locationInfo.updated && environment.latest.weather.updated &&
						environment.latest.pollution.updated;
}
