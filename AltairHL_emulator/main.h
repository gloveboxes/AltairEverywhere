#pragma once

// DevX Libraries
#include "dx_config.h"
#include "dx_device_twins.h"
#include "dx_exit_codes.h"
#include "dx_terminate.h"
#include "dx_timer.h"
#include "dx_version.h"

// System Libraries
#include "applibs_versions.h"
#include <applibs/log.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>

// Altair app
#include "comms_manager_wolf.h"
#include <curl/curl.h>
#include <curl/easy.h>
#include "front_panel_virtual.h"
#include "iotc_manager.h"
#include "sphere_panel.h"
#include "utils.h"
#include "weather.h"

// Intel 8080 emulator
#include "intel8080.h"
#include "88dcdd.h"
#include "memory.h"

#ifdef ALTAIR_FRONT_PANEL_PI_SENSE
#include "front_panel_pi_sense_hat.h"
#else
#include "front_panel_none.h"
#endif // ALTAIR_FRONT_PANEL_PI_SENSE

#define ALTAIR_ON_AZURE_SPHERE_VERSION "3.0"
#define Log_Debug(f_, ...) dx_Log_Debug((f_), ##__VA_ARGS__)
#define DX_LOGGING_ENABLED FALSE

// https://docs.microsoft.com/en-us/azure/iot-pnp/overview-iot-plug-and-play
#define IOT_PLUG_AND_PLAY_MODEL_ID "dtmi:com:example:emulator:altair;1"
#define NETWORK_INTERFACE "wlan0"
DX_USER_CONFIG userConfig;

#define BASIC_SAMPLES_DIRECTORY "BasicSamples"

static const char *AltairMsg = "\x1b[2J\r\nAzure Sphere - Altair 8800 Emulator\r\n";

// local variables

char msgBuffer[MSG_BUFFER_BYTES] = {0};

// CPU CPU_RUNNING STATE (CPU_STOPPED/CPU_RUNNING)
CPU_OPERATING_MODE cpu_operating_mode = CPU_STOPPED;

static intel8080_t cpu;
uint8_t memory[64 * 1024]; // Altair system memory.

ALTAIR_COMMAND cmd_switches;
uint16_t bus_switches = 0x00;

// basic app load helpers.
static bool haveCtrlPending = false;
static char haveCtrlCharacter = 0x00;
static bool haveAppLoad = false;
static int basicAppLength = 0;
static int appLoadPtr = 0;
static uint8_t *ptrBasicApp = NULL;

static bool haveTerminalInputMessage = false;
static bool haveTerminalOutputMessage = false;
static int terminalInputMessageLen = 0;
static int terminalOutputMessageLen = 0;
static int altairInputBufReadIndex = 0;
static int altairOutputBufReadIndex = 0;

static pthread_cond_t wait_message_processed_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t wait_message_processed_mutex = PTHREAD_MUTEX_INITIALIZER;
static char *input_data = NULL;

bool dirty_buffer = false;
bool send_messages = false;
bool send_ping = false;
bool renderText = false;

static WEATHER_TELEMETRY onboard_telemetry;
static WEATHER_TELEMETRY weather;

static char Log_Debug_Time_buffer[128];

static DX_DECLARE_TIMER_HANDLER(mqtt_work_scheduler_handler);
static DX_DECLARE_TIMER_HANDLER(panel_refresh_handler);
static DX_DECLARE_TIMER_HANDLER(report_memory_usage);
static DX_DECLARE_TIMER_HANDLER(measure_sensor_handler);
static void *altair_thread(void *arg);
static void process_control_panel_commands(void);

const uint8_t reverse_lut[16] = {0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe, 0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf};

// Common Timers
static DX_TIMER_BINDING tmr_mqtt_do_work = {.repeat = &(struct timespec){0, 250 * OneMS}, .name = "tmr_mqtt_do_work", .handler = mqtt_work_scheduler_handler};
static DX_TIMER_BINDING tmr_report_memory_usage = {.repeat = &(struct timespec){30, 0}, .name = "tmr_report_memory_usage", .handler = report_memory_usage};
static DX_TIMER_BINDING tmr_measure_sensor = {.repeat = &(struct timespec){5, 0}, .name = "tmr_measure_sensor", .handler = measure_sensor_handler};


#ifdef ALTAIR_FRONT_PANEL_PI_SENSE
static DX_TIMER_BINDING tmr_panel_refresh = {.delay = &(timespec){0, 10 * OneMS}, .name = "tmr_panel_refresh", .handler = panel_refresh_handler};
#else  // else create a disabled timer
static DX_TIMER_BINDING tmr_panel_refresh = {.name = "tmr_panel_refresh", .handler = panel_refresh_handler};
#endif // ALTAIR_FRONT_PANEL_PI_SENSE

// Azure IoT Central Properties (Device Twins)
DX_DEVICE_TWIN_BINDING dt_countryCode = {.propertyName = "CountryCode", .twinType = DX_DEVICE_TWIN_STRING};
DX_DEVICE_TWIN_BINDING dt_humidity = {.propertyName = "Humidity", .twinType = DX_DEVICE_TWIN_INT};
DX_DEVICE_TWIN_BINDING dt_ledBrightness = {.propertyName = "LedBrightness", .twinType = DX_DEVICE_TWIN_INT, .handler = set_led_brightness_handler};
DX_DEVICE_TWIN_BINDING dt_location = {.propertyName = "Location", .twinType = DX_DEVICE_TWIN_JSON_OBJECT};
DX_DEVICE_TWIN_BINDING dt_pressure = {.propertyName = "Pressure", .twinType = DX_DEVICE_TWIN_INT};
DX_DEVICE_TWIN_BINDING dt_temperature = {.propertyName = "Temperature", .twinType = DX_DEVICE_TWIN_INT};
DX_DEVICE_TWIN_BINDING dt_weather = {.propertyName = "Weather", .twinType = DX_DEVICE_TWIN_STRING};
static DX_DEVICE_TWIN_BINDING dt_deviceStartTime = {.propertyName = "StartTime", .twinType = DX_DEVICE_TWIN_STRING};
static DX_DEVICE_TWIN_BINDING dt_softwareVersion = {.propertyName = "SoftwareVersion", .twinType = DX_DEVICE_TWIN_STRING};


// initialize bindings
static DX_TIMER_BINDING *timer_bindings[] = {&tmr_mqtt_do_work, &tmr_panel_refresh, &tmr_report_memory_usage, &tmr_measure_sensor};
static DX_DEVICE_TWIN_BINDING *device_twin_bindings[] = {&dt_deviceStartTime, &dt_softwareVersion, &dt_ledBrightness, &dt_temperature, &dt_pressure,
                                                         &dt_humidity,        &dt_weather,         &dt_location,      &dt_countryCode};
