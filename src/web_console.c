/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "web_console.h"

#include <stdatomic.h>
#include <stdint.h>
#include <string.h>

// =============================================================================
// Constants and Configuration
// =============================================================================

#define OUTPUT_BUFFER_SIZE         4096
#define FLUSH_INTERVAL_MS          20
#define FLUSH_THRESHOLD_PERCENT    75
#define TERMINAL_INPUT_BUFFER_SIZE 256

// =============================================================================
// Type Definitions
// =============================================================================

// Output buffer structure
typedef struct
{
    char buffer[OUTPUT_BUFFER_SIZE];
    size_t head;
    size_t tail;
    size_t count;
    pthread_mutex_t mutex;
} output_buffer_t;

// Terminal input queue structure
typedef struct
{
    char buffer[TERMINAL_INPUT_BUFFER_SIZE];
    size_t head;
    size_t tail;
    pthread_mutex_t mutex;
} terminal_input_queue_t;

// =============================================================================
// Static Variables
// =============================================================================

// Output buffer
static output_buffer_t output_buffer = {
    .head  = 0,
    .tail  = 0,
    .count = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
};

// Terminal input queue
static terminal_input_queue_t terminal_input_queue = {
    .buffer = {0},
    .head   = 0,
    .tail   = 0,
    .mutex  = PTHREAD_MUTEX_INITIALIZER,
};

// WebSocket client management
static atomic_uintptr_t current_client = 0;
static void (*_client_connected_cb)(void);

// Session management
static bool cleanup_required     = false;
static const int session_minutes = 1 * 60 * 30; // 30 minutes

#ifdef ALTAIR_CLOUD
static struct timeval ws_timeout = {0, 250 * 1000};
#endif

// =============================================================================
// Forward Declarations
// =============================================================================

static DX_DECLARE_TIMER_HANDLER(expire_session_handler);
static DX_DECLARE_TIMER_HANDLER(flush_output_buffer_handler);
static void cleanup_session(void);
static void flush_output_buffer(void);
static bool buffer_add_data(const void *data, size_t length);
static inline size_t terminal_queue_capacity(void);

// =============================================================================
// Timer Bindings
// =============================================================================

static DX_TIMER_BINDING tmr_expire_session = {
    .name    = "tmr_expire_session",
    .handler = expire_session_handler,
};

static DX_TIMER_BINDING tmr_flush_output = {
    .name    = "tmr_flush_output",
    .handler = flush_output_buffer_handler,
    .repeat  = &(struct timespec){0, FLUSH_INTERVAL_MS *ONE_MS},
};

// =============================================================================
// Session Management Functions
// =============================================================================

/// <summary>
/// Clean up session resources
/// </summary>
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

// =============================================================================
// Event Handlers
// =============================================================================

/// <summary>
/// Timer handler for session expiration
/// </summary>
static DX_TIMER_HANDLER(expire_session_handler)
{
    ws_cli_conn_t client = (ws_cli_conn_t)atomic_load(&current_client);
    if (client != 0)
    {
        ws_close_client(client);
    }
    cleanup_session();
}
DX_TIMER_HANDLER_END

/// <summary>
/// Timer handler for WebSocket ping/pong
/// </summary>
DX_TIMER_HANDLER(ws_ping_pong_handler)
{
    ws_cli_conn_t client = (ws_cli_conn_t)atomic_load(&current_client);
    if (client != 0)
    {
        // Allow for up to 60 seconds (6 missed pings × 10 sec interval) before closing
        // Note: Browsers should auto-respond to PING with PONG at protocol level
        ws_ping(client, 6);
    }
}
DX_TIMER_HANDLER_END

/// <summary>
/// Async handler for session expiration
/// </summary>
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

