/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "web_console.h"

#include <stdatomic.h>
#include <stdint.h>
#include <string.h>

// =============================================================================
// Constants and Configuration
// =============================================================================

#define OUTPUT_BUFFER_SIZE 4096
#define FLUSH_INTERVAL_MS 20
#define FLUSH_THRESHOLD_PERCENT 75
#define TERMINAL_INPUT_BUFFER_SIZE 256

// =============================================================================
// Type Definitions
// =============================================================================

// Output buffer structure
typedef struct {
    char buffer[OUTPUT_BUFFER_SIZE];
    _Atomic size_t head;
    _Atomic size_t tail;
    _Atomic size_t count;
} output_buffer_t;

// Terminal input queue structure
typedef struct {
    char buffer[TERMINAL_INPUT_BUFFER_SIZE];
    _Atomic size_t head;
    _Atomic size_t tail;
} terminal_input_queue_t;

// =============================================================================
// Static Variables
// =============================================================================

// Output buffer
static output_buffer_t output_buffer = {
    .head = 0,
    .tail = 0,
    .count = 0,
};

// Terminal input queue
static terminal_input_queue_t terminal_input_queue = {
    .buffer = {0},
    .head   = 0,
    .tail   = 0,
};

// WebSocket client management
static atomic_uintptr_t current_client = 0;
static void (*_client_connected_cb)(void);

// Session management
static bool cleanup_required = false;
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
    .repeat  = &(struct timespec){0, FLUSH_INTERVAL_MS * ONE_MS},
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

    size_t current_count = atomic_load_explicit(&output_buffer.count, memory_order_relaxed);
    
    // Check if buffer has enough space
    if (current_count + length > OUTPUT_BUFFER_SIZE)
    {
        return false;
    }

    const char *bytes = (const char *)data;
    size_t head = atomic_load_explicit(&output_buffer.head, memory_order_relaxed);
    
    for (size_t i = 0; i < length; i++)
    {
        output_buffer.buffer[(head + i) % OUTPUT_BUFFER_SIZE] = bytes[i];
    }
    
    // Update head and count atomically
    atomic_store_explicit(&output_buffer.head, (head + length) % OUTPUT_BUFFER_SIZE, memory_order_release);
    atomic_fetch_add_explicit(&output_buffer.count, length, memory_order_release);
    
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
        atomic_store_explicit(&output_buffer.head, 0, memory_order_relaxed);
        atomic_store_explicit(&output_buffer.tail, 0, memory_order_relaxed);
        atomic_store_explicit(&output_buffer.count, 0, memory_order_relaxed);
        return;
    }

    size_t bytes_to_send = atomic_load_explicit(&output_buffer.count, memory_order_acquire);
    if (bytes_to_send == 0)
    {
        return;
    }

    // Prepare data to send
    char temp_buffer[OUTPUT_BUFFER_SIZE];
    size_t tail = atomic_load_explicit(&output_buffer.tail, memory_order_relaxed);

    // Efficient copy from circular buffer using memcpy
    // Handle potential wraparound in circular buffer
    if (tail + bytes_to_send <= OUTPUT_BUFFER_SIZE)
    {
        // Contiguous data - single memcpy
        memcpy(temp_buffer, &output_buffer.buffer[tail], bytes_to_send);
    }
    else
    {
        // Data wraps around - two memcpy operations
        size_t first_chunk = OUTPUT_BUFFER_SIZE - tail;
        size_t second_chunk = bytes_to_send - first_chunk;
        
        memcpy(temp_buffer, &output_buffer.buffer[tail], first_chunk);
        memcpy(temp_buffer + first_chunk, &output_buffer.buffer[0], second_chunk);
    }

    // Update buffer state atomically
    atomic_store_explicit(&output_buffer.tail, (tail + bytes_to_send) % OUTPUT_BUFFER_SIZE, memory_order_release);
    atomic_store_explicit(&output_buffer.count, 0, memory_order_release);

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
    size_t current_count = atomic_load_explicit(&output_buffer.count, memory_order_relaxed);

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
    publish_message(&character, 1);
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
    atomic_store_explicit(&output_buffer.head, 0, memory_order_relaxed);
    atomic_store_explicit(&output_buffer.tail, 0, memory_order_relaxed);
    atomic_store_explicit(&output_buffer.count, 0, memory_order_relaxed);

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
    atomic_store_explicit(&output_buffer.head, 0, memory_order_relaxed);
    atomic_store_explicit(&output_buffer.tail, 0, memory_order_relaxed);
    atomic_store_explicit(&output_buffer.count, 0, memory_order_relaxed);

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

    // Initialize output buffer (atomics are already initialized to 0)
    atomic_store_explicit(&output_buffer.head, 0, memory_order_relaxed);
    atomic_store_explicit(&output_buffer.tail, 0, memory_order_relaxed);
    atomic_store_explicit(&output_buffer.count, 0, memory_order_relaxed);

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
    size_t tail = atomic_load_explicit(&terminal_input_queue.tail, memory_order_relaxed);
    size_t head = atomic_load_explicit(&terminal_input_queue.head, memory_order_acquire);

    if (tail - head >= terminal_queue_capacity())
    {
        return; // drop character if buffer full
    }

    terminal_input_queue.buffer[tail % terminal_queue_capacity()] = character;
    atomic_store_explicit(&terminal_input_queue.tail, tail + 1, memory_order_release);
}

/// <summary>
/// Remove and return character from terminal input queue
/// </summary>
/// <returns>Character from queue, or 0 if queue is empty</returns>
char dequeue_terminal_input_character(void)
{
    char c = 0;
    size_t head = atomic_load_explicit(&terminal_input_queue.head, memory_order_relaxed);
    size_t tail = atomic_load_explicit(&terminal_input_queue.tail, memory_order_acquire);

    if (head != tail)
    {
        c = terminal_input_queue.buffer[head % terminal_queue_capacity()];
        atomic_store_explicit(&terminal_input_queue.head, head + 1, memory_order_release);
    }
    
    return c;
}

/// <summary>
/// Clear the terminal input queue
/// </summary>
void clear_terminal_input_queue(void)
{
    atomic_store_explicit(&terminal_input_queue.head, 0, memory_order_relaxed);
    atomic_store_explicit(&terminal_input_queue.tail, 0, memory_order_relaxed);
}
