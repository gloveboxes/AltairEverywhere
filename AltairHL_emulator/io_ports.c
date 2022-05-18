/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "io_ports.h"

static int copy_web(char *url);

typedef struct
{
	size_t len;
	size_t count;
	char buffer[64];
} REQUEST_UNIT_T;

typedef struct
{
	char buffer[256];
	int index;
} JSON_UNIT_T;

typedef struct
{
	int fd;
	char filename[15];
	char url[128];
	bool file_opened;
	bool enabled;
	bool end_of_file;
	int index;
	int ch;
} COPY_X_T;

static COPY_X_T copy_x;

static JSON_UNIT_T ju;
static REQUEST_UNIT_T ru;
static volatile bool delay_milliseconds_enabled = false;
static volatile bool delay_seconds_enabled      = false;
static volatile bool publish_json_pending       = false;
static volatile bool publish_weather_pending    = false;

// set tick_count to 1 as the tick count timer doesn't kick in until 1 second after startup
static uint32_t tick_count = 1;

#ifdef ALTAIR_FRONT_PANEL_PI_SENSE
// Font buffers
static uint8_t bitmap[8];
uint16_t panel_8x8_buffer[64];
#endif // ALTAIR_FRONT_PANEL_PI_SENSE

// clang-format off
DX_MESSAGE_PROPERTY *json_msg_properties[] = {
    &(DX_MESSAGE_PROPERTY){.key = "appid", .value = "altair"},
    &(DX_MESSAGE_PROPERTY){.key = "type", .value = "custom"},
    &(DX_MESSAGE_PROPERTY){.key = "schema", .value = "1"}};

DX_MESSAGE_CONTENT_PROPERTIES json_content_properties = {
    .contentEncoding = "utf-8", .contentType = "application/json"};
// clang-format on

static void format_double4(const void *value);
static void format_float0(const void *value);
static void format_float2(const void *value);
static void format_int(const void *value);
static void format_string(const void *value);

// Weather definitions
static const void *w_key[] = {
	"Celsius", "Millibar", "Humidity %", "Wind km/h", "Wind degrees", "Observation"};
static const void *w_value[] = {&environment.latest.weather.temperature, &environment.latest.weather.pressure,
	&environment.latest.weather.humidity, &environment.latest.weather.wind_speed,
	&environment.latest.weather.wind_direction, &environment.latest.weather.description};
static void (*w_formatter[])(const void *value) = {
	format_int, format_int, format_int, format_float2, format_int, format_string};

// Location definitions
static const void *l_key[]   = {"Latitude", "Longitude", "Country", "City"};
static const void *l_value[] = {&environment.locationInfo.lat, &environment.locationInfo.lng,
	&environment.locationInfo.country, &environment.locationInfo.city};
static void (*l_formatter[])(const void *value) = {
	format_double4, format_double4, format_string, format_string};

// Pollution defintions
static const void *p_key[]   = {"AQI(CAQI)", "CO", "NO", "NO2", "O3", "SO2", "NH3", "PM2.5", "PM1.0"};
static const void *p_value[] = {&environment.latest.pollution.air_quality_index,
	&environment.latest.pollution.carbon_monoxide, &environment.latest.pollution.nitrogen_monoxide,
	&environment.latest.pollution.nitrogen_dioxide, &environment.latest.pollution.ozone,
	&environment.latest.pollution.sulphur_dioxide, &environment.latest.pollution.ammonia,
	&environment.latest.pollution.pm2_5, &environment.latest.pollution.pm10};
static void (*p_formatter[])(const void *value) = {format_float0, format_float2, format_float2, format_float2,
	format_float2, format_float2, format_float2, format_float2, format_float2};

static void format_float0(const void *value)
{
	ru.len = (size_t)snprintf(ru.buffer, sizeof(ru.buffer), "%.0f", *(float *)value);
}

static void format_float2(const void *value)
{
	ru.len = (size_t)snprintf(ru.buffer, sizeof(ru.buffer), "%.2f", *(float *)value);
}

static void format_double4(const void *value)
{
	ru.len = (size_t)snprintf(ru.buffer, sizeof(ru.buffer), "%.4f", *(double *)value);
}

static void format_int(const void *value)
{
	ru.len = (size_t)snprintf(ru.buffer, sizeof(ru.buffer), "%d", *(int *)value);
}

static void format_string(const void *value)
{
	ru.len = (size_t)snprintf(ru.buffer, sizeof(ru.buffer), "%s", (char *)value);
}

DX_TIMER_HANDLER(timer_seconds_expired_handler)
{
	delay_seconds_enabled = false;
}
DX_TIMER_HANDLER_END

