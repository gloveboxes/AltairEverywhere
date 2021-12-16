/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

   // Hardware definition
#include "hw/azure_sphere_learning_path.h"

// System Libraries
#include "applibs_versions.h"
#include <applibs/log.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <applibs/storage.h>

#include "curldefs.h"
#include "weather.h"
#include "utils.h"
#include "comms_manager_wolf.h"
#include "front_panel_virtual.h"
#include "iotc_manager.h"

#include "intel8080.h"
#include "88dcdd.h"
#include "sphere_panel.h"
#include "memory.h"

#include "main.h"

#define ALTAIR_ON_AZURE_SPHERE_VERSION "3.0"

#define Log_Debug(f_, ...) dx_Log_Debug((f_), ##__VA_ARGS__)
#define DX_LOGGING_ENABLED FALSE

// https://docs.microsoft.com/en-us/azure/iot-pnp/overview-iot-plug-and-play
#define IOT_PLUG_AND_PLAY_MODEL_ID "dtmi:com:example:azuresphere:altair;1"
#define NETWORK_INTERFACE "wlan0"
DX_USER_CONFIG userConfig;

#define BASIC_SAMPLES_DIRECTORY "BasicSamples"

static const char* AltairMsg = "\x1b[2J\r\nAzure Sphere - Altair 8800 Emulator\r\n";

char msgBuffer[MSG_BUFFER_BYTES] = { 0 };

// CPU CPU_RUNNING STATE (CPU_STOPPED/CPU_RUNNING)
CPU_OPERATING_MODE cpu_operating_mode = CPU_STOPPED;

static intel8080_t cpu;
uint8_t memory[64 * 1024]; // Altair system memory.

ALTAIR_COMMAND cmd_switches;
uint16_t bus_switches = 0x00;

int altair_spi_fd = -1;
int console_fd = -1;

const struct itimerspec watchdogInterval = { {60, 0}, {60, 0} };
timer_t watchdogTimer;

// basic app load helpers.
static bool haveCtrlPending = false;
static char haveCtrlCharacter = 0x00;
static bool haveAppLoad = false;
static int basicAppLength = 0;
static int appLoadPtr = 0;
static uint8_t* ptrBasicApp = NULL;

static bool haveTerminalInputMessage = false;
static bool haveTerminalOutputMessage = false;
static int terminalInputMessageLen = 0;
static int terminalOutputMessageLen = 0;
static int altairInputBufReadIndex = 0;
static int altairOutputBufReadIndex = 0;

static pthread_cond_t wait_message_processed_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t wait_message_processed_mutex = PTHREAD_MUTEX_INITIALIZER;
static char* input_data = NULL;

bool local_serial = true;
bool dirty_buffer = false;
bool send_messages = false;
bool invoke_mqtt_sync = false;
bool renderText = false;

static char Log_Debug_Time_buffer[64];

// End of variable declarations

/// <summary>
/// This timer extends the app level lease watchdog timer
/// </summary>
/// <param name="eventLoopTimer"></param>
static void WatchdogMonitorTimerHandler(EventLoopTimer* eventLoopTimer) {
	if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0) {
		dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
		return;
	}
	timer_settime(watchdogTimer, 0, &watchdogInterval, NULL);
}

/// <summary>
/// Set up watchdog timer - the lease is extended via the WatchdogMonitorTimerHandler function
/// </summary>
/// <param name=""></param>
void SetupWatchdog(void) {
	struct sigevent alarmEvent;
	alarmEvent.sigev_notify = SIGEV_SIGNAL;
	alarmEvent.sigev_signo = SIGALRM;
	alarmEvent.sigev_value.sival_ptr = &watchdogTimer;

	if (timer_create(CLOCK_MONOTONIC, &alarmEvent, &watchdogTimer) == 0) {
		if (timer_settime(watchdogTimer, 0, &watchdogInterval, NULL) == -1) {
			Log_Debug("Issue setting watchdog timer. %s %d\n", strerror(errno), errno);
		}
	}
}

