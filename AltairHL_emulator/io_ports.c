/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "io_ports.h"
#define GAMES_REPO        "https://raw.githubusercontent.com/AzureSphereCloudEnabledAltair8800/RetroGames/main"
#define PERSONAL_REPO     "http://192.168.10.136:5555"
#define ENDPOINT_LEN      128
#define ENDPOINT_ELEMENTS 2

static int copy_web(char *url);

// Device ID is 128 bytes. But we are only taking the first 8 bytes and allowing a terminating NULL;
#define DEVICE_ID_BUFFER_SIZE 9
static char azsphere_device_id[DEVICE_ID_BUFFER_SIZE];

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

enum WEBGET_STATUS
{
    WEBGET_EOF,
    WEBGET_WAITING,
    WEBGET_DATA_READY,
    WEBGET_FAILED
};

pthread_mutex_t webget_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct
{
    bool end_of_file;
    char *byte_stream;
    char bytes[1024];
    char personal_endpoint[ENDPOINT_LEN];
    uint8_t selected_endpoint;
    char filename[15];
    char url[150];
    enum WEBGET_STATUS status;
    int index;
    size_t byte_stream_length;
} WEBGET_T;

typedef struct
{
    int fd;
    char filename[15];
    bool file_opened;
    bool enabled;
    bool end_of_file;
    int index;
    int ch;
} DEVGET_T;

static WEBGET_T webget;
static DEVGET_T devget;

static JSON_UNIT_T ju;
static REQUEST_UNIT_T ru;
static volatile bool delay_milliseconds_enabled = false;
static volatile bool delay_seconds_enabled      = false;
static volatile bool publish_json_pending       = false;
static volatile bool publish_weather_pending    = false;

// set tick_count to 1 as the tick count timer doesn't kick in until 1 second after startup
static uint32_t tick_count = 1;

#ifdef OEM_AVNET
static float x, y, z;
static bool accelerometer_running = false;
static char PREDICTION[20]        = {'n', 'o', 'r', 'm', 'a', 'l'};
#endif // OEM_AVNET

#ifdef ALTAIR_FRONT_PANEL_PI_SENSE
// Font buffers
static uint8_t bitmap[8];
uint16_t panel_8x8_buffer[64];

#endif // ALTAIR_FRONT_PANEL_PI_SENSE

#if defined(ALTAIR_FRONT_PANEL_RETRO_CLICK) || defined(ALTAIR_FRONT_PANEL_PI_SENSE)

typedef union {
    uint8_t bitmap[8];
    uint64_t bitmap64;
} PIXEL_MAP;

PIXEL_MAP pixel_map;

#endif // ALTAIR_FRONT_PANEL_RETRO_CLICK

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

/// <summary>
/// Callback handler for Asynchronous Inter-Core Messaging Pattern
/// </summary>
void intercore_classify_response_handler(void *data_block, ssize_t message_length)
{
#ifdef OEM_AVNET

    INTERCORE_PREDICTION_BLOCK_T *ic_message_block = (INTERCORE_PREDICTION_BLOCK_T *)data_block;

    switch (ic_message_block->cmd)
    {
        case IC_PREDICTION:
            // Log_Debug("Prediction %s\n", ic_message_block->PREDICTION);
            memcpy(PREDICTION, ic_message_block->PREDICTION, sizeof(PREDICTION));
            break;
        default:
            break;
    }

#endif // OEM_AVNET
}

DX_TIMER_HANDLER(read_accelerometer_handler)
{
#ifdef OEM_AVNET
    float xx, yy, zz;

    avnet_get_acceleration(&xx, &yy, &zz);

    // Log_Debug("now x %f, y %f, z %f\n", xx, yy, zz);

    x += xx;
    y += yy;
    z += zz;
    x /= 2;
    y /= 2;
    z /= 2;

    intercore_ml_classify_block.x = x;
    intercore_ml_classify_block.y = y;
    intercore_ml_classify_block.z = z;

    dx_intercorePublish(
        &intercore_ml_classify_ctx, &intercore_ml_classify_block, sizeof(intercore_ml_classify_block));

    // dx_Log_Debug("avg x %f, y %f, z %f\n", x, y, z);

    dx_timerOneShotSet(&tmr_read_accelerometer, &(struct timespec){0, 10 * ONE_MS});
#endif // OEM_AVNET
}
DX_TIMER_HANDLER_END

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