DX_TIMER_HANDLER(timer_millisecond_expired_handler)
{
	delay_milliseconds_enabled = false;
}
DX_TIMER_HANDLER_END

DX_TIMER_HANDLER(tick_count_handler)
{
	tick_count++;
}
DX_TIMER_HANDLER_END

DX_ASYNC_HANDLER(async_copyx_request_handler, handle)
{
	copy_web(copy_x.url);
	copy_x.end_of_file = false;
}
DX_ASYNC_HANDLER_END

DX_ASYNC_HANDLER(async_set_timer_seconds_handler, handle)
{
	int data = *((int *)handle->data);
	dx_timerOneShotSet(&tmr_timer_seconds_expired, &(struct timespec){data, 0});
}
DX_ASYNC_HANDLER_END

DX_ASYNC_HANDLER(async_set_timer_millisecond_handler, handle)
{
	int data = *((int *)handle->data);
	dx_timerOneShotSet(&tmr_timer_millisecond_expired, &(struct timespec){0, data * ONE_MS});
}
DX_ASYNC_HANDLER_END

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
		dx_azurePublish(ju.buffer, strlen(ju.buffer), json_msg_properties, NELEMS(json_msg_properties),
			&json_content_properties);
#endif
	}
	publish_json_pending = false;
}
DX_ASYNC_HANDLER_END

