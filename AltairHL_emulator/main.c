/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#define _POSIX_C_SOURCE 200809L

#include "main.h"
#include "app_exit_codes.h"
#include <signal.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <stdatomic.h>

// Thread synchronization
static pthread_mutex_t cpu_state_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t terminal_state_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t terminal_input_cond = PTHREAD_COND_INITIALIZER;

// Atomic CPU state for high-performance read access
static _Atomic(CPU_OPERATING_MODE) atomic_cpu_operating_mode = CPU_STOPPED;

// Constants to replace magic numbers
#define TERMINAL_COMMAND_BUFFER_SIZE 30
#define FILE_PATH_BUFFER_SIZE 64
#define MAX_FILENAME_LENGTH 40
#define TERMINAL_WAIT_TIMEOUT_MS 200
#define MAX_RETRY_COUNT 20
#define CTRL_M_MAPPED_VALUE 28
#define ASCII_CTRL_MAX 29
#define ASCII_MASK_7BIT 0x7F
#define MEMORY_SIZE_64K (64 * 1024)
#define ROM_LOADER_ADDRESS 0xFF00

// Forward declarations
static void wait_for_terminal_completion(bool *flag);
static void signal_terminal_completion(bool *flag);

/// <summary>
/// High-performance CPU operating mode getter for emulation loop
/// Uses atomic load with relaxed ordering for maximum performance
/// </summary>
static inline CPU_OPERATING_MODE get_cpu_operating_mode_fast(void)
{
    return atomic_load_explicit(&atomic_cpu_operating_mode, memory_order_relaxed);
}

/// <summary>
/// Thread-safe CPU operating mode getter for non-critical paths
/// </summary>
CPU_OPERATING_MODE get_cpu_operating_mode(void)
{
    pthread_mutex_lock(&cpu_state_mutex);
    CPU_OPERATING_MODE mode = cpu_operating_mode;
    pthread_mutex_unlock(&cpu_state_mutex);
    return mode;
}

/// <summary>
/// Thread-safe CPU operating mode setter
/// Updates both the legacy variable and atomic version
/// </summary>
void set_cpu_operating_mode(CPU_OPERATING_MODE new_mode)
{
    pthread_mutex_lock(&cpu_state_mutex);
    cpu_operating_mode = new_mode;
    atomic_store_explicit(&atomic_cpu_operating_mode, new_mode, memory_order_release);
    pthread_mutex_unlock(&cpu_state_mutex);
}

/// <summary>
/// Get geo location and environment for geo location from Open Weather Map
/// Requires Open Weather Map free api key
/// </summary>
static DX_TIMER_HANDLER(update_environment_handler)
{
    if (dx_isNetworkConnected(network_interface))
    {
        update_weather();
    }

    dx_timerOneShotSet(&tmr_update_environment, &(struct timespec){30 * 60, 0});
}
DX_TIMER_HANDLER_END

/// <summary>
/// Reports memory usage to IoT Central
/// </summary>
static DX_TIMER_HANDLER(report_memory_usage)
{
    struct rusage r_usage;
    getrusage(RUSAGE_SELF, &r_usage);

    if (azure_connected && dx_jsonSerialize(msgBuffer, sizeof(msgBuffer), 1, DX_JSON_INT, "memoryUsage", r_usage.ru_maxrss))
    {
        dx_azurePublish(msgBuffer, strlen(msgBuffer), diag_msg_properties, NELEMS(diag_msg_properties), &diag_content_properties);
        update_geo_location(&environment); // Hitch a ride on the report_memory_usage event. Only publishes once.
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
        dx_deviceTwinReportValue(&dt_heartbeatUtc, dx_getCurrentUtc(msgBuffer, sizeof(msgBuffer))); // DX_TYPE_STRING
        dx_deviceTwinReportValue(&dt_new_sessions, dt_new_sessions.propertyValue);
    }
}
DX_TIMER_HANDLER_END

/// <summary>
/// Handle enter key press in terminal
/// </summary>
/// <param name="data">Input data buffer</param>
/// <returns>true if handled and should cleanup, false to continue processing</returns>
static bool handle_enter_key(char *data)
{
    switch (get_cpu_operating_mode())
    {
        case CPU_RUNNING:
            send_terminal_character(0x0d, false);
            break;
        case CPU_STOPPED:
            data[0] = 0x00;
            process_virtual_input(data);
            break;
        default:
            break;
    }
    return true;
}

