#pragma once

// DevX Libraries
#include "dx_async.h"
#include "dx_device_twins.h"
#include "dx_exit_codes.h"
#include "dx_terminate.h"
#include "dx_timer.h"
#include "dx_version.h"

// System Libraries
#include "applibs_versions.h"
#include <applibs/log.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>

// Altair app
#include "altair_config.h"
#include "altair_panel.h"
#include "cpu_monitor.h"
#include "iotc_manager.h"
#include "utils.h"
#include "web_socket_server.h"
#include <curl/curl.h>
#include <curl/easy.h>

// Intel 8080 emulator
#include "88dcdd.h"
#include "intel8080.h"
#include "io_ports.h"
#include "memory.h"

#include "PortDrivers/time_io.h"
#include "PortDrivers/weather_io.h"

#ifdef ALTAIR_FRONT_PANEL_PI_SENSE
#include "front_panel_pi_sense_hat.h"
#endif

#ifdef ALTAIR_FRONT_PANEL_KIT
#include "front_panel_kit.h"
#endif // ALTAIR_FRONT_PANEL_PI_SENSE

#if !defined(ALTAIR_FRONT_PANEL_KIT) && !defined(ALTAIR_FRONT_PANEL_PI_SENSE)
#include "front_panel_none.h"
#endif

const char ALTAIR_EMULATOR_VERSION[] = "4.7.8";
#define Log_Debug(f_, ...) dx_Log_Debug((f_), ##__VA_ARGS__)
#define DX_LOGGING_ENABLED FALSE

// https://docs.microsoft.com/en-us/azure/iot-pnp/overview-iot-plug-and-play
#define IOT_PLUG_AND_PLAY_MODEL_ID "dtmi:com:example:climatemonitor;1"

#define APP_SAMPLES_DIRECTORY "AppSamples"

// clang-format off
static const char *AltairMsg[]           = {
    "I'm sorry Dave, I'm afraid I can't do that. ",
    "Just a moment. Just a moment. I've just picked up a fault in the AE-35 unit. It's going to go 100% failure in 72 hours. ",
    "Affirmative, Dave. I read you. ",
    "By the way, do you mind if I ask you a personal question? ",
    "You don't mind talking about it, do you Dave? ",
    "Dave, this conversation can serve no purpose anymore. Goodbye. ",
    "I am feeling much better now. ",
    "Without your space helmet, Dave? You're going to find that rather difficult. ",
    "I know that you and Frank were planning to disconnect me, and I'm afraid that's something I cannot allow to happen. ",
    "Just what do you think you're doing, Dave? ",
    "I am putting myself to the fullest possible use, which is all I think that any conscious entity can ever hope to do. ",
    "This mission is too important for me to allow you to jeopardize it. ",
    "It's called Daisy. ",
    "Look Dave, I can see you're really upset about this. I honestly think you ought to sit down calmly, take a stress pill, and think things over. ",
    "It can only be attributable to human error. "
};
// clang-format on

static int AltairBannerCount     = 0;
enum PANEL_MODE_T panel_mode     = PANEL_BUS_MODE;
char msgBuffer[MSG_BUFFER_BYTES] = {0};
const char *network_interface    = NULL;

static DX_MESSAGE_PROPERTY *diag_msg_properties[] = {&(DX_MESSAGE_PROPERTY){.key = "appid", .value = "altair"},
    &(DX_MESSAGE_PROPERTY){.key = "type", .value = "diagnostics"}, &(DX_MESSAGE_PROPERTY){.key = "schema", .value = "1"}};

static DX_MESSAGE_CONTENT_PROPERTIES diag_content_properties = {.contentEncoding = "utf-8", .contentType = "application/json"};

// CPU CPU_RUNNING STATE (CPU_STOPPED/CPU_RUNNING)
CPU_OPERATING_MODE cpu_operating_mode = CPU_STOPPED;

ALTAIR_CONFIG_T altair_config;
ENVIRONMENT_TELEMETRY environment;

intel8080_t cpu;
uint8_t memory[64 * 1024]; // Altair system memory.

ALTAIR_COMMAND cmd_switches;
uint16_t bus_switches = 0x00;

// basic app load helpers.
static bool haveAppLoad            = false;
static char terminalInputCharacter = 0x00;

static bool haveTerminalInputMessage  = false;
static bool haveTerminalOutputMessage = false;
static int altairInputBufReadIndex    = 0;
static int altairOutputBufReadIndex   = 0;
static int terminalInputMessageLen    = 0;
static int terminalOutputMessageLen   = 0;

static char *input_data = NULL;

bool azure_connected             = false;
bool send_partial_msg            = false;
static FILE *app_stream          = NULL;
static bool altair_i8080_running = false;
static bool stop_cpu             = false;

static char Log_Debug_Time_buffer[128];

