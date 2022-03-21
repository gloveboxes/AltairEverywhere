/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "io_ports.h"

static FILE *copyx_stream;
static bool copyx_enabled = false;
static char copyx_filename[15];
static int copyx_index                       = 0;
static volatile bool delay_enabled           = false;
static volatile bool publish_weather_pending = false;
static volatile bool publish_json_pending    = false;
static int jitter                            = 0;
static int request_len;
static int request_count;
static char request_buffer[64];
static char json_buffer[256];

// set tick_count to 1 as the tick count timer doesn't kick in until 1 second after startup
static volatile uint32_t tick_count = 1;

DX_MESSAGE_PROPERTY *json_msg_properties[] = {&(DX_MESSAGE_PROPERTY){.key = "appid", .value = "altair"},
	&(DX_MESSAGE_PROPERTY){.key = "type", .value = "json"},
	&(DX_MESSAGE_PROPERTY){.key = "schema", .value = "1"}};

DX_MESSAGE_CONTENT_PROPERTIES json_content_properties = {
	.contentEncoding = "utf-8", .contentType = "application/json"};

static void format_double4(void *value);
static void format_float0(void *value);
static void format_float2(void *value);
static void format_int(void *value);
static void format_string(const void *value);

// Weather definitions
static const void *w_key[] = {
	"Celsius", "Millibar", "Humidity %", "Wind km/h", "Wind degrees", "Description"};
static const void *w_value[] = {&environment.latest.weather.temperature, &environment.latest.weather.pressure,
	&environment.latest.weather.humidity, &environment.latest.weather.wind_speed,
	&environment.latest.weather.wind_direction, &environment.latest.weather.description};
static void (*w_formatter[])() = {
	format_int, format_int, format_int, format_float2, format_int, format_string};

// Location definitions
static const void *l_key[]     = {"Latitude", "Longitude", "Country", "City"};
static const void *l_value[]   = {&environment.locationInfo.lat, &environment.locationInfo.lng,
    &environment.locationInfo.country, &environment.locationInfo.city};
static void (*l_formatter[])() = {format_double4, format_double4, format_string, format_string};

// Pollution defintions
static const void *p_key[]   = {"AQI(CAQI)", "CO", "NO", "NO2", "O3", "SO2", "NH3", "PM2.5", "PM1.0"};
static const void *p_value[] = {&environment.latest.pollution.air_quality_index,
	&environment.latest.pollution.carbon_monoxide, &environment.latest.pollution.nitrogen_monoxide,
	&environment.latest.pollution.nitrogen_dioxide, &environment.latest.pollution.ozone,
	&environment.latest.pollution.sulphur_dioxide, &environment.latest.pollution.ammonia,
	&environment.latest.pollution.pm2_5, &environment.latest.pollution.pm10};

static void (*p_formatter[])() = {format_float0, format_float2, format_float2, format_float2, format_float2,
	format_float2, format_float2, format_float2, format_float2};

static void format_float0(void *value)
{
	request_len   = snprintf(request_buffer, sizeof(request_buffer), "%.0f", *(float *)value);
	request_count = 0;
}

static void format_float2(void *value)
{
	request_len   = snprintf(request_buffer, sizeof(request_buffer), "%.2f", *(float *)value);
	request_count = 0;
}

static void format_double4(void *value)
{
	request_len   = snprintf(request_buffer, sizeof(request_buffer), "%.4f", *(double *)value);
	request_count = 0;
}

static void format_int(void *value)
{
	request_len   = snprintf(request_buffer, sizeof(request_buffer), "%d", *(int *)value);
	request_count = 0;
}

static void format_string(const void *value)
{
	request_len   = snprintf(request_buffer, sizeof(request_buffer), "%s", (char *)value);
	request_count = 0;
}

DX_TIMER_HANDLER(port_timer_expired_handler)
{
	delay_enabled = false;
}
DX_TIMER_HANDLER_END

DX_TIMER_HANDLER(tick_count_handler)
{
	tick_count++;
}
DX_TIMER_HANDLER_END

DX_TIMER_HANDLER(port_out_weather_handler)
{
	if (environment.valid && azure_connected)
	{
		environment.latest.weather.temperature += jitter;
		publish_telemetry(&environment);
		environment.latest.weather.temperature -= jitter;
	}
	publish_weather_pending = false;
}
DX_TIMER_HANDLER_END

DX_TIMER_HANDLER(port_out_json_handler)
{
	if (azure_connected)
	{
		dx_azurePublish(json_buffer, strlen(json_buffer), json_msg_properties, NELEMS(json_msg_properties),
			&json_content_properties);
	}
	publish_json_pending = false;
}
DX_TIMER_HANDLER_END

