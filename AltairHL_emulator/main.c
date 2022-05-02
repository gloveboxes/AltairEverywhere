/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "main.h"

/// <summary>
/// Get geo location and environment for geo location from Open Weather Map
/// Requires Open Weather Map free api key
/// </summary>
static DX_TIMER_HANDLER(update_environment_handler)
{
	update_weather();

	if (azure_connected && environment.locationInfo.updated)
	{
		update_geo_properties(&environment);
		dx_timerOneShotSet(&tmr_update_environment, &(struct timespec){60, 0});
	}
	else
	{
		dx_timerOneShotSet(&tmr_update_environment, &(struct timespec){10, 0});
	}
}
DX_TIMER_HANDLER_END

/// <summary>
/// Reports memory usage to IoT Central
/// </summary>
static DX_TIMER_HANDLER(report_memory_usage)
{
	struct rusage r_usage;
	getrusage(RUSAGE_SELF, &r_usage);

	if (azure_connected &&
		dx_jsonSerialize(msgBuffer, sizeof(msgBuffer), 1, DX_JSON_INT, "memoryUsage", r_usage.ru_maxrss))
	{
		dx_azurePublish(msgBuffer, strlen(msgBuffer), diag_msg_properties, NELEMS(diag_msg_properties),
			&diag_content_properties);
	}
}
DX_TIMER_HANDLER_END

/// <summary>
/// Reports IoT Central Heatbeat UTC Date and Time
/// </summary>
static DX_TIMER_HANDLER(heart_beat_handler)
{
	if (azure_connected)
	{
		dx_deviceTwinReportValue(
			&dt_heartbeatUtc, dx_getCurrentUtc(msgBuffer, sizeof(msgBuffer))); // DX_TYPE_STRING
		dx_deviceTwinReportValue(&dt_filesystem_reads, dt_filesystem_reads.propertyValue);
		dx_deviceTwinReportValue(&dt_difference_disk_reads, dt_difference_disk_reads.propertyValue);
		dx_deviceTwinReportValue(&dt_difference_disk_writes, dt_difference_disk_writes.propertyValue);
		dx_deviceTwinReportValue(&dt_new_sessions, dt_new_sessions.propertyValue);
	}
}
DX_TIMER_HANDLER_END

// /// <summary>
// /// Updates the PI Sense HAT with Altair address bus, databus, and CPU state
// /// </summary>
// DX_TIMER_HANDLER(panel_refresh_handler)
// {
// 	uint8_t status = cpu.cpuStatus;
// 	uint8_t data   = cpu.data_bus;
// 	uint16_t bus   = cpu.address_bus;

// 	status = (uint8_t)(reverse_lut[(status & 0xf0) >> 4] | reverse_lut[status & 0xf] << 4);
// 	data   = (uint8_t)(reverse_lut[(data & 0xf0) >> 4] | reverse_lut[data & 0xf] << 4);
// 	bus    = (uint16_t)(reverse_lut[(bus & 0xf000) >> 12] << 8 | reverse_lut[(bus & 0x0f00) >> 8] << 12 |
//                      reverse_lut[(bus & 0xf0) >> 4] | reverse_lut[bus & 0xf] << 4);

// 	update_panel_status_leds(status, data, bus);

// 	dx_timerOneShotSet(&tmr_panel_refresh, &(struct timespec){0, 50 * ONE_MS});
// }
// DX_TIMER_HANDLER_END

