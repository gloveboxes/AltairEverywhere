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
		case 41:
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
		case 45:
			if (environment.latest.weather.updated)
			{
				request_len = snprintf(
					request_buffer, sizeof(request_buffer), "%d", environment.latest.weather.temperature);
				request_count = 0;
			}
			break;
		case 46:
			if (environment.latest.weather.updated)
			{
				request_len = snprintf(
					request_buffer, sizeof(request_buffer), "%d", environment.latest.weather.pressure);
				request_count = 0;
			}
			break;
		case 47:
			if (environment.latest.weather.updated)
			{
				request_len = snprintf(
					request_buffer, sizeof(request_buffer), "%d", environment.latest.weather.humidity);
				request_count = 0;
			}
			break;
		case 48:
			if (environment.latest.weather.updated)
			{
				request_len = snprintf(
					request_buffer, sizeof(request_buffer), "%s", environment.latest.weather.description);
				request_count = 0;
			}
			break;
		case 49:
			if (environment.locationInfo.updated)
			{
				request_len =
					snprintf(request_buffer, sizeof(request_buffer), "%.6f", environment.locationInfo.lat);
				request_count = 0;
			}
			break;
		case 50:
			if (environment.locationInfo.updated)
			{
				request_len =
					snprintf(request_buffer, sizeof(request_buffer), "%.6f", environment.locationInfo.lng);
				request_count = 0;
			}
			break;
		case 53:
			if (environment.latest.weather.updated)
			{
				request_len = snprintf(
					request_buffer, sizeof(request_buffer), "%.2f", environment.latest.weather.wind_speed);
				request_count = 0;
			}
			break;
		case 54:
			if (environment.latest.weather.updated)
			{
				request_len = snprintf(
					request_buffer, sizeof(request_buffer), "%d", environment.latest.weather.wind_direction);
				request_count = 0;
			}
			break;
		case 55:
			if (environment.locationInfo.updated)
			{
				request_len =
					snprintf(request_buffer, sizeof(request_buffer), "%s", environment.locationInfo.country);
				request_count = 0;
			}
			break;
		case 57:
			if (environment.locationInfo.updated)
			{
				request_len =
					snprintf(request_buffer, sizeof(request_buffer), "%s", environment.locationInfo.city);
				request_count = 0;
			}
			break;
		case 60:
			if (environment.latest.pollution.updated)
			{
				request_len   = snprintf(request_buffer, sizeof(request_buffer), "%.0f",
					  environment.latest.pollution.air_quality_index);
				request_count = 0;
			}
			break;
		case 61:
			if (environment.latest.pollution.updated)
			{
				request_len   = snprintf(request_buffer, sizeof(request_buffer), "%.2f",
					  environment.latest.pollution.carbon_monoxide);
				request_count = 0;
			}
			break;
		case 62:
			if (environment.latest.pollution.updated)
			{
				request_len   = snprintf(request_buffer, sizeof(request_buffer), "%.2f",
					  environment.latest.pollution.nitrogen_monoxide);
				request_count = 0;
			}
			break;
		case 63:
			if (environment.latest.pollution.updated)
			{
				request_len   = snprintf(request_buffer, sizeof(request_buffer), "%.2f",
					  environment.latest.pollution.nitrogen_dioxide);
				request_count = 0;
			}
			break;
		case 64:
			if (environment.latest.pollution.updated)
			{
				request_len = snprintf(
					request_buffer, sizeof(request_buffer), "%.2f", environment.latest.pollution.ozone);
				request_count = 0;
			}
			break;
		case 65:
			if (environment.latest.pollution.updated)
			{
				request_len   = snprintf(request_buffer, sizeof(request_buffer), "%.2f",
					  environment.latest.pollution.sulphur_dioxide);
				request_count = 0;
			}
			break;
		case 66:
			if (environment.latest.pollution.updated)
			{
				request_len = snprintf(
					request_buffer, sizeof(request_buffer), "%.2f", environment.latest.pollution.ammonia);
				request_count = 0;
			}
			break;
		case 67:
			if (environment.latest.pollution.updated)
			{
				request_len = snprintf(
					request_buffer, sizeof(request_buffer), "%.2f", environment.latest.pollution.pm2_5);
				request_count = 0;
			}
			break;
		case 68:
			if (environment.latest.pollution.updated)
			{
				request_len = snprintf(
					request_buffer, sizeof(request_buffer), "%.2f", environment.latest.pollution.pm10);
				request_count = 0;
			}
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