/// <summary>
/// Handle control character input
/// </summary>
/// <param name="data">Input character</param>
/// <param name="application_message_size">Size of the message</param>
/// <returns>true if handled and should cleanup, false to continue processing</returns>
static bool handle_ctrl_character(char *data, size_t application_message_size)
{
    if (application_message_size == 1 && data[0] > 0 && data[0] < ASCII_CTRL_MAX)
    {
        // ctrl-m is mapped to ascii 28 to get around ctrl-m being /r
        if (data[0] == CTRL_M_MAPPED_VALUE)
        {
            CPU_OPERATING_MODE current_mode = get_cpu_operating_mode();
            CPU_OPERATING_MODE new_mode = current_mode == CPU_RUNNING ? CPU_STOPPED : CPU_RUNNING;
            set_cpu_operating_mode(new_mode);
            if (new_mode == CPU_STOPPED)
            {
                bus_switches = cpu.address_bus;
                publish_message("\r\nCPU MONITOR> ", 15);
            }
            else
            {
                send_terminal_character(0x0d, false);
            }
        }
        else // pass through the ctrl character
        {
            send_terminal_character(data[0], false);
        }
        return true;
    }
    return false;
}

/// <summary>
/// Handle single character input
/// </summary>
/// <param name="data">Input character</param>
/// <param name="application_message_size">Size of the message</param>
/// <returns>true if handled and should cleanup, false to continue processing</returns>
static bool handle_single_character(char *data, size_t application_message_size)
{
    if (application_message_size == 1)
    {
        if (get_cpu_operating_mode() == CPU_RUNNING)
        {
            altairOutputBufReadIndex  = 0;
            terminalOutputMessageLen  = 0;
            terminalInputCharacter    = data[0];
            haveTerminalOutputMessage = true;
            // send_terminal_character(data[0], false);
        }
        else
        {
            data[0] = (char)toupper(data[0]);
            data[1] = 0x00;
            process_virtual_input(data);
        }
        return true;
    }
    return false;
}

/// <summary>
/// Handle LOADX command processing
/// </summary>
/// <param name="command">Command buffer</param>
/// <param name="application_message_size">Size of the message</param>
/// <returns>true if handled and should cleanup, false to continue processing</returns>
static bool handle_loadx_command(char *command, size_t application_message_size)
{
    if (strncmp(command, "LOADX ", 6) == 0 && application_message_size > 8 && 
        application_message_size < TERMINAL_COMMAND_BUFFER_SIZE && (command[application_message_size - 2] == '"'))
    {
        command[application_message_size - 2] = '\0'; // replace the '"' with \0
        load_application(&command[7]);
        return true;
    }
    return false;
}

/// <summary>
/// Handler called to process inbound message
/// </summary>
void terminal_handler(WS_INPUT_BLOCK_T *in_block)
{
    char command[TERMINAL_COMMAND_BUFFER_SIZE];
    memset(command, 0x00, sizeof(command));

    pthread_mutex_lock(&in_block->block_lock);

    size_t application_message_size = in_block->length;
    char *data                      = in_block->buffer;

    // Was just enter pressed
    if (data[0] == '\r')
    {
        if (handle_enter_key(data)) {
            goto cleanup;
        }
    }

    // Handle control characters
    if (handle_ctrl_character(data, application_message_size))
    {
        goto cleanup;
    }

    // Handle single character input
    if (handle_single_character(data, application_message_size))
    {
        goto cleanup;
    }

    // Check for loadx command
    // upper case the first 30 chars for command processing with bounds checking
    size_t copy_len = (sizeof(command) - 1 < application_message_size) ? sizeof(command) - 1 : application_message_size;
    for (size_t i = 0; i < copy_len; i++)
    {
        command[i] = (char)toupper(data[i]);
    }
    command[copy_len] = '\0'; // Ensure null termination

    // Handle LOADX command
    if (handle_loadx_command(command, application_message_size))
    {
        goto cleanup;
    }

    switch (get_cpu_operating_mode())
    {
        case CPU_RUNNING:

            if (application_message_size > 0)
            {
                input_data = data;

                altairInputBufReadIndex  = 0;
                altairOutputBufReadIndex = 0;
                terminalInputMessageLen  = (int)application_message_size;
                terminalOutputMessageLen = (int)application_message_size - 1;

                haveTerminalInputMessage = haveTerminalOutputMessage = true;

                wait_for_terminal_completion(&haveTerminalInputMessage);
            }
            break;
        case CPU_STOPPED:
            process_virtual_input(command);
            break;
        default:
            break;
    }

cleanup:
    in_block->length = 0;
    pthread_mutex_unlock(&in_block->block_lock);
}