DX_TIMER_HANDLER(tmr_i8080_wakeup_handler)
{
#ifdef AZURE_SPHERE
    altair_wake();
#endif
}
DX_TIMER_HANDLER_END

/*******************************************************************************************************************************************************
 * Async handlers. These async handlers marshall calls from threads calling event loop functions of the event
 *loop running on the main thread.
 *******************************************************************************************************************************************************/

DX_ASYNC_HANDLER(async_power_management_enable_handler, handle)
{
#ifdef OEM_AVNET
    dx_timerStart(&tmr_terminal_io_monitor);
#endif // OEM_AVNET
}
DX_ASYNC_HANDLER_END

DX_ASYNC_HANDLER(async_power_management_disable_handler, handle)
{
#ifdef OEM_AVNET
    dx_timerStop(&tmr_terminal_io_monitor);
#endif // OEM_AVNET
}
DX_ASYNC_HANDLER_END

DX_ASYNC_HANDLER(async_power_management_sleep_handler, handle)
{
#ifdef OEM_AVNET
    altair_sleep();
#endif // OEM_AVNET
}
DX_ASYNC_HANDLER_END

DX_ASYNC_HANDLER(async_power_management_wake_handler, handle)
{
#ifdef OEM_AVNET
    dx_timerOneShotSet(&tmr_i8080_wakeup, &(struct timespec){*((int *)handle->data), 0});
#endif // OEM_AVNET
}
DX_ASYNC_HANDLER_END

DX_ASYNC_HANDLER(async_accelerometer_start_handler, handle)
{
#ifdef OEM_AVNET
    avnet_get_temperature_lps22h(); // This is a hack to initialize the accelerometer :)
    dx_timerStart(&tmr_read_accelerometer);
    dx_timerOneShotSet(&tmr_read_accelerometer, &(struct timespec){0, 10 * ONE_MS});
#endif // OEM_AVNET
}
DX_ASYNC_HANDLER_END

DX_ASYNC_HANDLER(async_accelerometer_stop_handler, handle)
{
#ifdef OEM_AVNET
    dx_timerStop(&tmr_read_accelerometer);
#endif // OEM_AVNET
}
DX_ASYNC_HANDLER_END

