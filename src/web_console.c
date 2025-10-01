/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "web_console.h"

#include <stdatomic.h>
#include <stdint.h>
#include <string.h>

// Output buffering configuration
#define OUTPUT_BUFFER_SIZE 4096
#define FLUSH_INTERVAL_MS 20
#define FLUSH_THRESHOLD_PERCENT 75

// Output buffer structure
typedef struct {
    char buffer[OUTPUT_BUFFER_SIZE];
    size_t head;
    size_t tail;
    size_t count;
    pthread_mutex_t mutex;
} output_buffer_t;

static output_buffer_t output_buffer = {
    .head = 0,
    .tail = 0,
    .count = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER
};

static DX_DECLARE_TIMER_HANDLER(expire_session_handler);
static DX_DECLARE_TIMER_HANDLER(flush_output_buffer_handler);
static void (*_client_connected_cb)(void);
static void cleanup_session(void);
static void flush_output_buffer(void);
static bool buffer_add_data(const void *data, size_t length);

static DX_TIMER_BINDING tmr_expire_session = {
    .name    = "tmr_expire_session",
    .handler = expire_session_handler,
};

static DX_TIMER_BINDING tmr_flush_output = {
    .name    = "tmr_flush_output",
    .handler = flush_output_buffer_handler,
    .repeat  = &(struct timespec){0, FLUSH_INTERVAL_MS * ONE_MS},
};

static bool cleanup_required     = false;
static const int session_minutes = 1 * 60 * 30; // 30 minutes
ws_cli_conn_t current_client     = 0;

#ifdef ALTAIR_CLOUD
static struct timeval ws_timeout = {0, 250 * 1000};
#endif

static DX_TIMER_HANDLER(expire_session_handler)
{
    if (current_client != 0)
    {
        ws_close_client(current_client);
    }
    cleanup_session();
}
DX_TIMER_HANDLER_END

DX_TIMER_HANDLER(ws_ping_pong_handler)
{
    if (current_client != 0)
    {
        // allow for up to 60 seconds of no pong response before closing the ws connection
        ws_ping(current_client, 6);
    }
}
DX_TIMER_HANDLER_END

DX_ASYNC_HANDLER(async_expire_session_handler, handle)
{
    dx_timerOneShotSet(&tmr_expire_session, &(struct timespec){session_minutes, 0});
}
DX_ASYNC_HANDLER_END

/// <summary>
/// Timer handler to periodically flush output buffer
/// </summary>
static DX_TIMER_HANDLER(flush_output_buffer_handler)
{
    flush_output_buffer();
}
DX_TIMER_HANDLER_END

/// <summary>
/// Add data to the output buffer
/// </summary>
/// <param name="data">Data to add</param>
/// <param name="length">Length of data</param>
/// <returns>true if added successfully, false if buffer full</returns>
static bool buffer_add_data(const void *data, size_t length)
{
    if (data == NULL || length == 0)
    {
        return false;
    }

    pthread_mutex_lock(&output_buffer.mutex);

    // Check if buffer has enough space
    if (output_buffer.count + length > OUTPUT_BUFFER_SIZE)
    {
        pthread_mutex_unlock(&output_buffer.mutex);
        return false;
    }

    const char *bytes = (const char *)data;
    for (size_t i = 0; i < length; i++)
    {
        output_buffer.buffer[output_buffer.head] = bytes[i];
        output_buffer.head = (output_buffer.head + 1) % OUTPUT_BUFFER_SIZE;
        output_buffer.count++;
    }

    pthread_mutex_unlock(&output_buffer.mutex);
    return true;
}

/// <summary>
/// Flush the output buffer to the WebSocket client
/// </summary>
static void flush_output_buffer(void)
{
    if (current_client == 0)
    {
        // No client connected, clear the buffer
        pthread_mutex_lock(&output_buffer.mutex);
        output_buffer.head = 0;
        output_buffer.tail = 0;
        output_buffer.count = 0;
        pthread_mutex_unlock(&output_buffer.mutex);
        return;
    }

    pthread_mutex_lock(&output_buffer.mutex);

    if (output_buffer.count == 0)
    {
        pthread_mutex_unlock(&output_buffer.mutex);
        return;
    }

    // Prepare data to send
    char temp_buffer[OUTPUT_BUFFER_SIZE];
    size_t bytes_to_send = 0;

    // Copy data from circular buffer to temporary buffer
    while (output_buffer.count > 0 && bytes_to_send < OUTPUT_BUFFER_SIZE)
    {
        temp_buffer[bytes_to_send++] = output_buffer.buffer[output_buffer.tail];
        output_buffer.tail = (output_buffer.tail + 1) % OUTPUT_BUFFER_SIZE;
        output_buffer.count--;
    }

    pthread_mutex_unlock(&output_buffer.mutex);

    // Send the buffered data
    if (bytes_to_send > 0)
    {
        printf("Flushing output buffer: %zu bytes\n", bytes_to_send);
        if (ws_sendframe(current_client, temp_buffer, bytes_to_send, WS_FR_OP_TXT) == -1)
        {
            printf("ws_sendframe failed - connection may be broken\n");
            ws_close_client(current_client);
            current_client = 0;
            if (cleanup_required)
            {
                cleanup_session();
            }
        }
    }
}