/// <summary>
/// Report on first connect the software version and device startup UTC time
/// </summary>
/// <param name="connected"></param>
static void azure_connection_changed(bool connected) {
    static bool first_time_only = true;
    if (first_time_only) {
        first_time_only = false;
        snprintf(msgBuffer, sizeof(msgBuffer), "Altair emulator version: %s, DevX version: %s", ALTAIR_ON_AZURE_SPHERE_VERSION, AZURE_SPHERE_DEVX_VERSION);
        dx_deviceTwinReportValue(&dt_softwareVersion, msgBuffer);
        dx_deviceTwinReportValue(&dt_reportedDeviceStartTime, dx_getCurrentUtc(msgBuffer, sizeof(msgBuffer))); // DX_TYPE_STRING
	}
}

static void mqtt_connected_cb(void) {
	static bool connection_initialised = false;
	static const char* connected_message = "\r\nCONNECTED TO AZURE SPHERE ALTAIR 8800 EMULATOR VERSION: %s, DevX VERSION: %s.\r\n\r\n";
	static const char* reconnected_message =
		"\r\nRECONNECTED TO AZURE SPHERE ALTAIR 8800 EMULATOR VERSION: %s, DevX VERSION: "
		"%s.\r\n\r\n";

	if (!connection_initialised) {
		connection_initialised = true;
		int len = snprintf(msgBuffer, sizeof(msgBuffer), connected_message, ALTAIR_ON_AZURE_SPHERE_VERSION, AZURE_SPHERE_DEVX_VERSION);
		queue_mqtt_message(msgBuffer, (size_t)len);
		cpu_operating_mode = CPU_RUNNING;
		// if (dt_desiredCpuState.propertyValue) {
		//	cpu_operating_mode = CPU_RUNNING;
		//}
	} else {
		int len = snprintf(msgBuffer, sizeof(msgBuffer), reconnected_message, ALTAIR_ON_AZURE_SPHERE_VERSION, AZURE_SPHERE_DEVX_VERSION);
		queue_mqtt_message(msgBuffer, (size_t)len);
	}
}

/// <summary>
/// Load sample BASIC applications
/// </summary>
/// <param name="fileName"></param>
/// <returns></returns>
static bool load_application(const char* fileName) {
	char filePathAndName[50];
	snprintf(filePathAndName, sizeof(filePathAndName), "%s/%s", BASIC_SAMPLES_DIRECTORY, fileName);

	Log_Debug("LOADING '%s'\n", fileName);
	int GameFd = Storage_OpenFileInImagePackage(filePathAndName);
	if (GameFd >= 0) {
		// get length.
		off_t length = lseek(GameFd, 0, SEEK_END);
		ptrBasicApp = (uint8_t*)malloc((size_t)length + 9);
		memset((void*)ptrBasicApp, 0x00, (size_t)length + 9);
		lseek(GameFd, 0, SEEK_SET);
		read(GameFd, ptrBasicApp + 5, (size_t)length);
		close(GameFd);

		// wrap loaded app with 2 leading <CR><LF>, and 2 trailing <CR><LF>

		ptrBasicApp[0] = 0x0d;
		ptrBasicApp[1] = 0x0a;
		ptrBasicApp[2] = 0x0d;
		ptrBasicApp[3] = 0x0a;
		ptrBasicApp[length + 3] = 0x0d;
		ptrBasicApp[length + 4] = 0x0a;
		ptrBasicApp[length + 5] = 0x0d;
		ptrBasicApp[length + 6] = 0x0a;

		terminalInputMessageLen = (int)(length + 4);
		terminalOutputMessageLen = (int)(length + 4);

		appLoadPtr = 0;
		basicAppLength = (int)(length + 9); // add extra <CR><LF> * 2
		haveAppLoad = true;
	} else {
		return false;
	}
	return true;
}