DX_ASYNC_HANDLER(async_copyx_request_handler, handle)
{
    copy_web(webget.url);
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

/*******************************************************************************************************************************************************
 * i8080 output port implementations
 *******************************************************************************************************************************************************/

/// <summary>
/// Intel 8080 OUT Port handler
/// </summary>
/// <param name="port"></param>
/// <param name="data"></param>
void io_port_out(uint8_t port, uint8_t data)
{
    memset(&ru, 0x00, sizeof(REQUEST_UNIT_T));
    static int timer_delay;
    static int wake_delay;
    static int timer_milliseconds_delay;

#ifdef AZURE_SPHERE
    Applications_OsVersion os_version;
#endif

#if defined(ALTAIR_FRONT_PANEL_RETRO_CLICK) || defined(ALTAIR_FRONT_PANEL_PI_SENSE)
    union {
        uint32_t mask[2];
        uint64_t mask64;
    } pixel_mask;
#endif

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
        case 33: // copy file from web server to mutable storage
            if (webget.index == 0)
            {
                memset(webget.filename, 0x00, sizeof(webget.filename));
            }

            if (data != 0 && webget.index < sizeof(webget.filename))
            {
                webget.filename[webget.index] = data;
                webget.index++;
            }

            if (data == 0) // NULL TERMINATION
            {
                webget.index              = 0;
                webget.status             = WEBGET_WAITING;
                webget.byte_stream_length = 0;
                webget.end_of_file        = false;
                pthread_mutex_unlock(&webget_mutex);

                memset(webget.url, 0x00, sizeof(webget.url));
                snprintf(webget.url, sizeof(webget.url), "%s/%s", PERSONAL_REPO, webget.filename);

                dx_asyncSend(&async_copyx_request, NULL);
            }
            break;
        case 32:
            if (!publish_weather_pending)
            {
                publish_weather_pending = true;
                dx_asyncSend(&async_publish_weather, NULL);
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
        case 63: // Onboard sensors temperature, pressure, and light
            switch (data)
            {
                case 0:
                    // Temperature minus 9 is super rough calibration
                    ru.len = (size_t)snprintf(
                        ru.buffer, sizeof(ru.buffer), "%d", (int)onboard_get_temperature() - 9);
                    break;
                case 1:
                    ru.len =
                        (size_t)snprintf(ru.buffer, sizeof(ru.buffer), "%d", (int)onboard_get_pressure());
                    break;
                case 2:
#ifdef OEM_AVNET
                    ru.len =
                        (size_t)snprintf(ru.buffer, sizeof(ru.buffer), "%d", avnet_get_light_level() * 2);
#else
                    ru.len = (size_t)snprintf(ru.buffer, sizeof(ru.buffer), "%d", 0);
#endif // OEM_AVNET
                    break;
            }

            break;
#endif // AZURE_SPHERE

#ifdef OEM_AVNET
        case 64:
            switch (data)
            {
                case 0: // accelerometer X
                    ru.len = (size_t)snprintf(ru.buffer, sizeof(ru.buffer), "%f", x);
                    break;
                case 1: // accelerometer Y
                    ru.len = (size_t)snprintf(ru.buffer, sizeof(ru.buffer), "%f", y);
                    break;
                case 2: // accelerometer Z
                    ru.len = (size_t)snprintf(ru.buffer, sizeof(ru.buffer), "%f", z);
                    break;
                case 3: // accelerometer start feeding TinyML model running on real-time core
                    if (!accelerometer_running)
                    {
                        dx_asyncSend(&async_accelerometer_start, NULL);
                        accelerometer_running = true;
                    }
                    break;
                case 4: // accelerometer stop feeding TinyML model running on real-time core
                    if (accelerometer_running)
                    {
                        dx_asyncSend(&async_accelerometer_stop, NULL);
                        accelerometer_running = false;
                    }
                    break;
                case 5: // Load accelerometer x,y,z
                    if (!accelerometer_running)
                    {
                        avnet_get_acceleration(&x, &y, &z);
                    }
                    break;
                case 6: // Calibrate accelerometer for angular rate
                    if (!accelerometer_running)
                    {
                        avnet_calibrate_angular_rate();
                    }
                    break;
                case 7: // Load accelerometer for angular rate
                    if (!accelerometer_running)
                    {
                        avnet_get_angular_rate(&x, &y, &z);
                    }
                    break;
                case 8: // Get Tiny ML latest movement inference result
                    ru.len = (size_t)snprintf(ru.buffer, sizeof(ru.buffer), "%s", PREDICTION);
                    break;
            }
            break;
#endif // OEM_AVNET

#ifdef ALTAIR_FRONT_PANEL_RETRO_CLICK
        case 65: // Enable/Disable 8x8 LED Panel and 4x4 keypad
            if (data == 0)
            {
                dx_timerStop(&tmr_read_panel);
                dx_timerStop(&tmr_refresh_panel);
                as1115_clear(&retro_click);
            }
            else
            {
                dx_timerStart(&tmr_read_panel);
                dx_timerStart(&tmr_refresh_panel);
                as1115_set_brightness(&retro_click, (unsigned char)(data - 1));
            }
            break;
#endif

#ifdef AZURE_SPHERE
        case 66: // enable/disable power management

            switch (data)
            {
                case 0:
                    dx_asyncSend(&async_power_management_disable, NULL);
                    break;
                case 1:
                    dx_asyncSend(&async_power_management_enable, NULL);
                    break;
                case 2:
                    dx_asyncSend(&async_power_management_sleep, NULL);
                    break;
                default:
                    break;
            }

            break;

        case 67: // wake from sleep in seconds
            if (data > 0)
            {
                wake_delay = data;
                dx_asyncSend(&async_power_management_wake, (void *)&wake_delay);
            }
            break;

        case 68: // set devget filename
            if (devget.index == 0)
            {
                if (devget.file_opened && devget.fd != -1)
                {
                    close(devget.fd);
                }
                memset(&devget, 0x00, sizeof(DEVGET_T));
                devget.fd = -1;
            }

            if (data != 0 && devget.index < sizeof(devget.filename))
            {
                devget.filename[devget.index] = data;
                devget.index++;
            }

            if (data == 0) // NULL TERMINATION
            {
                devget.index = 0;
            }

            break;

#endif // AZURE SPHERE

        case 70: // Load Altair version number
            ru.len = (size_t)snprintf(ru.buffer, sizeof(ru.buffer), "%s", ALTAIR_EMULATOR_VERSION);
            break;

#ifdef AZURE_SPHERE

        case 71: // OS Version
            Applications_GetOsVersion(&os_version);
            ru.len = (size_t)snprintf(ru.buffer, sizeof(ru.buffer), "%s", os_version.version);
            break;

        case 72: // Get Azure Sphere Device ID - note for security reasons only getting the first 8 chars on
                 // the 128 string
            memset(azsphere_device_id, 0x00, sizeof(azsphere_device_id));
            if (GetDeviceID((char *)&azsphere_device_id, sizeof(azsphere_device_id)) == 0)
            {
                ru.len = (size_t)snprintf(ru.buffer, sizeof(ru.buffer), "%s", azsphere_device_id);
            }
            else
            {
                ru.len = (size_t)snprintf(ru.buffer, sizeof(ru.buffer), "%s", "A7B43940");
            }
            break;

#endif // AZURE_SPHERE

#ifdef ALTAIR_FRONT_PANEL_RETRO_CLICK

        case 80: // panel_mode 0 = bus data, 1 = font, 2 = bitmap
            if (data < 3)
            {
                panel_mode = data;
            }
            break;
        case 85: // display character
            gfx_load_character(data, retro_click.bitmap);
            gfx_rotate_counterclockwise(retro_click.bitmap, 1, 1, retro_click.bitmap);
            gfx_reverse_panel(retro_click.bitmap);
            gfx_rotate_counterclockwise(retro_click.bitmap, 1, 1, retro_click.bitmap);
            as1115_panel_write(&retro_click);
            break;

#endif // ALTAIR_FRONT_PANEL_RETRO_CLICK

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

#if defined(ALTAIR_FRONT_PANEL_RETRO_CLICK) || defined(ALTAIR_FRONT_PANEL_PI_SENSE)

        case 90: // Bitmap row 0
            pixel_map.bitmap[0] = data;
            break;
        case 91: // Bitmap row 1
            pixel_map.bitmap[1] = data;
            break;
        case 92: // Bitmap row 2
            pixel_map.bitmap[2] = data;
            break;
        case 93: // Bitmap row 3
            pixel_map.bitmap[3] = data;
            break;
        case 94: // Bitmap row 4
            pixel_map.bitmap[4] = data;
            break;
        case 95: // Bitmap row 5
            pixel_map.bitmap[5] = data;
            break;
        case 96: // Bitmap row 6
            pixel_map.bitmap[6] = data;
            break;
        case 97: // Bitmap row 7
            pixel_map.bitmap[7] = data;
            break;
        case 98: // Pixel on
            if (data < 64)
            {
                pixel_mask.mask64                 = 0;
                pixel_mask.mask[(int)(data / 32)] = data < 32 ? 1u << data : 1u << (data - 32);
                pixel_map.bitmap64                = pixel_map.bitmap64 | pixel_mask.mask64;
            }
            break;
        case 99: // Pixel off
            if (data < 64)
            {
                pixel_mask.mask64                 = 0;
                pixel_mask.mask[(int)(data / 32)] = data < 32 ? 1u << data : 1u << (data - 32);
                pixel_mask.mask64 ^= 0xFFFFFFFFFFFFFFFF;
                pixel_map.bitmap64 = pixel_map.bitmap64 & pixel_mask.mask64;
            }
            break;
        case 100: // Pixel flip
            if (data < 64)
            {
                pixel_mask.mask64                 = 0;
                pixel_mask.mask[(int)(data / 32)] = data < 32 ? 1u << data : 1u << (data - 32);
                pixel_map.bitmap64                = pixel_map.bitmap64 ^ pixel_mask.mask64;
            }
            break;
        case 101: // clear all pixels
            pixel_map.bitmap64 = 0;
            break;

#endif // defined(ALTAIR_FRONT_PANEL_RETRO_CLICK) || defined(ALTAIR_FRONT_PANEL_PI_SENSE)

#ifdef ALTAIR_FRONT_PANEL_RETRO_CLICK
        case 102: // Bitmap draw
            gfx_rotate_counterclockwise(pixel_map.bitmap, 1, 1, retro_click.bitmap);
            gfx_reverse_panel(retro_click.bitmap);
            gfx_rotate_counterclockwise(retro_click.bitmap, 1, 1, retro_click.bitmap);
            // retro_click.bitmap64 = pixel_map.bitmap64;
            as1115_panel_write(&retro_click);
            break;

#endif // ALTAIR_FRONT_PANEL_RETRO_CLICK

#ifdef ALTAIR_FRONT_PANEL_PI_SENSE
        case 102: // Bitmap draw
            gfx_rotate_counterclockwise(pixel_map.bitmap, 1, 1, bitmap);
            gfx_reverse_panel(bitmap);
            gfx_rotate_counterclockwise(bitmap, 1, 1, bitmap);
            gfx_bitmap_to_rgb(bitmap, panel_8x8_buffer, sizeof(panel_8x8_buffer));
            pi_sense_8x8_panel_update(panel_8x8_buffer, sizeof(panel_8x8_buffer));
            break;
#endif // PI SENSE HAT
        case 110:
            if (webget.index == 0)
            {
                memset(webget.personal_endpoint, 0x00, ENDPOINT_LEN);
            }

            if (data != 0 && webget.index < ENDPOINT_LEN)
            {
                webget.personal_endpoint[webget.index++] = data;
            }

            if (data == 0) // NULL TERMINATION
            {
                webget.personal_endpoint[webget.index] = 0x00;
                webget.index                           = 0;
            }
            break;
        case 111:
            ru.len = (size_t)snprintf(ru.buffer, sizeof(ru.buffer), "%s", webget.personal_endpoint);
            break;
        case 112:
            if (data < ENDPOINT_ELEMENTS)
            {
                webget.selected_endpoint = data;
            }
            break;
        case 113:
            ru.len = (size_t)snprintf(ru.buffer, sizeof(ru.buffer), "%d", webget.selected_endpoint);
            break;
        case 114: // copy file from web server to mutable storage
            if (webget.index == 0)
            {
                memset(webget.filename, 0x00, sizeof(webget.filename));
            }

            if (data != 0 && webget.index < sizeof(webget.filename))
            {
                webget.filename[webget.index] = data;
                webget.index++;
            }

            if (data == 0) // NULL TERMINATION
            {
                webget.index              = 0;
                webget.status             = WEBGET_WAITING;
                webget.byte_stream_length = 0;
                webget.end_of_file        = false;
                pthread_mutex_unlock(&webget_mutex);

                memset(webget.url, 0x00, sizeof(webget.url));

                switch (webget.selected_endpoint)
                {
                    case 0:
                        snprintf(webget.url, sizeof(webget.url), "%s/%s", GAMES_REPO, webget.filename);
                        break;

                    case 1:
                        snprintf(webget.url, sizeof(webget.url), "%s/%s", webget.personal_endpoint,
                            webget.filename);
                        break;
                    default:
                        break;
                }
                dx_asyncSend(&async_copyx_request, NULL);
            }
            break;
        default:
            break;
    }
}

/*******************************************************************************************************************************************************
 * i8080 input port implementations
 *******************************************************************************************************************************************************/

/// <summary>
/// Intel 8080 IN Port handler
/// </summary>
/// <param name="port"></param>
/// <returns></returns>
uint8_t io_port_in(uint8_t port)
{
    uint8_t retVal = 0;
    char devget_path_and_filename[50];

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
            retVal = webget.status;
            break;
        case 68: // has devget eof
            retVal = devget.end_of_file;
            break;
        case 69: // get network ready state
            retVal = dx_isNetworkReady();
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
        case 201: // Read file from http(s) web server
            if (webget.byte_stream_length > 0)
            {
                retVal = webget.byte_stream[0];
                webget.byte_stream++;
                webget.byte_stream_length--;

                if (webget.byte_stream_length == 0)
                {
                    webget.status = webget.end_of_file ? WEBGET_EOF : WEBGET_WAITING;
                    pthread_mutex_unlock(&webget_mutex);
                }
            }
            else
            {
                retVal = 0x00;
            }
            break;
#ifdef AZURE_SPHERE

        case 202: // READ DEVGET file from immutable storage
            if (devget.end_of_file)
            {
                retVal = 0x00;
            }
            else
            {
                if (!devget.file_opened)
                {
                    /* open the file */
                    snprintf(devget_path_and_filename, sizeof(devget_path_and_filename), "%s/%s",
                        APP_SAMPLES_DIRECTORY, devget.filename);

                    if ((devget.fd = Storage_OpenFileInImagePackage(devget_path_and_filename)) != -1)
                    {
                        devget.file_opened = true;
                    }
                    else
                    {
                        devget.end_of_file = true;
                    }
                }

                if (devget.file_opened)
                {
                    if (read(devget.fd, &retVal, 1) == 0)
                    {
                        close(devget.fd);

                        devget.file_opened = false;
                        devget.end_of_file = true;
                        devget.fd          = -1;
                        retVal             = 0x00;
                    }
                }
                else
                {
                    retVal = 0x00;
                }
            }
            break;

#endif // AZURE SPHERE
        default:
            retVal = 0x00;
    }

    return retVal;
}

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *webget)
{
    size_t realsize = size * nmemb;
    WEBGET_T *wg    = (WEBGET_T *)webget;

    pthread_mutex_lock(&webget_mutex);

    memcpy(wg->bytes, ptr, realsize);

    wg->byte_stream        = wg->bytes;
    wg->byte_stream_length = realsize;
    wg->status             = WEBGET_DATA_READY;

    return realsize;
}

