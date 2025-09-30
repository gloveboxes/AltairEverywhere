/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "web_console.h"

#include <stdatomic.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define OUTPUT_QUEUE_CAPACITY 4096
#define OUTPUT_CHUNK_TARGET   512

typedef struct
{
    char buffer[OUTPUT_QUEUE_CAPACITY];
    _Atomic size_t head;
    _Atomic size_t tail;
} OUTPUT_MESSAGE_QUEUE;

static OUTPUT_MESSAGE_QUEUE output_queue = {
    .buffer = {0},
    .head   = 0,
    .tail   = 0,
};

static DX_DECLARE_TIMER_HANDLER(expire_session_handler);
static void (*_client_connected_cb)(void);
static void cleanup_session(void);
static void publish_message_direct(const char *message, size_t message_length);
static void start_output_queue_consumer(void);
static DX_DECLARE_TIMER_HANDLER(output_queue_flush_handler);

static DX_TIMER_BINDING tmr_expire_session = {
    .name    = "tmr_expire_session",
    .handler = expire_session_handler,
};
static DX_TIMER_BINDING tmr_output_queue = {
    .repeat  = &(struct timespec){0, 20 * ONE_MS},
    .name    = "tmr_output_queue",
    .handler = output_queue_flush_handler,
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

static inline size_t output_queue_capacity(void)
{
    return sizeof(output_queue.buffer);
}

static size_t enqueue_output_bytes(const char *message, size_t message_length)
{
    size_t enqueued = 0;

    for (size_t index = 0; index < message_length; ++index)
    {
        size_t tail = atomic_load_explicit(&output_queue.tail, memory_order_relaxed);
        size_t head = atomic_load_explicit(&output_queue.head, memory_order_acquire);

        if (tail - head >= output_queue_capacity())
        {
            dx_Log_Debug("Output queue full, dropping remaining %zu byte%s\n", message_length - index, (message_length - index) == 1 ? "" : "s");
            break; // drop remaining bytes if buffer full
        }

        output_queue.buffer[tail % output_queue_capacity()] = message[index];
        atomic_store_explicit(&output_queue.tail, tail + 1, memory_order_release);
        enqueued++;
    }

    return enqueued;
}

static size_t dequeue_output_bytes(char *destination, size_t max_length)
{
    if (max_length == 0)
    {
        return 0;
    }

    const size_t capacity = output_queue_capacity();
    size_t head            = atomic_load_explicit(&output_queue.head, memory_order_relaxed);
    size_t tail            = atomic_load_explicit(&output_queue.tail, memory_order_acquire);
    size_t available       = tail - head;

    if (available == 0)
    {
        return 0;
    }

    size_t to_copy = available < max_length ? available : max_length;
    size_t index   = head % capacity;
    size_t first_chunk = capacity - index;

    if (first_chunk > to_copy)
    {
        first_chunk = to_copy;
    }

    memcpy(destination, &output_queue.buffer[index], first_chunk);

    if (first_chunk < to_copy)
    {
        memcpy(destination + first_chunk, output_queue.buffer, to_copy - first_chunk);
    }

    atomic_store_explicit(&output_queue.head, head + to_copy, memory_order_release);

    return to_copy;
}

static void publish_message_direct(const char *message, size_t message_length)
{
    // Validate input parameters
    if (message == NULL || message_length == 0)
    {
        printf("publish_message: Invalid message parameters\n");
        return;
    }

    if (current_client != 0)
    {
        if (ws_sendframe(current_client, message, message_length, WS_FR_OP_TXT) == -1)
        {
            printf("ws_sendframe failed - connection may be broken\n");
            // Close the problematic connection to prevent further errors
            ws_close_client(current_client);
            current_client = 0;
            if (cleanup_required)
            {
                cleanup_session();
            }
        }
    }
}

inline void publish_character(char character)
{
    publish_message(&character, 1);
}

void publish_message(const void *message, size_t message_length)
{
    if (message == NULL || message_length == 0)
    {
        printf("publish_message: Invalid message parameters\n");
        return;
    }

    size_t enqueued = enqueue_output_bytes((const char *)message, message_length);

    if (enqueued == 0)
    {
        return; // drop when buffer remains full
    }

    if (enqueued < message_length)
    {
        size_t dropped = message_length - enqueued;
        printf("publish_message: Output queue full, dropped %zu byte%s\n", dropped, dropped == 1 ? "" : "s");
    }
}

static DX_TIMER_HANDLER(output_queue_flush_handler)
{
    char chunk[OUTPUT_CHUNK_TARGET];

    while (true)
    {
        size_t bytes_to_send = dequeue_output_bytes(chunk, sizeof(chunk));

        if (bytes_to_send == 0)
        {
            break;
        }

        dx_Log_Debug("Flushing %zu byte%s from output queue\n", bytes_to_send, bytes_to_send == 1 ? "" : "s");

        publish_message_direct(chunk, bytes_to_send);

        if (bytes_to_send < sizeof(chunk))
        {
            break;
        }
    }
}
DX_TIMER_HANDLER_END

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

    printf("Session closed\n");
    current_client = 0;

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

    // start_output_queue_consumer();
    dx_timerStart(&tmr_output_queue);
    dx_timerStart(&tmr_expire_session);

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