/// <summary>
/// Handler called to process inbound message
/// </summary>
DX_TIMER_HANDLER(deferred_input_handler)
{
	char command[30];
	bool send_cr = false;
	int retry    = 0;

	size_t application_message_size = ws_input_block.length;
	char *data                      = ws_input_block.buffer;

	memset(command, 0x00, sizeof(command));

	// is last char carriage return then strip off and set flag to add line feed at then end
	if (application_message_size > 0 && data[application_message_size - 1] == '\r')
	{
		send_cr = true;
		application_message_size--;
	}

	if (application_message_size > 0)
	{
		// ctrl-m is mapped to ascii 28 to get around ctrl-m being /r
		if (data[0] == 28)
		{
			cpu_operating_mode = cpu_operating_mode == CPU_RUNNING ? CPU_STOPPED : CPU_RUNNING;
			if (cpu_operating_mode == CPU_STOPPED)
			{
				bus_switches = cpu.address_bus;
				publish_message("\r\nCPU MONITOR> ", 15);
			}
			else
			{
				haveCtrlCharacter = 0x0d;
				spin_wait(&haveCtrlPending);
			}
			goto cleanup;
		}

		// test for ctlr characters
		if (data[0] > 0 && data[0] < 27)
		{
			haveCtrlCharacter = data[0];
			spin_wait(&haveCtrlPending);

			if (cpu_operating_mode == CPU_RUNNING)
			{
				goto cleanup;
			}
		}

		// upper case the first 30 chars for command processing
		for (int i = 0; i < sizeof(command) - 1 && i < application_message_size; i++)
		{ // -1 to allow for trailing null
			command[i] = (char)toupper(data[i]);
		}

		// if command is loadx then try looking for in baked in samples
		if (strncmp(command, "LOADX ", 6) == 0 && application_message_size > 6 &&
			(command[application_message_size - 1] == '"'))
		{
			command[application_message_size - 1] = 0x00; // replace the '"' with \0
			load_application(&command[7]);
			goto cleanup;
		}
	}

	switch (cpu_operating_mode)
	{
		case CPU_RUNNING:

			if (application_message_size > 0)
			{ // for example just cr was send so don't try send chars to CPU
				input_data = data;

				altairInputBufReadIndex  = 0;
				altairOutputBufReadIndex = 0;
				terminalInputMessageLen  = (int)application_message_size;
				terminalOutputMessageLen = (int)application_message_size;

				haveTerminalOutputMessage = true;
				spin_wait(&haveTerminalInputMessage);
			}

			if (send_cr)
			{
				haveCtrlCharacter = 0x0d;
				spin_wait(&haveCtrlPending);
			}
			break;
		case CPU_STOPPED:
			process_virtual_input(command, process_control_panel_commands);
			break;
		default:
			break;
	}

cleanup:
	ws_input_block.active = false;
}
DX_TIMER_HANDLER_END

/// <summary>
/// Sets wait for terminal io cmd to be processed and flag reset
/// </summary>
static void spin_wait(volatile bool *flag)
{
	struct timespec delay = {0, 10 * ONE_MS};
	int retry             = 0;
	*flag                 = true;

	// wait max 200ms = 10 x 20ms
	while (*flag && retry++ < 10)
	{
		while (nanosleep(&delay, &delay))
			;
	}
}

/// <summary>
/// Client connected successfully
/// </summary>
static void client_connected_cb(void)
{
	print_console_banner();
	cpu_operating_mode = CPU_RUNNING;
}

/// <summary>
/// Load sample BASIC applications
/// </summary>
/// <param name="fileName"></param>
/// <returns></returns>
static bool load_application(const char *fileName)
{
	int retry = 0;
	char filePathAndName[50];
	snprintf(filePathAndName, sizeof(filePathAndName), "%s/%s", BASIC_SAMPLES_DIRECTORY, fileName);

	// precaution
	if (app_stream != NULL)
	{
		fclose(app_stream);
	}

	if ((app_stream = fopen(filePathAndName, "r")) != NULL)
	{
		haveTerminalInputMessage = false;
		haveAppLoad              = true;

		retry = 0;
		while (haveAppLoad && retry++ < 20)
		{
			nanosleep(&(struct timespec){0, 10 * ONE_MS}, NULL);
		}
		return true;
	}

	publish_message("\n\rFile not found\n\r", 18);
	return false;
}

static char terminal_read(void)
{
	uint8_t rxBuffer[2] = {0};
	char retVal;
	int ch;

	if (haveCtrlPending)
	{
		haveCtrlPending = false;
		return haveCtrlCharacter;
	}

	if (haveTerminalInputMessage)
	{
		retVal = input_data[altairInputBufReadIndex++];

		if (altairInputBufReadIndex >= terminalInputMessageLen)
		{
			haveTerminalInputMessage = false;
		}
		return retVal;
	}
	else if (haveAppLoad)
	{
		if ((ch = fgetc(app_stream)) == EOF)
		{
			retVal = 0x00;
			fclose(app_stream);
			app_stream  = NULL;
			haveAppLoad = false;
		}
		else
		{
			retVal = (uint8_t)ch;
			if (retVal == '\n')
			{
				retVal = 0x0D;
			}
		}
		return retVal;
	}
	return 0;
}