static void send_terminal_character(char character, bool wait)
{
    int retry              = 0;
    terminalInputCharacter = character;

    if (!wait)
    {
        return;
    }

    while (terminalInputCharacter && retry++ < 10)
    {
        nanosleep(&(struct timespec){0, 1 * ONE_MS}, NULL);
    }
}

/// <summary>
/// Wait for terminal input message with proper synchronization
/// </summary>
/// <param name="flag">Pointer to flag to wait for</param>
static void wait_for_terminal_completion(bool *flag)
{
    pthread_mutex_lock(&terminal_state_mutex);
    
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_nsec += TERMINAL_WAIT_TIMEOUT_MS * ONE_MS; // timeout
    if (timeout.tv_nsec >= 1000000000) {
        timeout.tv_sec++;
        timeout.tv_nsec -= 1000000000;
    }
    
    *flag = true;
    
    // Wait for flag to be reset or timeout
    while (*flag) {
        int result = pthread_cond_timedwait(&terminal_input_cond, &terminal_state_mutex, &timeout);
        if (result == ETIMEDOUT) {
            dx_Log_Debug("Terminal input wait timeout\n");
            break;
        }
    }
    
    pthread_mutex_unlock(&terminal_state_mutex);
}

/// <summary>
/// Signal terminal completion
/// </summary>
/// <param name="flag">Pointer to flag to reset</param>
static void signal_terminal_completion(bool *flag)
{
    pthread_mutex_lock(&terminal_state_mutex);
    *flag = false;
    pthread_cond_signal(&terminal_input_cond);
    pthread_mutex_unlock(&terminal_state_mutex);
}

/// <summary>
/// Client connected successfully
/// </summary>
static void client_connected_cb(void)
{
    print_console_banner();
    set_cpu_operating_mode(CPU_RUNNING);
}

/// <summary>
/// Validate filename for security and length constraints
/// </summary>
/// <param name="filename">Filename to validate</param>
/// <returns>true if valid, false otherwise</returns>
static bool validate_filename(const char *filename)
{
    if (!filename || strlen(filename) == 0) {
        dx_Log_Debug("Invalid filename: null or empty\n");
        return false;
    }
    
    size_t len = strlen(filename);
    if (len >= MAX_FILENAME_LENGTH) { // Leave room for directory and null terminator
        dx_Log_Debug("Filename too long: %zu characters\n", len);
        return false;
    }
    
    // Check for directory traversal attempts
    if (strstr(filename, "..") || strstr(filename, "/") || strstr(filename, "\\")) {
        dx_Log_Debug("Invalid filename: contains path separators or traversal\n");
        return false;
    }
    
    return true;
}

/// <summary>
/// Load sample BASIC applications
/// </summary>
/// <param name="fileName"></param>
/// <returns></returns>
static bool load_application(const char *fileName)
{
    if (!validate_filename(fileName)) {
        publish_message("\n\rInvalid filename\n\r", 19);
        return false;
    }

    int retry = 0;
    char filePathAndName[FILE_PATH_BUFFER_SIZE]; // Increased buffer size for safety
    int result = snprintf(filePathAndName, sizeof(filePathAndName), "%s/%s", APP_SAMPLES_DIRECTORY, fileName);
    
    if (result >= sizeof(filePathAndName)) {
        dx_Log_Debug("File path too long: %d characters\n", result);
        publish_message("\n\rFile path too long\n\r", 22);
        return false;
    }

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
        while (haveAppLoad && retry++ < MAX_RETRY_COUNT)
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

    if (terminalInputCharacter)
    {
        retVal                 = terminalInputCharacter;
        terminalInputCharacter = 0x00;
        retVal &= ASCII_MASK_7BIT; // take first 7 bits (127 ascii chars)
        return retVal;
    }

    if (haveTerminalInputMessage)
    {
        retVal = input_data[altairInputBufReadIndex++];

        if (altairInputBufReadIndex >= terminalInputMessageLen)
        {
            signal_terminal_completion(&haveTerminalInputMessage);
        }
        retVal &= ASCII_MASK_7BIT; // take first 7 bits (127 ascii chars)
        return retVal;
    }

    if (haveAppLoad)
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
        retVal &= ASCII_MASK_7BIT; // take first 7 bits (127 ascii chars)
        return retVal;
    }
    return 0;
}