/// <summary>
/// Set the temperature status led.
/// Red if HVAC needs to be turned on to get to desired temperature.
/// Blue to turn on cooler.
/// Green equals just right, no action required.
/// </summary>
void set_hvac_operating_mode(int temperature) {
	if (!dt_desiredTemperature.propertyUpdated || !onboard_telemetry.updated) {
		return;
	}

	onboard_telemetry.latest_operating_mode = temperature == *(int*)dt_desiredTemperature.propertyValue ? HVAC_MODE_GREEN
		: temperature > *(int*)dt_desiredTemperature.propertyValue ? HVAC_MODE_COOLING
		: HVAC_MODE_HEATING;

	if (onboard_telemetry.previous_operating_mode != onboard_telemetry.latest_operating_mode) {
		// minus one as first item is HVAC_MODE_UNKNOWN
		if (onboard_telemetry.previous_operating_mode != HVAC_MODE_UNKNOWN) {
			dx_gpioOff(ledRgb[onboard_telemetry.previous_operating_mode - 1]);
		}
		onboard_telemetry.previous_operating_mode = onboard_telemetry.latest_operating_mode;
		dx_deviceTwinReportValue(&dt_reportedTemperature, &onboard_telemetry.latest.temperature);
	}

	// minus one as first item is HVAC_MODE_UNKNOWN
	dx_gpioOn(ledRgb[onboard_telemetry.latest_operating_mode - 1]);
}

/// <summary>
/// Device Twin Handler to set the desired temperature value
/// </summary>
static void device_twin_set_temperature_handler(DX_DEVICE_TWIN_BINDING* deviceTwinBinding) {
	// validate data is sensible range before applying
	if (deviceTwinBinding->twinType == DX_DEVICE_TWIN_INT && IN_RANGE(*(int*)deviceTwinBinding->propertyValue, -20, 80)) {
		set_hvac_operating_mode(onboard_telemetry.latest.temperature);
		dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue, DX_DEVICE_TWIN_RESPONSE_COMPLETED);
	} else {
		dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue, DX_DEVICE_TWIN_RESPONSE_ERROR);
	}
}

/// <summary>
/// Read sensor and send to Azure IoT - called every 60 seconds
/// </summary>
static void measure_sensor_handler(EventLoopTimer* eventLoopTimer) {
	if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0) {
		dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
		return;
	}
	onboard_sensors_read(&onboard_telemetry.latest);
	onboard_telemetry.updated = true;
	set_hvac_operating_mode(onboard_telemetry.latest.temperature);
}

/// <summary>
/// Update device stats - only update device twins if they have changed
/// </summary>
static void device_stats_handler(EventLoopTimer* eventLoopTimer) {
	if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0) {
		dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
		return;
	}
	static int32_t previous_diskCacheHits = INT32_MAX;
	static int32_t previous_diskCacheMisses = INT32_MAX;
	static int32_t previous_diskTotalErrors = INT32_MAX;
	static int32_t previous_diskTotalWrites = INT32_MAX;

	if (previous_diskCacheHits != *(int*)dt_diskCacheHits.propertyValue) {
		previous_diskCacheHits = *(int*)dt_diskCacheHits.propertyValue;
		dx_deviceTwinReportValue(&dt_diskCacheHits, dt_diskCacheHits.propertyValue);
	}

	if (previous_diskCacheMisses != *(int*)dt_diskCacheMisses.propertyValue) {
		previous_diskCacheMisses = *(int*)dt_diskCacheMisses.propertyValue;
		dx_deviceTwinReportValue(&dt_diskCacheMisses, dt_diskCacheMisses.propertyValue);
	}

	if (previous_diskTotalErrors != *(int*)dt_diskTotalErrors.propertyValue) {
		previous_diskTotalErrors = *(int*)dt_diskTotalErrors.propertyValue;
		dx_deviceTwinReportValue(&dt_diskTotalErrors, dt_diskTotalErrors.propertyValue);
	}

	if (previous_diskTotalWrites != *(int*)dt_diskTotalWrites.propertyValue) {
		previous_diskTotalWrites = *(int*)dt_diskTotalWrites.propertyValue;
		dx_deviceTwinReportValue(&dt_diskTotalWrites, dt_diskTotalWrites.propertyValue);
	}
}