static void cleanup_session(void)
{
#ifdef ALTAIR_CLOUD
    set_cpu_operating_mode(CPU_STOPPED);

    // Sleep this thread so the Altair CPU thread can complete current instruction
    nanosleep(&(struct timespec){0, 250 * ONE_MS}, NULL);

    load_boot_disk();
    clear_difference_disk();
#endif
    cleanup_required = false;
}

void publish_message(const void *message, size_t message_length)
{
    // Validate input parameters
    if (message == NULL || message_length == 0)
    {
        printf("publish_message: Invalid message parameters\n");
        return;
    }

    if (current_client == 0)
    {
        return;
    }

    // Try to add data to buffer
    bool added = buffer_add_data(message, message_length);

    if (!added)
    {
        // Buffer is full, flush it first
        flush_output_buffer();
        
        // Try adding again
        added = buffer_add_data(message, message_length);
        
        if (!added)
        {
            // Message is too large, send directly
            if (ws_sendframe(current_client, message, message_length, WS_FR_OP_TXT) == -1)
            {
                printf("ws_sendframe failed - connection may be broken\n");
                ws_close_client(current_client);
                current_client = 0;
                if (cleanup_required)
                {
                    cleanup_session();
                }
            }
            return;
        }
    }

    // Check if we should flush immediately (only on threshold, not newlines)
    size_t threshold = (OUTPUT_BUFFER_SIZE * FLUSH_THRESHOLD_PERCENT) / 100;
    
    pthread_mutex_lock(&output_buffer.mutex);
    size_t current_count = output_buffer.count;
    pthread_mutex_unlock(&output_buffer.mutex);

    // Flush only if buffer is above threshold
    // Timer will handle periodic flushing (every 20ms)
    if (current_count >= threshold)
    {
        flush_output_buffer();
    }
}

inline void publish_character(char character)
{
    publish_message(&character, 1);
}

void onopen(ws_cli_conn_t client)
{
    // Validate client connection
    if (client == 0)
    {
        printf("onopen: Invalid client connection\n");
        return;
    }

    printf("New session\n");
    current_client = client;

    // Clear the output buffer for new client
    pthread_mutex_lock(&output_buffer.mutex);
    output_buffer.head = 0;
    output_buffer.tail = 0;
    output_buffer.count = 0;
    pthread_mutex_unlock(&output_buffer.mutex);

    if (cleanup_required)
    {
        cleanup_session();
    }

#ifdef ALTAIR_CLOUD
    cleanup_required = true;
    dx_asyncSend(&async_expire_session, NULL);
#endif

    // Log new session instead of device twin reporting
    printf("New WebSocket session established\n");

    // Call client connected callback if it's valid
    if (_client_connected_cb != NULL)
    {
        _client_connected_cb();
    }
}

void onclose(ws_cli_conn_t client)
{
    // Validate that we're closing the current client
    if (client != current_client && current_client != 0)
    {
        printf("onclose: Closing client does not match current client\n");
    }

    // Flush any remaining data before closing
    flush_output_buffer();

    printf("Session closed\n");
    current_client = 0;

    // Clear the output buffer
    pthread_mutex_lock(&output_buffer.mutex);
    output_buffer.head = 0;
    output_buffer.tail = 0;
    output_buffer.count = 0;
    pthread_mutex_unlock(&output_buffer.mutex);

    if (cleanup_required)
    {
        cleanup_session();
    }
}

void onmessage(ws_cli_conn_t client, const unsigned char *msg, uint64_t size, int type)
{
    (void)client;
    (void)type;

    if (msg == NULL || size == 0)
    {
        return;
    }

    terminal_handler((char *)msg, (size_t)size);
}

void init_web_socket_server(void (*client_connected_cb)(void))
{
    // Validate input parameter
    if (client_connected_cb == NULL)
    {
        printf("init_web_socket_server: client_connected_cb callback is NULL\n");
        return;
    }

    _client_connected_cb = client_connected_cb;

    // Initialize output buffer
    pthread_mutex_init(&output_buffer.mutex, NULL);
    output_buffer.head = 0;
    output_buffer.tail = 0;
    output_buffer.count = 0;

    // Start timers
    dx_timerStart(&tmr_expire_session);
    dx_timerStart(&tmr_flush_output);

    struct ws_server ws_srv = {.host = NULL, // NULL means bind to all interfaces
        .port                        = 8082,
        .thread_loop                 = 1,
        .timeout_ms                  = 250,
        .evs                         = {.onopen = &onopen, .onclose = &onclose, .onmessage = &onmessage},
        .context                     = NULL};

    ws_socket(&ws_srv);
}

/// <summary>
/// Partial message check callback
/// </summary>
/// <param name="eventLoopTimer"></param>