static void terminal_write(uint8_t c)
{
    c &= ASCII_MASK_7BIT; // take first 7 bits (127 ascii chars) only and discard 8th bit.

    if (haveTerminalOutputMessage)
    {
        altairOutputBufReadIndex++;

        if (altairOutputBufReadIndex > terminalOutputMessageLen)
        {
            haveTerminalOutputMessage = false;
        }
    }
    else
    {
        publish_character(c);
    }
}

static inline uint8_t sense(void)
{
    return (uint8_t)(bus_switches >> 8);
}

/// <summary>
/// Print welcome banner
/// </summary>
void print_console_banner(void)
{
    static bool first = true;
    // char reset[] = "\x1b[2J\r\n";
    const char reset[]          = "\r\n\r\n";
    const char altair_version[] = "\r\nAltair version: ";

    for (int x = 0; x < strlen(reset); x++)
    {
        terminal_write(reset[x]);
    }

    for (int x = 0; x < strlen(AltairMsg[AltairBannerCount]); x++)
    {
        terminal_write(AltairMsg[AltairBannerCount][x]);
    }

    AltairBannerCount++;
    AltairBannerCount = AltairBannerCount == NELEMS(AltairMsg) ? 0 : AltairBannerCount;

    if (first)
    {
        first = false;

        for (int x = 0; x < strlen(altair_version); x++)
        {
            terminal_write(altair_version[x]);
        }

        for (int x = 0; x < strlen(ALTAIR_EMULATOR_VERSION); x++)
        {
            terminal_write(ALTAIR_EMULATOR_VERSION[x]);
        }
    }
    else
    {
        send_terminal_character(0x0d, true);
    }
}

/// <summary>
/// Updates the PI Sense HAT with Altair address bus, data bus, and CPU state
/// </summary>
static void *panel_refresh_thread(void *arg)
{
    uint8_t last_status = 0;
    uint8_t last_data   = 0;
    uint16_t last_bus   = 0;

    while (true)
    {
        if (panel_mode == PANEL_BUS_MODE)
        {
            uint8_t status = cpu.cpuStatus;
            uint8_t data   = cpu.data_bus;
            uint16_t bus   = cpu.address_bus;

            // if (status != last_status || data != last_data || bus != last_bus)
            // {
                last_status = status;
                last_data   = data;
                last_bus    = bus;

                status = (uint8_t)(reverse_lut[(status & 0xf0) >> 4] | reverse_lut[status & 0xf] << 4);
                data   = (uint8_t)(reverse_lut[(data & 0xf0) >> 4] | reverse_lut[data & 0xf] << 4);
                bus    = (uint16_t)(reverse_lut[(bus & 0xf000) >> 12] << 8 | reverse_lut[(bus & 0x0f00) >> 8] << 12 |
                                 reverse_lut[(bus & 0xf0) >> 4] | reverse_lut[bus & 0xf] << 4);

                front_panel_io(status, data, bus, process_control_panel_commands);
            // }
            nanosleep(&(struct timespec){0, 5 * ONE_MS}, NULL);
        }
        else
        {
            nanosleep(&(struct timespec){0, 500 * ONE_MS}, NULL);
        }
    }
    return NULL;
}