/// <summary>
/// Handle inbound MQTT messages
/// </summary>
/// <param name="topic_name"></param>
/// <param name="topic_name_size"></param>
/// <param name="message"></param>
/// <param name="message_size"></param>
static void handle_inbound_message(const char* topic_name, size_t topic_name_size, const char* message, size_t message_size) {
	char command[30];
	char* data;
	bool send_cr = false;
	size_t application_message_size;

	TOPIC_TYPE topic = topic_type((char*)topic_name, topic_name_size);
	data = (char*)message;
	application_message_size = message_size;

	switch (topic) {
	case TOPIC_DATA_SUB: // data message
		// upper case incoming message
		memset(command, 0, sizeof(command));

		if (application_message_size > 0 && data[application_message_size - 1] == '\r') { // is last char carriage return ?
			send_cr = true;
			application_message_size--;
		}

		for (int i = 0; i < sizeof(command) - 1 && i < application_message_size; i++) { // -1 to allow for trailing null
			command[i] = (char)toupper(data[i]);
		}

		// if command is load then try looking for in baked in samples otherwise pass on to Altair
		// emulator
		if (strncmp(command, "LOAD ", 5) == 0 && application_message_size > 5 && (command[application_message_size - 1] == '"')) {
			command[application_message_size - 1] = 0x00; // replace the '"' with \0
			if (load_application(&command[6])) {
				return;
			}
		}

		switch (cpu_operating_mode) {
		case CPU_RUNNING:
			if (application_message_size > 0) { // for example just cr was send so don't try send chars to CPU
				input_data = data;

				altairInputBufReadIndex = 0;
				altairOutputBufReadIndex = 0;
				terminalInputMessageLen = (int)application_message_size;
				terminalOutputMessageLen = (int)application_message_size;

				haveTerminalInputMessage = true;
				haveTerminalOutputMessage = true;

				pthread_mutex_lock(&wait_message_processed_mutex);
				pthread_cond_wait(&wait_message_processed_cond, &wait_message_processed_mutex);
				pthread_mutex_unlock(&wait_message_processed_mutex);
			}

			if (send_cr) {
				haveCtrlCharacter = 0x0d;
				haveCtrlPending = true;

				pthread_mutex_lock(&wait_message_processed_mutex);
				pthread_cond_wait(&wait_message_processed_cond, &wait_message_processed_mutex);
				pthread_mutex_unlock(&wait_message_processed_mutex);
			}
			break;
		case CPU_STOPPED:
			process_virtual_input(command, process_control_panel_commands);
			break;
		default:
			break;
		}
		break;
	case TOPIC_PASTE_SUB: // paste message
		break;
	case TOPIC_CONTROL_SUB: // control message
		if (data[0] >= 'A' && data[0] <= 'Z') {
			if (data[0] == 'M') { // CPU Monitor mode
				cpu_operating_mode = cpu_operating_mode == CPU_RUNNING ? CPU_STOPPED : CPU_RUNNING;
				if (cpu_operating_mode == CPU_STOPPED) {
					queue_mqtt_message("\r\nCPU MONITOR> ", 15);
					// publish_message("\r\nCPU MONITOR> ", 15, pub_topic_data);
				} else {
					queue_mqtt_message("\r\n", 2);
					// publish_message("\r\n", 2, pub_topic_data);
				}
			} else {
				haveCtrlPending = true;
				haveCtrlCharacter = data[0] & 31; // https://en.wikipedia.org/wiki/Control_character

				pthread_mutex_lock(&wait_message_processed_mutex);
				pthread_cond_wait(&wait_message_processed_cond, &wait_message_processed_mutex);
				pthread_mutex_unlock(&wait_message_processed_mutex);
			}
		}
		break;
	case TOPIC_VDISK_SUB: // vdisk response
		vdisk_mqtt_response_cb(data);
		break;
	default:
		break;
	}
}

/// <summary>
/// MQTT recieved message callback
/// </summary>
/// <param name="msg"></param>
static void publish_callback_wolf(MqttMessage* msg) {
	handle_inbound_message(msg->topic_name, msg->topic_name_len, msg->buffer, msg->buffer_len);
}