static void terminal_write(uint8_t c)
{
	c &= 0x7F;

	if (haveTerminalOutputMessage)
	{
		altairOutputBufReadIndex++;

		if (altairOutputBufReadIndex > terminalOutputMessageLen)
			haveTerminalOutputMessage = false;
	}

	if (!haveTerminalOutputMessage)
	{
		publish_character(c);
	}
}

static inline uint8_t sense(void)
{
	return (uint8_t)(bus_switches >> 8);
}

void print_console_banner(void)
{
	for (int x = 0; x < strlen(AltairMsg); x++)
	{
		terminal_write(AltairMsg[x]);
	}

	for (int x = 0; x < strlen(ALTAIR_EMULATOR_VERSION); x++)
	{
		terminal_write(ALTAIR_EMULATOR_VERSION[x]);
	}

	for (int x = 0; x < strlen("\r\n"); x++)
	{
		terminal_write("\r\n"[x]);
	}
}

/// <summary>
/// Updates the PI Sense HAT with Altair address bus, databus, and CPU state
/// </summary>
static void *panel_refresh_thread(void *arg)
{
	uint8_t last_status = 0;
	uint8_t last_data   = 0;
	uint16_t last_bus   = 0;

	while (true)
	{
		uint8_t status = cpu.cpuStatus;
		uint8_t data   = cpu.data_bus;
		uint16_t bus   = cpu.address_bus;

		if (status != last_status || data != last_data || bus != last_bus)
		{
			last_status = status;
			last_data   = data;
			last_bus    = bus;

			status = (uint8_t)(reverse_lut[(status & 0xf0) >> 4] | reverse_lut[status & 0xf] << 4);
			data   = (uint8_t)(reverse_lut[(data & 0xf0) >> 4] | reverse_lut[data & 0xf] << 4);
			bus = (uint16_t)(reverse_lut[(bus & 0xf000) >> 12] << 8 | reverse_lut[(bus & 0x0f00) >> 8] << 12 |
							 reverse_lut[(bus & 0xf0) >> 4] | reverse_lut[bus & 0xf] << 4);

			update_panel_status_leds(status, data, bus);
			nanosleep(&(struct timespec){0, 50 * ONE_MS}, NULL);
		}
		else
		{
			nanosleep(&(struct timespec){0, 10 * ONE_MS}, NULL);
		}
	}
	return NULL;
}

/// <summary>
/// Main Altair CPU execution thread
/// </summary>
static void *altair_thread(void *arg)
{
	Log_Debug("Altair Thread starting...\n");
	print_console_banner();

	memset(memory, 0x00, 64 * 1024); // clear Altair memory.

	disk_controller_t disk_controller;
	disk_controller.disk_function = disk_function;
	disk_controller.disk_select   = disk_select;
	disk_controller.disk_status   = disk_status;
	disk_controller.read          = disk_read;
	disk_controller.write         = disk_write;
	disk_controller.sector        = sector;

#ifdef ALTAIR_CLOUD
	if ((disk_drive.disk1.fp = open(DISK_A, O_RDONLY)) == -1)
	{
		Log_Debug("Failed to open %s disk image\n", DISK_A);
		exit(-1);
	}
#else
	if ((disk_drive.disk1.fp = open(DISK_A, O_RDWR)) == -1)
	{
		Log_Debug("Failed to open %s disk image\n", DISK_A);
		exit(-1);
	}
#endif // ALTAIR_CLOUD

	disk_drive.disk1.diskPointer = 0;
	disk_drive.disk1.sector      = 0;
	disk_drive.disk1.track       = 0;

#ifdef ALTAIR_CLOUD
	if ((disk_drive.disk2.fp = open(DISK_B, O_RDONLY)) == -1)
	{
		Log_Debug("Failed to open %s disk image\n", DISK_A);
		exit(-1);
	}
#else
	if ((disk_drive.disk2.fp = open(DISK_B, O_RDWR)) == -1)
	{
		Log_Debug("Failed to open %s disk image\n", DISK_B);
		exit(-1);
	}
#endif // ALTAIR_CLOUD
	disk_drive.disk2.diskPointer = 0;
	disk_drive.disk2.sector      = 0;
	disk_drive.disk2.track       = 0;

	i8080_reset(&cpu, (port_in)terminal_read, (port_out)terminal_write, sense, &disk_controller,
		(azure_sphere_port_in)io_port_in, (azure_sphere_port_out)io_port_out);

	// load Disk Loader at 0xff00
	if (!loadRomImage(DISK_LOADER, 0xff00))
	{
		Log_Debug("Failed to open %s disk load ROM image\n", DISK_LOADER);
	}

	i8080_examine(&cpu, 0xff00); // 0xff00 loads from disk, 0x0000 loads basic

	while (1)
	{
		if (cpu_operating_mode == CPU_RUNNING)
		{
			i8080_cycle(&cpu);
		}

		if (send_partial_msg)
		{
			send_partial_message();
			send_partial_msg = false;
		}
	}

	return NULL;
}

