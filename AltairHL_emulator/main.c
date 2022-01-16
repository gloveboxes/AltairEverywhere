/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "main.h"

// End of variable declarations

static void mqtt_connected_cb(void)
{
    static bool connection_initialised = false;
    static const char *connected_message = "\r\nCONNECTED TO AZURE SPHERE ALTAIR 8800 EMULATOR VERSION: %s, DevX VERSION: %s.\r\n\r\n";
    static const char *reconnected_message = "\r\nRECONNECTED TO AZURE SPHERE ALTAIR 8800 EMULATOR VERSION: %s, DevX VERSION: %s.\r\n\r\n";

    if (!connection_initialised) {
        connection_initialised = true;
        int len = snprintf(msgBuffer, sizeof(msgBuffer), connected_message, ALTAIR_ON_AZURE_SPHERE_VERSION, AZURE_SPHERE_DEVX_VERSION);
        queue_mqtt_message((const uint8_t *)msgBuffer, (size_t)len);
        cpu_operating_mode = CPU_RUNNING;
        // if (dt_cpuState.propertyValue) {
        //	cpu_operating_mode = CPU_RUNNING;
        //}
    } else {
        int len = snprintf(msgBuffer, sizeof(msgBuffer), reconnected_message, ALTAIR_ON_AZURE_SPHERE_VERSION, AZURE_SPHERE_DEVX_VERSION);
        queue_mqtt_message((const uint8_t *)msgBuffer, (size_t)len);
    }
}

/// <summary>
/// Load sample BASIC applications
/// </summary>
/// <param name="fileName"></param>
/// <returns></returns>
static bool load_application(const char *fileName)
{
    char filePathAndName[50];
    snprintf(filePathAndName, sizeof(filePathAndName), "%s/%s", BASIC_SAMPLES_DIRECTORY, fileName);

    Log_Debug("LOADING '%s'\n", fileName);

    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    FILE *stream = fopen(filePathAndName, "r");
    if (stream == NULL) {
        return false;
    }

    while ((nread = getline(&line, &len, stream)) != -1) {
        printf("Retrieved line of length %zd:\n", nread);
        fwrite(line, nread, 1, stdout);

        if (line[nread - 1] == '\n') {
            nread--;
        }

        if (nread > 0) {

            terminalInputMessageLen = nread;
            terminalOutputMessageLen = nread;

            appLoadPtr = 0;
            basicAppLength = nread;

            ptrBasicApp = (uint8_t *)line;
            haveAppLoad = true;

            pthread_mutex_lock(&wait_message_processed_mutex);
            pthread_cond_wait(&wait_message_processed_cond, &wait_message_processed_mutex);
            pthread_mutex_unlock(&wait_message_processed_mutex);
        }

        haveCtrlCharacter = 0x0d;
        haveCtrlPending = true;
    }

    free(line);
    fclose(stream);

    return true;
}

/// <summary>
/// Handle inbound MQTT messages
/// </summary>
/// <param name="topic_name"></param>
/// <param name="topic_name_size"></param>
/// <param name="message"></param>
/// <param name="message_size"></param>
static void handle_inbound_message(const char *topic_name, size_t topic_name_size, const char *message, size_t message_size)
{
    char command[30];
    char *data;
    bool send_cr = false;
    size_t application_message_size;

    TOPIC_TYPE topic = topic_type((char *)topic_name, topic_name_size);
    data = (char *)message;
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

                // pthread_mutex_lock(&wait_message_processed_mutex);
                // pthread_cond_wait(&wait_message_processed_cond, &wait_message_processed_mutex);
                // pthread_mutex_unlock(&wait_message_processed_mutex);
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
                    queue_mqtt_message((const uint8_t *)"\r\nCPU MONITOR> ", 15);
                    // publish_message("\r\nCPU MONITOR> ", 15, pub_topic_data);
                } else {
                    queue_mqtt_message((const uint8_t *)"\r\n", 2);
                    // publish_message("\r\n", 2, pub_topic_data);
                }
            } else {

                haveCtrlCharacter = data[0] & 31; // https://en.wikipedia.org/wiki/Control_character
                haveCtrlPending = true;
                // pthread_mutex_lock(&wait_message_processed_mutex);
                // pthread_cond_wait(&wait_message_processed_cond, &wait_message_processed_mutex);
                // pthread_mutex_unlock(&wait_message_processed_mutex);
            }
        }
        break;
    case TOPIC_VDISK_SUB: // vdisk response
        // vdisk_mqtt_response_cb(data);
        break;
    default:
        break;
    }
}