/// <summary>
/// Connection status led blink off oneshot timer callback
/// </summary>
/// <param name="eventLoopTimer"></param>
static void connection_status_led_off_handler(EventLoopTimer* eventLoopTimer) {
	if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0) {
		dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
		return;
	}
	dx_gpioOff(&azure_iot_connected_led);
}

/// <summary>
/// Blink LEDs timer handler
/// </summary>
static void connection_status_led_on_handler(EventLoopTimer* eventLoopTimer) {
	static int init_sequence = 25;

	if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0) {
		dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
		return;
	}

	if (init_sequence-- > 0) {

		dx_gpioOn(&azure_iot_connected_led);
		// on for 100ms off for 100ms = 200 ms in total
		dx_timerOneShotSet(&connectionStatusLedOnTimer, &(struct timespec){0, 200 * OneMS});
		dx_timerOneShotSet(&connectionStatusLedOffTimer, &(struct timespec){0, 100 * OneMS});

	} else if (dx_isAzureConnected() && is_mqtt_connected()) {

		dx_gpioOn(&azure_iot_connected_led);
		// on for 1400 off for 100ms = 1400 ms in total
		dx_timerOneShotSet(&connectionStatusLedOnTimer, &(struct timespec){1, 400 * OneMS});
        dx_timerOneShotSet(&connectionStatusLedOffTimer, &(struct timespec){1, 300 * OneMS});

	} else if (dx_isNetworkReady()) {

		dx_gpioOn(&azure_iot_connected_led);
		// on for 100ms off for 1300ms = 1400 ms in total
		dx_timerOneShotSet(&connectionStatusLedOnTimer, &(struct timespec){1, 400 * OneMS});
		dx_timerOneShotSet(&connectionStatusLedOffTimer, &(struct timespec){0, 700 * OneMS});

	} else {

		dx_gpioOn(&azure_iot_connected_led);
		// on for 700ms off for 700ms = 1400 ms in total
		dx_timerOneShotSet(&connectionStatusLedOnTimer, &(struct timespec){1, 400 * OneMS});
		dx_timerOneShotSet(&connectionStatusLedOffTimer, &(struct timespec){0, 100 * OneMS});
	}
}

/// <summary>
/// MQTT Dowork timer callback
/// </summary>
/// <param name="eventLoopTimer"></param>
static void mqtt_dowork_handler(EventLoopTimer* eventLoopTimer) {
	if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0) {
		dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
		return;
	}

	if (dirty_buffer) {
		send_messages = true;
	}

    dx_timerOneShotSet(&mqtt_do_work_timer, &(struct timespec){0, 300 * OneMS});
}

/// <summary>
/// Support for BASIC Port In for IOT.BAS temperature and pressure example
/// Example shows environment temperature and pressure example
/// </summary>
/// <param name="port"></param>
/// <returns></returns>
static uint8_t sphere_port_in(uint8_t port) {
	static bool reading_data = false;
	static char data[10];
	static int readPtr = 0;
	uint8_t retVal = 0;

	if (port == 43) {
		if (!reading_data) {
			readPtr = 0;
			snprintf(data, 10, "%d", onboard_telemetry.latest.temperature);
			publish_telemetry(onboard_telemetry.latest.temperature, onboard_telemetry.latest.pressure);
			reading_data = true;
		}

		retVal = data[readPtr++];
		if (retVal == 0x00) {
			reading_data = false;
		}
	}

	if (port == 44) {
		if (!reading_data) {
			readPtr = 0;
			snprintf(data, 10, "%d", onboard_telemetry.latest.pressure);
			reading_data = true;
		}

		retVal = data[readPtr++];
		if (retVal == 0x00) {
			reading_data = false;
		}
	}
	return retVal;
}

/// <summary>
/// BASIC Port Out for weather.bas
/// </summary>
/// <param name="port"></param>
/// <param name="data"></param>
static void sphere_port_out(uint8_t port, uint8_t data) {
	static float temperature = 0.0;
	struct location_info* locData;

	// get IP and Weather data.
	if (port == 32 && data == 1) {
		locData = GetLocationData();
		GetCurrentWeather(locData, &temperature);
	}

	// publish the telemetry to IoTC
	if (port == 32 && data == 2) {
		publish_telemetry((int)temperature, 1010);
	}
}