/// <summary>
/// Initialize a single disk drive
/// </summary>
/// <param name="disk_path">Path to the disk image file</param>
/// <param name="fp">Pointer to file descriptor to set</param>
/// <param name="disk_name">Name of the disk for logging</param>
/// <param name="exit_code">Exit code to use if initialization fails</param>
/// <returns>true on success, false on failure</returns>
static bool init_altair_disk(const char* disk_path, int* fp, const char* disk_name, APP_EXIT_CODE exit_code)
{
#ifdef ALTAIR_CLOUD
    if ((*fp = open(disk_path, O_RDONLY)) == -1)
    {
#else
    if ((*fp = open(disk_path, O_RDWR)) == -1)
    {
#endif
        Log_Debug("Failed to open %s disk image: %s\n", disk_name, strerror(errno));
        dx_terminate(exit_code);
        return false;
    }
    return true;
}

/// <summary>
/// Initialize the Altair disks and i8080 cpu
/// </summary>
static void init_altair(void)
{
    Log_Debug("Altair Thread starting...\n");
    // print_console_banner();

    memset(memory, 0x00, MEMORY_SIZE_64K); // clear Altair memory.

    disk_controller_t disk_controller;
    disk_controller.disk_function = disk_function;
    disk_controller.disk_select   = disk_select;
    disk_controller.disk_status   = disk_status;
    disk_controller.read          = disk_read;
    disk_controller.write         = disk_write;
    disk_controller.sector        = sector;

    // Initialize all disk drives
    if (!init_altair_disk(DISK_A, &disk_drive.disk1.fp, "DISK_A", APP_EXIT_DISK_A_INIT_FAILED))
        return;
    disk_drive.disk1.diskPointer = 0;
    disk_drive.disk1.sector      = 0;
    disk_drive.disk1.track       = 0;

    if (!init_altair_disk(DISK_B, &disk_drive.disk2.fp, "DISK_B", APP_EXIT_DISK_B_INIT_FAILED))
        return;
    disk_drive.disk2.diskPointer = 0;
    disk_drive.disk2.sector      = 0;
    disk_drive.disk2.track       = 0;

    if (!init_altair_disk(DISK_C, &disk_drive.disk3.fp, "DISK_C", APP_EXIT_DISK_C_INIT_FAILED))
        return;
    disk_drive.disk3.diskPointer = 0;
    disk_drive.disk3.sector      = 0;
    disk_drive.disk3.track       = 0;

    if (!init_altair_disk(DISK_D, &disk_drive.disk4.fp, "DISK_D", APP_EXIT_DISK_D_INIT_FAILED))
        return;
    disk_drive.disk4.diskPointer = 0;
    disk_drive.disk4.sector      = 0;
    disk_drive.disk4.track       = 0;

    i8080_reset(&cpu, (port_in)terminal_read, (port_out)terminal_write, sense, &disk_controller, (azure_sphere_port_in)io_port_in,
        (azure_sphere_port_out)io_port_out);

    // load Disk Loader at ROM_LOADER_ADDRESS
    if (!loadRomImage(DISK_LOADER, ROM_LOADER_ADDRESS))
    {
        Log_Debug("Failed to open %s disk load ROM image\n", DISK_LOADER);
        dx_terminate(APP_EXIT_ROM_LOAD_FAILED);
        return;
    }

    i8080_examine(&cpu, ROM_LOADER_ADDRESS); // ROM_LOADER_ADDRESS loads from disk, 0x0000 loads basic
}

/// <summary>
/// Cleanup Altair disk file handles to prevent resource leaks
/// </summary>
static void cleanup_altair_disks(void)
{
    if (disk_drive.disk1.fp != -1) {
        close(disk_drive.disk1.fp);
        disk_drive.disk1.fp = -1;
    }
    if (disk_drive.disk2.fp != -1) {
        close(disk_drive.disk2.fp);
        disk_drive.disk2.fp = -1;
    }
    if (disk_drive.disk3.fp != -1) {
        close(disk_drive.disk3.fp);
        disk_drive.disk3.fp = -1;
    }
    if (disk_drive.disk4.fp != -1) {
        close(disk_drive.disk4.fp);
        disk_drive.disk4.fp = -1;
    }
}

/// <summary>
/// Thread to run the i8080 cpu emulator on
/// </summary>
static void *altair_thread(void *arg)
{
    // Log_Debug("Altair Thread starting...\n");
    if (altair_i8080_running)
    {
        return NULL;
    }

    altair_i8080_running = true;

    while (!stop_cpu)
    {
        if (get_cpu_operating_mode_fast() == CPU_RUNNING)
        {
            i8080_cycle(&cpu);
        }

        if (send_partial_msg)
        {
            send_partial_message();
            send_partial_msg = false;
        }
    }

    altair_i8080_running = false;

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
        snprintf(msgBuffer, sizeof(msgBuffer), "Altair emulator: %s, DevX: %s", ALTAIR_EMULATOR_VERSION, AZURE_SPHERE_DEVX_VERSION);
        dx_deviceTwinReportValue(&dt_softwareVersion, msgBuffer);
        dx_deviceTwinReportValue(&dt_deviceStartTimeUtc, dx_getCurrentUtc(msgBuffer, sizeof(msgBuffer))); // DX_TYPE_STRING

        dx_azureUnregisterConnectionChangedNotification(report_software_version);
    }
}