/// <summary>
/// Report on first connect the software version and device startup UTC time
/// </summary>
/// <param name="connected"></param>
static void report_software_version(bool connected)
{
	if (connected)
	{
		snprintf(msgBuffer, sizeof(msgBuffer), "Altair emulator: %s, DevX: %s", ALTAIR_EMULATOR_VERSION,
			AZURE_SPHERE_DEVX_VERSION);
		dx_deviceTwinReportValue(&dt_softwareVersion, msgBuffer);
		dx_deviceTwinReportValue(
			&dt_deviceStartTimeUtc, dx_getCurrentUtc(msgBuffer, sizeof(msgBuffer))); // DX_TYPE_STRING

		dx_azureUnregisterConnectionChangedNotification(report_software_version);
	}
}

static void azure_connection_state(bool connection_state)
{
	azure_connected = connection_state;
}

/// <summary>
///  Initialize PeripheralGpios, device twins, direct methods, timers.
/// </summary>
/// <returns>0 on success, or -1 on failure</returns>
static void InitPeripheralAndHandlers(int argc, char *argv[])
{
	// https://stackoverflow.com/questions/18935446/program-received-signal-sigpipe-broken-pipe
	sigaction(SIGPIPE, &(struct sigaction){SIG_IGN}, NULL);

	curl_global_init(CURL_GLOBAL_ALL);

	setvbuf(stdout, NULL, _IONBF, 0); // disable stdout/printf buffering

	dx_Log_Debug_Init(Log_Debug_Time_buffer, sizeof(Log_Debug_Time_buffer));
	srand((unsigned int)time(NULL)); // seed the random number generator

	parse_altair_cmd_line_arguments(argc, argv, &altair_config);

	init_environment(&altair_config);

	dx_Log_Debug("Network interface %s %s\n", altair_config.user_config.network_interface,
		dx_isNetworkConnected(altair_config.user_config.network_interface) ? "connected" : "NOT connected");

	init_altair_hardware();

	// if there is no ID Scope or connection string then don't attempt to start connection to Azure IoT
	// Central
	if (!dx_isStringNullOrEmpty(altair_config.user_config.idScope) ||
		!dx_isStringNullOrEmpty(altair_config.user_config.connection_string))
	{
		dx_azureConnect(&altair_config.user_config, altair_config.user_config.network_interface,
			IOT_PLUG_AND_PLAY_MODEL_ID);

		dx_azureRegisterConnectionChangedNotification(azure_connection_state);
		dx_azureRegisterConnectionChangedNotification(report_software_version);
	}

	dx_deviceTwinSubscribe(device_twin_bindings, NELEMS(device_twin_bindings));

	init_web_socket_server(client_connected_cb);

	dx_timerSetStart(timer_bindings, NELEMS(timer_bindings));

	dx_startThreadDetached(altair_thread, NULL, "altair_thread");

#ifdef ALTAIR_FRONT_PANEL_PI_SENSE
	dx_startThreadDetached(panel_refresh_thread, NULL, "panel_refresh_thread");
#endif
}

/// <summary>
///     Close PeripheralGpios and handlers.
/// </summary>
static void ClosePeripheralAndHandlers(void)
{
	dx_azureToDeviceStop();
	dx_deviceTwinUnsubscribe();
	dx_timerEventLoopStop();

	curl_global_cleanup();
}

int main(int argc, char *argv[])
{
	dx_registerTerminationHandler();
	InitPeripheralAndHandlers(argc, argv);

	dx_eventLoopRun();

	ClosePeripheralAndHandlers();

	dx_Log_Debug("APPLICATION EXITED WITH EXIT CODE: %d\n\n", dx_getTerminationExitCode());
	return dx_getTerminationExitCode();
}