static char altair_read_terminal(void) {
	uint8_t rxBuffer[2] = { 0 };
	char retVal;

	if (haveCtrlPending) {
		haveCtrlPending = false;

		pthread_mutex_lock(&wait_message_processed_mutex);
		pthread_cond_signal(&wait_message_processed_cond);
		pthread_mutex_unlock(&wait_message_processed_mutex);

		return haveCtrlCharacter;
	}

	if (console_fd != -1) {
		ssize_t iRead = read(console_fd, rxBuffer, 1);
		if (iRead > 0) {
			// Log_Debug("Rx: 0x%02x (%c)\n", rxBuffer[0], rxBuffer[0] >= 0x20 ? rxBuffer[0] : '.');
			return (rxBuffer[0]);
		}
	}

	if (haveTerminalInputMessage) {
		// if (altairInputBufReadIndex > 0) {
		retVal = input_data[altairInputBufReadIndex++];
		//}

		if (altairInputBufReadIndex >= terminalInputMessageLen) {
			haveTerminalInputMessage = false;

			pthread_mutex_lock(&wait_message_processed_mutex);
			pthread_cond_signal(&wait_message_processed_cond);
			pthread_mutex_unlock(&wait_message_processed_mutex);
		}
		return retVal;

	} else if (haveAppLoad) {
		retVal = ptrBasicApp[appLoadPtr++];
		publish_character(retVal);

		if (appLoadPtr == basicAppLength) {
			haveAppLoad = false;
			free(ptrBasicApp);
			appLoadPtr = 0;
		}
		return retVal;
	}

	return 0;
}

static void altair_write_terminal(char c) {
	if (c > 0x80)
		c = (char)(c - 0x80);

	if (haveTerminalOutputMessage) {
		altairOutputBufReadIndex++;

		if (altairOutputBufReadIndex > terminalOutputMessageLen)
			haveTerminalOutputMessage = false;
	}

	if (!haveTerminalOutputMessage && !haveAppLoad) {
		publish_character(c);
	}
}

void process_control_panel_commands(void) {
	if (cpu_operating_mode == CPU_STOPPED) {
		switch (cmd_switches) {
		case RUN_CMD:
			cpu_operating_mode = CPU_RUNNING;
			break;
		case STOP_CMD:
			i8080_examine(&cpu, cpu.registers.pc);
			break;
		case SINGLE_STEP:
			i8080_cycle(&cpu);
			publish_cpu_state("Single step", cpu.address_bus, cpu.data_bus);
			break;
		case EXAMINE:
			i8080_examine(&cpu, bus_switches);
			publish_cpu_state("Examine", cpu.address_bus, cpu.data_bus);
			break;
		case EXAMINE_NEXT:
			i8080_examine_next(&cpu);
			publish_cpu_state("Examine next", cpu.address_bus, cpu.data_bus);
			break;
		case DEPOSIT:
			i8080_deposit(&cpu, (uint8_t)(bus_switches & 0xff));
			publish_cpu_state("Deposit", cpu.address_bus, cpu.data_bus);
			break;
		case DEPOSIT_NEXT:
			i8080_deposit_next(&cpu, (uint8_t)(bus_switches & 0xff));
			publish_cpu_state("Deposit next", cpu.address_bus, cpu.data_bus);
			break;
		default:
			break;
		}
	}

	if (cmd_switches & STOP_CMD) {
		cpu_operating_mode = CPU_STOPPED;
	}
	cmd_switches = 0x00;
}

static void update_panel_leds(uint8_t status, uint8_t data, uint16_t bus) {
	status = (uint8_t)(reverse_lut[(status & 0xf0) >> 4] | reverse_lut[status & 0xf] << 4);
	data = (uint8_t)(reverse_lut[(data & 0xf0) >> 4] | reverse_lut[data & 0xf] << 4);
	bus = (uint16_t)(reverse_lut[(bus & 0xf000) >> 12] << 8 | reverse_lut[(bus & 0x0f00) >> 8] << 12 | reverse_lut[(bus & 0xf0) >> 4] | reverse_lut[bus & 0xf] << 4);

	update_panel_status_leds(status, data, bus);
}