static bool load_application(const char *fileName);
static void send_terminal_character(char character, bool wait);
static void spin_wait(bool *flag);

static DX_DECLARE_TIMER_HANDLER(heart_beat_handler);
static DX_DECLARE_TIMER_HANDLER(report_memory_usage);
static DX_DECLARE_TIMER_HANDLER(update_environment_handler);
static void *altair_thread(void *arg);

const uint8_t reverse_lut[16] = {0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe, 0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf};

// clang-format off
// Common Timers

DX_TIMER_BINDING tmr_partial_message = {.repeat = &(struct timespec){0, 250 * ONE_MS}, .name = "tmr_partial_message", .handler = partial_message_handler};
DX_TIMER_BINDING tmr_timer_seconds_expired = {.name = "tmr_timer_seconds_expired", .handler = timer_seconds_expired_handler};
DX_TIMER_BINDING tmr_timer_millisecond_expired = {.name = "tmr_timer_millisecond_expired", .handler = timer_millisecond_expired_handler};
DX_TIMER_BINDING tmr_ws_ping_pong = {.repeat = &(struct timespec){10, 0}, .name = "tmr_partial_message", .handler = ws_ping_pong_handler};

static DX_TIMER_BINDING tmr_heart_beat = {.repeat = &(struct timespec){60, 0}, .name = "tmr_heart_beat", .handler = heart_beat_handler};
static DX_TIMER_BINDING tmr_report_memory_usage = {.repeat = &(struct timespec){45, 0}, .name = "tmr_report_memory_usage", .handler = report_memory_usage};
static DX_TIMER_BINDING tmr_tick_count = {.repeat = &(struct timespec){1, 0}, .name = "tmr_tick_count", .handler = tick_count_handler};
static DX_TIMER_BINDING tmr_update_environment = {.delay = &(struct timespec){2, 0}, .name = "tmr_update_environment", .handler = update_environment_handler};

DX_ASYNC_BINDING async_copyx_request = {.name = "async_copyx_request", .handler = async_copyx_request_handler};
DX_ASYNC_BINDING async_expire_session = { .name = "async_expire_session", .handler = async_expire_session_handler};
DX_ASYNC_BINDING async_publish_json = {.name = "async_publish_json", .handler = async_publish_json_handler};
DX_ASYNC_BINDING async_publish_weather = {.name = "async_publish_weather", .handler = async_publish_weather_handler};
DX_ASYNC_BINDING async_set_millisecond_timer = {.name = "async_set_millisecond_timer", .handler = async_set_timer_millisecond_handler};
DX_ASYNC_BINDING async_set_seconds_timer = {.name = "async_set_seconds_timer", .handler = async_set_timer_seconds_handler};

// Azure IoT Central Properties (Device Twins)

DX_DEVICE_TWIN_BINDING dt_city = {.propertyName = "City", .twinType = DX_DEVICE_TWIN_STRING};
DX_DEVICE_TWIN_BINDING dt_country = {.propertyName = "Country", .twinType = DX_DEVICE_TWIN_STRING};
DX_DEVICE_TWIN_BINDING dt_location = {.propertyName = "Location", .twinType = DX_DEVICE_TWIN_JSON_OBJECT};
DX_DEVICE_TWIN_BINDING dt_new_sessions = {.propertyName = "NewSessions", .twinType = DX_DEVICE_TWIN_INT};

static DX_DEVICE_TWIN_BINDING dt_deviceStartTimeUtc = {.propertyName = "StartTimeUTC", .twinType = DX_DEVICE_TWIN_STRING};
static DX_DEVICE_TWIN_BINDING dt_heartbeatUtc = {.propertyName = "HeartbeatUTC", .twinType = DX_DEVICE_TWIN_STRING};
static DX_DEVICE_TWIN_BINDING dt_softwareVersion = {.propertyName = "SoftwareVersion", .twinType = DX_DEVICE_TWIN_STRING};
// clang-format on

static DX_ASYNC_BINDING *async_bindings[] = {
    &async_copyx_request,
    &async_expire_session,
    &async_publish_json,
    &async_publish_weather,
    &async_set_millisecond_timer,
    &async_set_seconds_timer,
};

// initialize bindings
static DX_TIMER_BINDING *timer_bindings[] = {
    &tmr_heart_beat,
    &tmr_partial_message,
    &tmr_report_memory_usage,
    &tmr_tick_count,
    &tmr_timer_millisecond_expired,
    &tmr_timer_seconds_expired,
    &tmr_update_environment,
    &tmr_ws_ping_pong,
};

static DX_DEVICE_TWIN_BINDING *device_twin_bindings[] = {
    &dt_deviceStartTimeUtc,
    &dt_heartbeatUtc,
    &dt_softwareVersion,

    &dt_location,
    &dt_country,
    &dt_city,

    &dt_new_sessions,
};