/// <summary>
/// Intel 8080 OUT Port handler
/// </summary>
/// <param name="port"></param>
/// <param name="data"></param>
void io_port_out(uint8_t port, uint8_t data)
{
	memset(&ru, 0x00, sizeof(REQUEST_UNIT_T));
	static int timer_delay;
	static int timer_milliseconds_delay;

	switch (port)
	{
		case 29:
			delay_milliseconds_enabled = false;
			if (data > 0)
			{
				delay_milliseconds_enabled = true;
				timer_milliseconds_delay   = data;
				dx_asyncSend(&async_set_millisecond_timer, (void *)&timer_milliseconds_delay);
			}
			break;
		case 30:
			delay_seconds_enabled = false;
			if (data > 0)
			{
				delay_seconds_enabled = true;
				timer_delay           = data;
				dx_asyncSend(&async_set_seconds_timer, (void *)&timer_delay);
			}
			break;
		case 31:
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
		case 32:
			if (!publish_weather_pending)
			{
				publish_weather_pending = true;
				dx_asyncSend(&async_publish_weather, NULL);
			}
			break;
		case 33: // copy file from web server to mutable storage
			if (copy_x.index == 0)
			{
				memset(copy_x.filename, 0x00, sizeof(copy_x.filename));
				if (copy_x.file_opened && copy_x.fd != -1)
				{
					close(copy_x.fd);
					copy_x.file_opened = false;
					copy_x.end_of_file = true;
					copy_x.fd          = -1;
				}
			}

			if (data != 0 && copy_x.index < sizeof(copy_x.filename))
			{
				copy_x.filename[copy_x.index] = data;
				copy_x.index++;
			}

			if (data == 0) // NULL TERMINATION
			{
				copy_x.index       = 0;
				copy_x.end_of_file = true;

				memset(copy_x.url, 0x00, sizeof(copy_x.url));
				snprintf(copy_x.url, sizeof(copy_x.url), "%s/%s", altair_config.copy_x_url, copy_x.filename);

				dx_asyncSend(&async_copyx_request, NULL);
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
			ru.len = (size_t)snprintf(ru.buffer, sizeof(ru.buffer), "%u", tick_count);
			break;
		case 42: // get utc date and time
			dx_getCurrentUtc(ru.buffer, sizeof(ru.buffer));
			ru.len = strnlen(ru.buffer, sizeof(ru.buffer));
			break;
		case 43: // get local date and time
#ifdef AZURE_SPHERE
			dx_getCurrentUtc(ru.buffer, sizeof(ru.buffer));
#else
			dx_getLocalTime(ru.buffer, sizeof(ru.buffer));
#endif
			ru.len = strnlen(ru.buffer, sizeof(ru.buffer));
			break;
		case 44: // Generate random number to seed mbasic randomize command
			ru.len = (size_t)snprintf(ru.buffer, sizeof(ru.buffer), "%d", ((rand() % 64000) - 32000));
			break;
#ifdef AZURE_SPHERE
		case 60: // Red LEB
			dx_gpioStateSet(&gpioRed, (bool)data);
			break;
		case 61: // Green LEB
			dx_gpioStateSet(&gpioGreen, (bool)data);
			break;
		case 62: // Blue LEB
			dx_gpioStateSet(&gpioBlue, (bool)data);
			break;
		case 63: // Onboard temperature
			ru.len = (size_t)snprintf(ru.buffer, sizeof(ru.buffer), "%d",
				data ? onboard_telemetry.pressure : onboard_telemetry.temperature);
			break;
#endif // AZURE_SPHERE
#ifdef ALTAIR_FRONT_PANEL_PI_SENSE
		case 80: // panel_mode 0 = bus data, 1 = font, 2 = bitmap
			if (data < 3)
			{
				panel_mode = data;
			}
			break;
		case 81:
			gfx_set_color(data);
			break;
		case 82: // set pixel red color
			break;
		case 83: // set pixel green color
			break;
		case 84: // set pixel blue color
			break;
		case 85: // display character
			memset(panel_8x8_buffer, 0x00, sizeof(panel_8x8_buffer));
			gfx_load_character(data, bitmap);
			gfx_rotate_counterclockwise(bitmap, 1, 1, bitmap);
			gfx_reverse_panel(bitmap);
			gfx_rotate_counterclockwise(bitmap, 1, 1, bitmap);
			gfx_bitmap_to_rgb(bitmap, panel_8x8_buffer, sizeof(panel_8x8_buffer));
			pi_sense_8x8_panel_update(panel_8x8_buffer, sizeof(panel_8x8_buffer));
			break;
#endif // PI SENSE HAT
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

	switch (port)
	{
		case 29: // Has delay expired
			retVal = (uint8_t)delay_milliseconds_enabled;
			break;
		case 30: // Has delay expired
			retVal = (uint8_t)delay_seconds_enabled;
			break;
		case 31:
			retVal = (uint8_t)publish_json_pending;
			break;
		case 32:
			retVal = (uint8_t)publish_weather_pending;
			break;
		case 33: // has copyx file need copied and loaded
			retVal = copy_x.end_of_file;
			break;
		case 200: // READ STRING
			if (ru.count < ru.len && ru.count < sizeof(ru.buffer))
			{
				retVal = ru.buffer[ru.count++];
			}
			else
			{
				retVal = 0x00;
			}
			break;
		case 201: // READ COPYX file from mutable storage
			if (copy_x.end_of_file)
			{
				retVal = 0x00;
			}
			else
			{
				if (!copy_x.file_opened)
				{
					/* open the file */
#ifdef AZURE_SPHERE
					copy_x.fd = Storage_OpenMutableFile();
#else
					copy_x.fd = open("MutableStorage/copyx", O_RDONLY);
#endif
					if (copy_x.fd != -1)
					{
						lseek(copy_x.fd, 0, SEEK_SET);
						copy_x.file_opened = true;
					}
				}

				if (copy_x.file_opened)
				{
					if (read(copy_x.fd, &retVal, 1) == 0)
					{
						close(copy_x.fd);
#ifdef AZURE_SPHERE
						Storage_DeleteMutableFile();
#endif
						copy_x.file_opened = false;
						copy_x.end_of_file = true;
						copy_x.fd          = -1;
						retVal             = 0x00;
					}
				}
				else
				{
					retVal = 0x00;
				}
			}
			break;
		default:
			retVal = 0x00;
	}

	return retVal;
}

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
	ssize_t written = write(*(int *)stream, ptr, nmemb);
	return (size_t)written;
}

static int copy_web(char *url)
{
	CURL *curl_handle;
	int copy_web_fd = -1;

#ifndef AZURE_SPHERE
	const char *pagefilename = "MutableStorage/copyx";
#endif

	/* init the curl session */
	curl_handle = curl_easy_init();

	/* set URL to get here */
	curl_easy_setopt(curl_handle, CURLOPT_URL, url);

	curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);

	// https://curl.se/libcurl/c/CURLOPT_NOSIGNAL.html
	curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1L);

	// https://curl.se/libcurl/c/CURLOPT_TIMEOUT.html
	curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 4L);

	/* Switch on full protocol/debug output while testing */
	// curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);

	/* disable progress meter, set to 0L to enable it */
	curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);

	/* send all data to this function  */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);

/* open the file */
#ifdef AZURE_SPHERE
	Storage_DeleteMutableFile();
	copy_web_fd = Storage_OpenMutableFile();
#else
	copy_web_fd = open(pagefilename, O_RDWR | O_TRUNC);
#endif

	if (copy_web_fd != -1)
	{

		/* write the page body to this file handle */
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &copy_web_fd);

		/* get it! */
		curl_easy_perform(curl_handle);

		/* close the header file */
		close(copy_web_fd);
	}

	/* cleanup curl stuff */
	curl_easy_cleanup(curl_handle);

	curl_global_cleanup();

	return 0;
}