static void read_panel_input(void) {
	static GPIO_Value_Type buttonAState = GPIO_Value_High;

	if (dx_gpioStateGet(&buttonA, &buttonAState)) { // Button A shortcut to start CPM on the device
		cpu_operating_mode = CPU_STOPPED;
		process_control_panel_commands();
		bus_switches = 0xff00;
		cmd_switches = EXAMINE;
		process_control_panel_commands();
		cpu_operating_mode = CPU_RUNNING;
		process_control_panel_commands();
	}
	read_altair_panel_switches(process_control_panel_commands);
}

static void panel_refresh_handler(EventLoopTimer* eventLoopTimer) {
	if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0) {
		dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
		return;
	}
	update_panel_leds(cpu.cpuStatus, cpu.data_bus, cpu.address_bus);
	read_panel_input();
}

static inline uint8_t sense(void) {
	return (uint8_t)(bus_switches >> 8);
}

static bool loadRomImage(char* romImageName, uint16_t loadAddress) {
	int romFd = Storage_OpenFileInImagePackage(romImageName);
	if (romFd == -1)
		return false;

	off_t length = lseek(romFd, 0, SEEK_END);
	lseek(romFd, 0, SEEK_SET);
	read(romFd, &memory[loadAddress], (size_t)length);
	close(romFd);

	return true;
}

#ifndef BOOT_CPM
static void load8kRom(void) {
	const uint8_t rom[] = {
#include "Altair8800/8krom.h"
	};
	memcpy(memory, rom, sizeof(rom));
}
#endif

static void print_console_banner(void) {
	for (int x = 0; x < strlen(AltairMsg); x++) {
		altair_write_terminal(AltairMsg[x]);
	}
}

#pragma GCC push_options
#pragma GCC optimize("O0")
static void* altair_thread(void* arg) {
	Log_Debug("Altair Thread starting...\n");
	print_console_banner();

	memset(memory, 0x00, 64 * 1024); // clear memory.

	// initially no disk controller.
	disk_controller_t disk_controller;
	disk_controller.disk_function = disk_function;
	disk_controller.disk_select = disk_select;
	disk_controller.disk_status = disk_status;
	disk_controller.read = disk_read;
	disk_controller.write = disk_write;
	disk_controller.sector = sector;

#ifndef SD_CARD_ENABLED
	disk_drive.disk1.fp = Storage_OpenFileInImagePackage("Disks/cpm63k.dsk");
	if (disk_drive.disk1.fp == -1) {
		Log_Debug("Failed to load CPM Disk\n");
	}
#else
    disk_drive.disk1.fp = -1;
    disk_drive.disk1.diskPointer = 0;
    disk_drive.disk1.sector = 0;
    disk_drive.disk1.track = 0;
#endif

	// drive 2 is virtual (Python Server or MQTT Server) or microSD Card
	disk_drive.disk2.fp = -1;
	disk_drive.disk2.diskPointer = 0;
	disk_drive.disk2.sector = 0;
	disk_drive.disk2.track = 0;

	i8080_reset(&cpu, (port_in)altair_read_terminal, (port_out)altair_write_terminal, sense, &disk_controller, (azure_sphere_port_in)sphere_port_in,
		(azure_sphere_port_out)sphere_port_out);

	// load Disk Loader at 0xff00
	if (!loadRomImage("Disks/88dskrom.bin", 0xff00))
		Log_Debug("Failed to load Disk ROM image\n");

#ifdef BOOT_CPM
    i8080_examine(&cpu, 0xff00); // 0xff00 loads from disk, 0x0000 loads basic
#else
	load8kRom();                 // load 8k rom basic into memory at address 0x0000.
    i8080_examine(&cpu, 0x0000); // 0xff00 loads from disk, 0x0000 loads basic
#endif // BOOT_CPM

	while (1) {
		if (cpu_operating_mode == CPU_RUNNING) {
			i8080_cycle(&cpu);
		}

		if (send_messages) {
			if (dirty_buffer) {
				send_partial_message();
			}
			dirty_buffer = send_messages = false;
		}
	}

	return NULL;
}
#pragma GCC pop_options