// =============================================================================
// Output Buffer Functions
// =============================================================================

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
        output_buffer.head                       = (output_buffer.head + 1) % OUTPUT_BUFFER_SIZE;
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
    ws_cli_conn_t client = (ws_cli_conn_t)atomic_load(&current_client);
    if (client == 0)
    {
        // No client connected, clear the buffer
        pthread_mutex_lock(&output_buffer.mutex);
        output_buffer.head  = 0;
        output_buffer.tail  = 0;
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
    size_t bytes_to_send = output_buffer.count;

    // Efficient copy from circular buffer using memcpy
    // Handle potential wraparound in circular buffer
    if (output_buffer.tail + bytes_to_send <= OUTPUT_BUFFER_SIZE)
    {
        // Contiguous data - single memcpy
        memcpy(temp_buffer, &output_buffer.buffer[output_buffer.tail], bytes_to_send);
    }
    else
    {
        // Data wraps around - two memcpy operations
        size_t first_chunk  = OUTPUT_BUFFER_SIZE - output_buffer.tail;
        size_t second_chunk = bytes_to_send - first_chunk;

        memcpy(temp_buffer, &output_buffer.buffer[output_buffer.tail], first_chunk);
        memcpy(temp_buffer + first_chunk, &output_buffer.buffer[0], second_chunk);
    }

    // Update buffer state
    output_buffer.tail  = (output_buffer.tail + bytes_to_send) % OUTPUT_BUFFER_SIZE;
    output_buffer.count = 0;

    pthread_mutex_unlock(&output_buffer.mutex);

    // Send the buffered data
    if (bytes_to_send > 0)
    {
        // printf("Flushing output buffer: %zu bytes\n", bytes_to_send);
        if (ws_sendframe(client, temp_buffer, bytes_to_send, WS_FR_OP_TXT) == -1)
        {
            printf("ws_sendframe failed - connection may be broken\n");
            ws_close_client(client);
            atomic_store(&current_client, 0);
            if (cleanup_required)
            {
                cleanup_session();
            }
        }
    }
}

// =============================================================================
// Output Functions
// =============================================================================