/// <summary>
/// MQTT recieved message callback
/// </summary>
/// <param name="msg"></param>
static void publish_callback_wolf(MqttMessage *msg)
{
    handle_inbound_message(msg->topic_name, msg->topic_name_len, (const char *)msg->buffer, msg->buffer_len);
}

/// <summary>
/// MQTT Dowork timer callback
/// </summary>
/// <param name="eventLoopTimer"></param>
static DX_TIMER_HANDLER(mqtt_dowork_handler)
{
    if (dirty_buffer) {
        send_messages = true;
    }
}
DX_TIMER_HANDLER_END

/// <summary>
/// Support for BASIC Port In for IOT.BAS temperature and pressure example
/// Example shows environment temperature and pressure example
/// </summary>
/// <param name="port"></param>
/// <returns></returns>
static uint8_t sphere_port_in(uint8_t port)
{
    static bool reading_data = false;
    static char data[10];
    static int readPtr = 0;
    uint8_t retVal = 0;

    if (port == 43) {
        if (!reading_data) {
            readPtr = 0;
            snprintf(data, 10, "%d", 25);
            publish_telemetry(25, 1050);
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
            snprintf(data, 10, "%d", 25);
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
static void sphere_port_out(uint8_t port, uint8_t data)
{
    static float temperature = 0.0;
    struct location_info *locData;

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

static char altair_read_terminal(void)
{
    uint8_t rxBuffer[2] = {0};
    char retVal;

    if (haveCtrlPending) {
        haveCtrlPending = false;

        // pthread_mutex_lock(&wait_message_processed_mutex);
        // pthread_cond_signal(&wait_message_processed_cond);
        // pthread_mutex_unlock(&wait_message_processed_mutex);

        return haveCtrlCharacter;
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

        if (appLoadPtr > basicAppLength) {
            haveAppLoad = false;
            haveTerminalInputMessage = false;
            appLoadPtr = 0;
            retVal = 0;

            pthread_mutex_lock(&wait_message_processed_mutex);
            pthread_cond_signal(&wait_message_processed_cond);
            pthread_mutex_unlock(&wait_message_processed_mutex);
        }
        return retVal;
    }

    return 0;
}

static void altair_write_terminal(uint8_t c)
{
    c &= 0x7F;

    if (haveTerminalOutputMessage) {
        altairOutputBufReadIndex++;

        if (altairOutputBufReadIndex > terminalOutputMessageLen)
            haveTerminalOutputMessage = false;
    }

    if (!haveTerminalOutputMessage && !haveAppLoad) {
        publish_character(c);
    }
}

void process_control_panel_commands(void)
{
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

static void update_panel_leds(uint8_t status, uint8_t data, uint16_t bus)
{
    status = (uint8_t)(reverse_lut[(status & 0xf0) >> 4] | reverse_lut[status & 0xf] << 4);
    data = (uint8_t)(reverse_lut[(data & 0xf0) >> 4] | reverse_lut[data & 0xf] << 4);
    bus = (uint16_t)(reverse_lut[(bus & 0xf000) >> 12] << 8 | reverse_lut[(bus & 0x0f00) >> 8] << 12 | reverse_lut[(bus & 0xf0) >> 4] | reverse_lut[bus & 0xf] << 4);

    update_panel_status_leds(status, data, bus);
}

static DX_TIMER_HANDLER(panel_refresh_handler)
{
    update_panel_leds(cpu.cpuStatus, cpu.data_bus, cpu.address_bus);
    dx_timerOneShotSet(&tmr_panel_refresh, &(struct timespec){0, 10 * ONE_MS});
}
DX_TIMER_HANDLER_END

static inline uint8_t sense(void)
{
    return (uint8_t)(bus_switches >> 8);
}

static bool loadRomImage(char *romImageName, uint16_t loadAddress)
{
    // TODO OPEN FILE
    // int romFd = Storage_OpenFileInImagePackage(romImageName);
    int romFd = -1;
    romFd = open(romImageName, O_RDWR);
    if (romFd == -1)
        return false;

    off_t length = lseek(romFd, 0, SEEK_END);
    lseek(romFd, 0, SEEK_SET);
    read(romFd, &memory[loadAddress], (size_t)length);
    close(romFd);

    return true;
}

#ifndef BOOT_CPM
static void load8kRom(void)
{
    const uint8_t rom[] = {
#include "Altair8800/8krom.h"
    };
    memcpy(memory, rom, sizeof(rom));
}
#endif

static void print_console_banner(void)
{
    for (int x = 0; x < strlen(AltairMsg); x++) {
        altair_write_terminal(AltairMsg[x]);
    }
}

#pragma GCC push_options
#pragma GCC optimize("O0")
static void *altair_thread(void *arg)
{
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

    if ((disk_drive.disk1.fp = open("Disks/cpm63k.dsk", O_RDWR)) == -1) {
        Log_Debug("Failed to open CPM disk image\\n");
        exit(-1);
    }

    disk_drive.disk1.diskPointer = 0;
    disk_drive.disk1.sector = 0;
    disk_drive.disk1.track = 0;

    if ((disk_drive.disk2.fp = open("Disks/blank.dsk", O_RDWR)) == -1) {
        Log_Debug("Failed to open blank disk image\n");
        exit(-1);
    }
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
/// Report on first connect the software version and device startup UTC time
/// </summary>
/// <param name="connected"></param>
static void azure_connection_changed(bool connected)
{
    if (connected) {
        snprintf(msgBuffer, sizeof(msgBuffer), "Altair emulator version: %s, DevX version: %s", ALTAIR_ON_AZURE_SPHERE_VERSION, AZURE_SPHERE_DEVX_VERSION);
        dx_deviceTwinReportValue(&dt_softwareVersion, msgBuffer);
        dx_deviceTwinReportValue(&dt_deviceStartTime, dx_getCurrentUtc(msgBuffer, sizeof(msgBuffer))); // DX_TYPE_STRING

        dx_azureUnregisterConnectionChangedNotification(azure_connection_changed);
    }
}

/// <summary>
///  Initialize PeripheralGpios, device twins, direct methods, timers.
/// </summary>
/// <returns>0 on success, or -1 on failure</returns>
static void InitPeripheralAndHandlers(int argc, char *argv[])
{
    dx_Log_Debug_Init(Log_Debug_Time_buffer, sizeof(Log_Debug_Time_buffer));

    init_altair_hardware();

#ifndef ALTAIR_FRONT_PANEL_NONE
    // dx_startThreadDetached(panel_thread, NULL, "panel_thread");
#endif // !ALTAIR_FRONT_PANEL_NONE

    dx_azureConnect(&userConfig, NETWORK_INTERFACE, IOT_PLUG_AND_PLAY_MODEL_ID);
    dx_azureRegisterConnectionChangedNotification(azure_connection_changed);

    init_mqtt(argc, argv, publish_callback_wolf, mqtt_connected_cb);

    dx_deviceTwinSubscribe(deviceTwinBindingSet, NELEMS(deviceTwinBindingSet));
    dx_timerSetStart(timerSet, NELEMS(timerSet));

    dx_startThreadDetached(altair_thread, NULL, "altair_thread");
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
    if (!dx_configParseCmdLineArguments(argc, argv, &userConfig)) {
        return dx_getTerminationExitCode();
    }
    InitPeripheralAndHandlers(argc, argv);

    dx_eventLoopRun();

    ClosePeripheralAndHandlers();
    Log_Debug("\n\nApplication exiting. Last known exit code: %d\n", dx_getTerminationExitCode());
    return dx_getTerminationExitCode();
}