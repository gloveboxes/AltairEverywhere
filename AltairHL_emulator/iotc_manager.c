/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "iotc_manager.h"

static DX_MESSAGE_PROPERTY *weather_msg_properties[] = {
	&(DX_MESSAGE_PROPERTY){.key = "appid", .value = "altair"},
	&(DX_MESSAGE_PROPERTY){.key = "type", .value = "weather"},
	&(DX_MESSAGE_PROPERTY){.key = "schema", .value = "1"}};

static DX_MESSAGE_CONTENT_PROPERTIES weather_content_properties = {
	.contentEncoding = "utf-8", .contentType = "application/json"};

/// <summary>
/// Device Twin Handler to set the brightness of panel LEDs
/// </summary>
DX_DEVICE_TWIN_HANDLER(set_led_brightness_handler, deviceTwinBinding)
{
	if (!IN_RANGE(*(int *)deviceTwinBinding->propertyValue, 0, 15))
	{
		dx_deviceTwinAckDesiredValue(
			deviceTwinBinding, deviceTwinBinding->propertyValue, DX_DEVICE_TWIN_RESPONSE_ERROR);
	}
	else
	{

#ifdef ALTAIR_FRONT_PANEL_PI_SENSE
		set_led_panel_color(*(int *)deviceTwinBinding->propertyValue);
#endif // ALTAIR_FRONT_PANEL_PI_SENSE

		dx_deviceTwinAckDesiredValue(
			deviceTwinBinding, deviceTwinBinding->propertyValue, DX_DEVICE_TWIN_RESPONSE_COMPLETED);
	}
}
DX_DEVICE_TWIN_HANDLER_END

/// <summary>
/// Determine if environment value changed. If so, update it's device twin
/// </summary>
/// <param name="new_value"></param>
/// <param name="previous_value"></param>
/// <param name="device_twin"></param>
static void device_twin_update_int(
	int *latest_value, int *previous_value, DX_DEVICE_TWIN_BINDING *device_twin)
{
	if (*latest_value != *previous_value)
	{
		*previous_value = *latest_value;
		dx_deviceTwinReportValue(device_twin, latest_value);
	}
}

static void device_twin_update_float(
	float *latest_value, float *previous_value, DX_DEVICE_TWIN_BINDING *device_twin)
{
	if (*latest_value != *previous_value)
	{
		*previous_value = *latest_value;
		dx_deviceTwinReportValue(device_twin, latest_value);
	}
}

static void device_twin_update_string(
	char *latest_value, char *previous_value, size_t length, DX_DEVICE_TWIN_BINDING *device_twin)
{
	if (strncmp(latest_value, previous_value, length) != 0)
	{
		DX_SAFE_STRING_COPY(previous_value, latest_value, length);
		dx_deviceTwinReportValue(device_twin, latest_value);
	}
}

static void device_twin_update_location(
	double latitude, double longitude, DX_DEVICE_TWIN_BINDING *device_twin)
{
	char location_buffer[128];
	snprintf(
		location_buffer, sizeof(location_buffer), "{\"lat\":%f,\"lon\":%f,\"alt\":0}", latitude, longitude);
	dx_deviceTwinReportValue(device_twin, location_buffer);
}

static void update_geo_location(ENVIRONMENT_TELEMETRY *environment)
{
	static bool updated = false;

	if (!updated)
	{
		updated = true;
		device_twin_update_location(
			environment->locationInfo.lat, environment->locationInfo.lng, &dt_location);
		dx_deviceTwinReportValue(&dt_country, environment->locationInfo.country);
		dx_deviceTwinReportValue(&dt_city, environment->locationInfo.city);
	}
}

void publish_properties(ENVIRONMENT_TELEMETRY *environment)
{
#define UPDATE_PROPERTY_FLOAT(name) \
	(device_twin_update_float(      \
		&environment->latest.pollution.name, &environment->previous.pollution.name, &dt_##name))
#define UPDATE_PROPERTY_INT(name) \
	(device_twin_update_int(      \
		&environment->latest.weather.name, &environment->previous.weather.name, &dt_##name))

	if (!azure_connected)
	{
		return;
	}

	UPDATE_PROPERTY_INT(temperature);
	UPDATE_PROPERTY_INT(pressure);
	UPDATE_PROPERTY_INT(humidity);

	device_twin_update_float(
		&environment->latest.weather.wind_speed, &environment->previous.weather.wind_speed, &dt_wind_speed);
	UPDATE_PROPERTY_INT(wind_direction);

	UPDATE_PROPERTY_FLOAT(air_quality_index);
	UPDATE_PROPERTY_FLOAT(carbon_monoxide);
	UPDATE_PROPERTY_FLOAT(nitrogen_monoxide);
	UPDATE_PROPERTY_FLOAT(nitrogen_dioxide);
	UPDATE_PROPERTY_FLOAT(ozone);
	UPDATE_PROPERTY_FLOAT(sulphur_dioxide);
	UPDATE_PROPERTY_FLOAT(ammonia);
	UPDATE_PROPERTY_FLOAT(pm2_5);
	UPDATE_PROPERTY_FLOAT(pm10);

	update_geo_location(environment);
}

void publish_telemetry(ENVIRONMENT_TELEMETRY *environment)
{
	if (!azure_connected || !environment->valid)
	{
		return;
	}

	const char *publish_template = "{"
								   "\"temperature\":%d,"
								   "\"pressure\":%d,"
								   "\"humidity\":%d,"
								   "\"windspeed\":%.2f,"
								   "\"winddirection\":%d,"
								   "\"aqi\":%.2f,"
								   "\"co\":%.2f,"
								   "\"no\":%.2f,"
								   "\"no2\":%.2f,"
								   "\"o3\":%.2f,"
								   "\"so2\":%.2f,"
								   "\"nh3\":%.2f,"
								   "\"pm2_5\":%.2f,"
								   "\"pm10\":%.2f,"
								   "\"latitude\":%.6f,"
								   "\"longitude\":%.6f"
								   "}";

	size_t msg_len =
		snprintf(msgBuffer, sizeof(msgBuffer), publish_template, environment->latest.weather.temperature,
			environment->latest.weather.pressure, environment->latest.weather.humidity,
			environment->latest.weather.wind_speed, environment->latest.weather.wind_direction,
			environment->latest.pollution.air_quality_index, environment->latest.pollution.carbon_monoxide,
			environment->latest.pollution.nitrogen_monoxide, environment->latest.pollution.nitrogen_dioxide,
			environment->latest.pollution.ozone, environment->latest.pollution.sulphur_dioxide,
			environment->latest.pollution.ammonia, environment->latest.pollution.pm2_5,
			environment->latest.pollution.pm10, environment->locationInfo.lat, environment->locationInfo.lng);

	if (msg_len < sizeof(msgBuffer))
	{
		dx_azurePublish(msgBuffer, msg_len, weather_msg_properties, NELEMS(weather_msg_properties),
			&weather_content_properties);
	}
	else
	{
		Log_Debug("MsgBuffer too small. Msg not sent.\n");
	}

	publish_properties(environment);
}