static int copy_web(char *url)
{
    CURL *curl_handle;

    /* init the curl session */
    curl_handle = curl_easy_init();

    /* set URL to get here */
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);

    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);

    // https://curl.se/libcurl/c/CURLOPT_NOSIGNAL.html
    curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1L);

    // https://curl.se/libcurl/c/CURLOPT_TIMEOUT.html
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 12L);

    /* Switch on full protocol/debug output while testing */
    // curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);

    /* disable progress meter, set to 0L to enable it */
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);

    // set returned data chuck to max 128 bytes
    curl_easy_setopt(curl_handle, CURLOPT_BUFFERSIZE, 1024L);

    /* send all data to this function  */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);

    // A long parameter set to 1 tells the library to fail the request if the HTTP code returned is equal to
    // or larger than 400
    curl_easy_setopt(curl_handle, CURLOPT_FAILONERROR, true);

    webget.status      = WEBGET_WAITING;
    webget.end_of_file = false;

    /* write the page body to this file handle */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &webget);

    /* get it! */
    CURLcode res = curl_easy_perform(curl_handle);

    if (res != CURLE_OK)
    {
        webget.status = WEBGET_FAILED;
    }

    webget.end_of_file = true;

    /* cleanup curl stuff */
    curl_easy_cleanup(curl_handle);

    curl_global_cleanup();

    return 0;
}