/// <summary>
/// Publish a message to the WebSocket client
/// </summary>
/// <param name="message">Message data to send</param>
/// <param name="message_length">Length of the message</param>
void publish_message(const void *message, size_t message_length)
{
    // Validate input parameters
    if (message == NULL || message_length == 0)
    {
        printf("publish_message: Invalid message parameters\n");
        return;
    }

    ws_cli_conn_t client = (ws_cli_conn_t)atomic_load(&current_client);
    if (client == 0)
    {
        return;
    }

    // Check if message is too large for buffer
    if (message_length > OUTPUT_BUFFER_SIZE)
    {
        // Message too large, flush buffer first to maintain order, then send directly
        flush_output_buffer();
        
        if (ws_sendframe(client, message, message_length, WS_FR_OP_TXT) == -1)
        {
            printf("ws_sendframe failed - connection may be broken\n");
            ws_close_client(client);
            atomic_store(&current_client, 0);
            if (cleanup_required)
            {
                cleanup_session();
            }
        }
        return;
    }

    // Try to add data to buffer
    bool added = buffer_add_data(message, message_length);

    if (!added)
    {
        // Buffer is full, flush it first then add the message
        flush_output_buffer();
        
        // Try adding again (should succeed now since buffer was flushed)
        added = buffer_add_data(message, message_length);
        
        if (!added)
        {
            // This shouldn't happen since we just flushed, but handle it
            printf("Failed to buffer message after flush - this shouldn't happen\n");
            if (ws_sendframe(client, message, message_length, WS_FR_OP_TXT) == -1)
            {
                printf("ws_sendframe failed - connection may be broken\n");
                ws_close_client(client);
                atomic_store(&current_client, 0);
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

/// <summary>
/// Publish a single character to the WebSocket client
/// </summary>
/// <param name="character">Character to send</param>
inline void publish_character(char character)
{
    // print to stdout for debugging
    // the text is not appearing in vs code terminal?
    fflush(stdout);
    dx_Log_Debug("%c", character);
    publish_message(&character, 1);
}

// =============================================================================
// Terminal Input Queue Functions
// =============================================================================

/// <summary>
/// Get terminal queue capacity
/// </summary>
/// <returns>Size of the terminal input buffer</returns>
static inline size_t terminal_queue_capacity(void)
{
    return sizeof(terminal_input_queue.buffer);
}

/// <summary>
/// Add character to terminal input queue
/// </summary>
/// <param name="character">Character to enqueue</param>
void enqueue_terminal_input_character(char character)
{
    pthread_mutex_lock(&terminal_input_queue.mutex);

    size_t tail = terminal_input_queue.tail;
    size_t head = terminal_input_queue.head;

    if (tail - head >= terminal_queue_capacity())
    {
        pthread_mutex_unlock(&terminal_input_queue.mutex);
        return; // drop character if buffer full
    }

    terminal_input_queue.buffer[tail % terminal_queue_capacity()] = character;
    terminal_input_queue.tail                                     = tail + 1;

    pthread_mutex_unlock(&terminal_input_queue.mutex);
}

/// <summary>
/// Remove and return character from terminal input queue
/// </summary>
/// <returns>Character from queue, or 0 if queue is empty</returns>
char dequeue_terminal_input_character(void)
{
    char c = 0;

    pthread_mutex_lock(&terminal_input_queue.mutex);

    size_t head = terminal_input_queue.head;
    size_t tail = terminal_input_queue.tail;

    if (head != tail)
    {
        c                         = terminal_input_queue.buffer[head % terminal_queue_capacity()];
        terminal_input_queue.head = head + 1;
    }

    pthread_mutex_unlock(&terminal_input_queue.mutex);
    return c;
}

/// <summary>
/// Clear the terminal input queue
/// </summary>
void clear_terminal_input_queue(void)
{
    pthread_mutex_lock(&terminal_input_queue.mutex);
    terminal_input_queue.head = 0;
    terminal_input_queue.tail = 0;
    pthread_mutex_unlock(&terminal_input_queue.mutex);
}

// =============================================================================
// WebSocket Event Handlers
// =============================================================================

/// <summary>
/// WebSocket connection opened event handler
/// </summary>
/// <param name="client">WebSocket client connection</param>
void onopen(ws_cli_conn_t client)
{
    // Validate client connection
    if (client == 0)
    {
        printf("onopen: Invalid client connection\n");
        return;
    }

    printf("New session\n");
    atomic_store(&current_client, (uintptr_t)client);

    // Clear the output buffer for new client
    pthread_mutex_lock(&output_buffer.mutex);
    output_buffer.head  = 0;
    output_buffer.tail  = 0;
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

/// <summary>
/// WebSocket connection closed event handler
/// </summary>
/// <param name="client">WebSocket client connection</param>
void onclose(ws_cli_conn_t client)
{
    // Validate that we're closing the current client
    ws_cli_conn_t current = (ws_cli_conn_t)atomic_load(&current_client);
    if (client != current && current != 0)
    {
        printf("onclose: Closing client does not match current client\n");
    }

    // Flush any remaining data before closing
    flush_output_buffer();

    printf("Session closed\n");
    atomic_store(&current_client, 0);

    // Clear the output buffer
    pthread_mutex_lock(&output_buffer.mutex);
    output_buffer.head  = 0;
    output_buffer.tail  = 0;
    output_buffer.count = 0;
    pthread_mutex_unlock(&output_buffer.mutex);

    if (cleanup_required)
    {
        cleanup_session();
    }
}

/// <summary>
/// WebSocket message received event handler
/// </summary>
/// <param name="client">WebSocket client connection</param>
/// <param name="msg">Message data received</param>
/// <param name="size">Size of the message</param>
/// <param name="type">Message type</param>
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

// =============================================================================
// WebSocket Server Functions
// =============================================================================

/// <summary>
/// Initialize the WebSocket server
/// </summary>
/// <param name="client_connected_cb">Callback function called when client connects</param>
void init_web_socket_server(void (*client_connected_cb)(void))
{
    // Validate input parameter
    if (client_connected_cb == NULL)
    {
        printf("init_web_socket_server: client_connected_cb callback is NULL\n");
        return;
    }

    _client_connected_cb = client_connected_cb;

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