static void azure_connection_state(bool connection_state)
{
    azure_connected = connection_state;
}

/// <summary>
/// Create the a thread with a low priority to encourage a thread to run efficiency cores on Apple Silicon
/// </summary>
/// <param name="daemon"></param>
/// <param name="arg"></param>
/// <param name="daemon_name"></param>
/// <param name="priority"></param>
bool start_altair_thread(void *(*daemon)(void *), void *arg, char *daemon_name, int priority)
{
    pthread_t thread;
    pthread_attr_t attr;
    struct sched_param param;

    pthread_attr_init(&attr);
    
    // https://stackoverflow.com/questions/9392415/linux-sched-other-sched-fifo-and-sched-rr-differences
    pthread_attr_setschedpolicy(&attr, SCHED_RR);

    param.sched_priority = priority; // Set your desired priority (0-99)
    pthread_attr_setschedparam(&attr, &param);

    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_create(&thread, &attr, daemon, arg))
    {
        printf("ERROR: Failed to start %s daemon.\n", daemon_name);
        return false;
    }
    return true;
}

/// <summary>
///  Initialize PeripheralGpios, device twins, direct methods, timers.
/// </summary>
/// <returns>0 on success, or -1 on failure</returns>
static void InitPeripheralAndHandlers(int argc, char *argv[])
{
    // https://stackoverflow.com/questions/18935446/program-received-signal-sigpipe-broken-pipe
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGPIPE, &sa, NULL);

    curl_global_init(CURL_GLOBAL_ALL);

    setvbuf(stdout, NULL, _IONBF, 0); // disable stdout/printf buffering

    dx_Log_Debug_Init(Log_Debug_Time_buffer, sizeof(Log_Debug_Time_buffer));
    srand((unsigned int)time(NULL)); // seed the random number generator

    parse_altair_cmd_line_arguments(argc, argv, &altair_config);

    network_interface = altair_config.user_config.network_interface;

    init_environment(&altair_config);

    if (!dx_isStringNullOrEmpty(altair_config.openai_api_key)){
        init_openai(altair_config.openai_api_key);
    }

    dx_Log_Debug("Network interface %s %s\n", network_interface, dx_isNetworkConnected(network_interface) ? "connected" : "NOT connected");

    init_altair_hardware();

    dx_asyncSetInit(async_bindings, NELEMS(async_bindings));

    // if there is no ID Scope or connection string then don't attempt to start connection to Azure IoT
    // Central
    if (!dx_isStringNullOrEmpty(altair_config.user_config.idScope) || !dx_isStringNullOrEmpty(altair_config.user_config.connection_string))
    {
        dx_azureConnect(&altair_config.user_config, network_interface, IOT_PLUG_AND_PLAY_MODEL_ID);

        dx_azureRegisterConnectionChangedNotification(azure_connection_state);
        dx_azureRegisterConnectionChangedNotification(report_software_version);
    }

    dx_deviceTwinSubscribe(device_twin_bindings, NELEMS(device_twin_bindings));
    init_web_socket_server(client_connected_cb);
    dx_timerSetStart(timer_bindings, NELEMS(timer_bindings));

#if defined(ALTAIR_FRONT_PANEL_PI_SENSE) || defined(ALTAIR_FRONT_PANEL_KIT)
    dx_startThreadDetached(panel_refresh_thread, NULL, "panel_refresh_thread");
#endif

    init_altair();
    start_altair_thread(altair_thread, NULL, "altair_thread", 1);
    while (!altair_i8080_running) // spin until i8080 thread starts
    {
        nanosleep(&(struct timespec){0, 1 * ONE_MS}, NULL);
    }
}

/// <summary>
///     Close PeripheralGpios and handlers.
/// </summary>
static void ClosePeripheralAndHandlers(void)
{
    dx_azureToDeviceStop();
    dx_deviceTwinUnsubscribe();
    dx_timerEventLoopStop();

    cleanup_altair_disks();

#ifdef ALTAIR_FRONT_PANEL_PI_SENSE
    pi_sense_hat_sensors_close();
#endif

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