/// <summary>
/// Intel 8080 OUT Port handler
/// </summary>
/// <param name="port"></param>
/// <param name="data"></param>
void io_port_out(uint8_t port, uint8_t data)
{
	static int json_buffer_index = 0;
	char filename[64];

	switch (port)
	{
		case 30:
			if (data > 0)
			{
				dx_timerOneShotSet(&tmr_port_timer_expired, &(struct timespec){data, 0});
				delay_enabled = true;
			}
			break;
		case 31:
			if (!publish_json_pending)
			{
				if (json_buffer_index == 0)
				{
					memset((void *)json_buffer, 0x00, sizeof(json_buffer));
				}

				if (data != 0 && json_buffer_index < sizeof(json_buffer))
				{
					json_buffer[json_buffer_index] = data;
					json_buffer_index++;
				}

				if (data == 0)
				{
					publish_json_pending = true;
					json_buffer_index    = 0;
					dx_timerOneShotSet(&tmr_deferred_port_out_json, &(struct timespec){0, 1});
				}
			}
			break;
		case 32:
			if (!publish_weather_pending)
			{
				publish_weather_pending = true;
				jitter                  = (int)data;
				dx_timerOneShotSet(&tmr_deferred_port_out_weather, &(struct timespec){0, 1});
			}
			break;
		case 33:
			if (copyx_index == 0)
			{
				memset(copyx_filename, 0x00, sizeof(copyx_filename));
				copyx_enabled = false;
				if (copyx_stream != NULL)
				{
					fclose(copyx_stream);
				}
			}

			if (data != 0 && copyx_index < sizeof(copyx_filename))
			{
				copyx_filename[copyx_index] = data;
				copyx_index++;
			}

			if (data == 0)
			{
				copyx_index = 0;
				memset(filename, 0x00, sizeof(filename));
				snprintf(filename, sizeof(filename), "%s/%s", COPYX_FOLDER_NAME, copyx_filename);

				if ((copyx_stream = fopen(filename, "r")) != NULL)
				{
					copyx_enabled = true;
				}
			}
			break;
		case 34: // Weather key
			if (data < NELEMS(w_key))
			{
				format_string(w_key[data]);
			}

			break;
		case 35: // weather value
			if (environment.latest.weather.updated && data < NELEMS(w_value))
			{
				w_formatter[data](w_value[data]);
			}
			break;
		case 36: // Location key
			if (data < NELEMS(l_key))
			{
				format_string(l_key[data]);
			}
			break;
		case 37: // Location value
			if (environment.locationInfo.updated && data < NELEMS(l_value))
			{
				l_formatter[data](l_value[data]);
			}
			break;
		case 38: // Pollution key
			if (data < NELEMS(p_key))
			{
				format_string(p_key[data]);
			}
			break;
		case 39: // Pollution value
			if (environment.latest.pollution.updated && data < NELEMS(p_value))
			{
				p_formatter[data](p_value[data]);
			}
			break;
		case 41: // System tick count
			request_len   = snprintf(request_buffer, sizeof(request_buffer), "%u", tick_count);
			request_count = 0;
			break;
		case 42: // get utc date and time
			dx_getCurrentUtc(request_buffer, sizeof(request_buffer));
			request_len   = strnlen(request_buffer, sizeof(request_buffer));
			request_count = 0;
			break;
		case 43: // get local date and time
			dx_getLocalTime(request_buffer, sizeof(request_buffer));
			request_len   = strnlen(request_buffer, sizeof(request_buffer));
			request_count = 0;
			break;
		case 44: // Generate random number to seed mbasic randomize command
			request_len = snprintf(request_buffer, sizeof(request_buffer), "%d", ((rand() % 64000) - 32000));
			request_count = 0;
			break;
		default:
			break;
	}
}

/// <summary>
/// Intel 8080 IN Port handler
/// </summary>
/// <param name="port"></param>
/// <returns></returns>
uint8_t io_port_in(uint8_t port)
{
	uint8_t retVal = 0;
	int ch;

	switch (port)
	{
		case 30: // Has delay expired
			retVal = (uint8_t)delay_enabled;
			break;
		case 31:
			retVal = (uint8_t)publish_json_pending;
			break;
		case 32:
			retVal = (uint8_t)publish_weather_pending;
			break;
		case 33:
			if (copyx_enabled)
			{
				if ((ch = fgetc(copyx_stream)) == EOF)
				{
					fclose(copyx_stream);
					copyx_stream  = NULL;
					copyx_enabled = false;
					retVal        = 0x00;
				}
				else
				{
					retVal = (uint8_t)ch;
				}
			}

			break;
		case 200: // READ STRING
			if (request_count < request_len)
			{
				retVal = request_buffer[request_count++];
			}
			else
			{
				retVal = 0x00;
			}

			break;
		default:
			retVal = 0x00;
	}

	return retVal;
}