/// <summary>
///  Initialize PeripheralGpios, device twins, direct methods, timers.
/// </summary>
/// <returns>0 on success, or -1 on failure</returns>
static void InitPeripheralAndHandlers(void) {
	dx_Log_Debug_Init(Log_Debug_Time_buffer, sizeof(Log_Debug_Time_buffer));
	curl_global_init(CURL_GLOBAL_DEFAULT);
	dx_gpioSetOpen(gpioSet, NELEMS(gpioSet));
	dx_gpioSetOpen(ledRgb, NELEMS(ledRgb));
    dx_i2cSetOpen(i2c_bindings, NELEMS(i2c_bindings));
	init_altair_hardware();

#ifndef ALTAIR_FRONT_PANEL_NONE
	// dx_startThreadDetached(panel_thread, NULL, "panel_thread");
#endif // !ALTAIR_FRONT_PANEL_NONE

	dx_azureConnect(&userConfig, NETWORK_INTERFACE, IOT_PLUG_AND_PLAY_MODEL_ID);
    dx_azureRegisterConnectionChangedNotification(azure_connection_changed);

	init_mqtt(publish_callback_wolf, mqtt_connected_cb);   

	dx_deviceTwinSubscribe(deviceTwinBindingSet, NELEMS(deviceTwinBindingSet));
	dx_timerSetStart(timerSet, NELEMS(timerSet));
	dx_directMethodSubscribe(directMethodBindingSet, NELEMS(directMethodBindingSet));

	onboard_sensors_init(i2c_onboard_sensors.fd);
	onboard_sensors_read(&onboard_telemetry.latest);
	onboard_telemetry.updated = true;

#ifdef SD_CARD_ENABLED

        dx_intercoreConnect(&intercore_sd_card_ctx);
        // set intercore read after publish timeout to 10000000 microseconds = 10 seconds
        dx_intercorePublishThenReadTimeout(&intercore_sd_card_ctx, 10000000);

#else

        dx_intercoreConnect(&intercore_disk_cache_ctx);
        // set intercore read after publish timeout to 1000 microseconds
        dx_intercorePublishThenReadTimeout(&intercore_disk_cache_ctx, 1000);

#endif // SD_CARD_ENABLED

	dx_timerOneShotSet(&mqtt_do_work_timer, &(struct timespec){1, 0});
	dx_timerOneShotSet(&connectionStatusLedOnTimer, &(struct timespec){1, 0});
	dx_startThreadDetached(altair_thread, NULL, "altair_thread");

	SetupWatchdog();
}

/// <summary>
///     Close PeripheralGpios and handlers.
/// </summary>
static void ClosePeripheralAndHandlers(void) {
	dx_azureToDeviceStop();
	dx_deviceTwinUnsubscribe();
	dx_directMethodUnsubscribe();
	dx_timerEventLoopStop();
	dx_gpioSetClose(gpioSet, NELEMS(gpioSet));
	dx_gpioSetClose(ledRgb, NELEMS(ledRgb));
	dx_i2cSetClose(i2c_bindings, NELEMS(i2c_bindings));
	onboard_sensors_close();
	curl_global_cleanup();
}

int main(int argc, char* argv[]) {
	dx_registerTerminationHandler();
	if (!dx_configParseCmdLineArguments(argc, argv, &userConfig)) {
		return dx_getTerminationExitCode();
	}
	InitPeripheralAndHandlers();

	// Main loop
	while (!dx_isTerminationRequired()) {
		int result = EventLoop_Run(dx_timerGetEventLoop(), -1, true);
		// Continue if interrupted by signal, e.g. due to breakpoint being set.
		if (result == -1 && errno != EINTR) {
			dx_terminate(DX_ExitCode_Main_EventLoopFail);
		}
	}
	ClosePeripheralAndHandlers();
	Log_Debug("\n\nApplication exiting. Last known exit code: %d\n", dx_getTerminationExitCode());
	return dx_getTerminationExitCode();
}