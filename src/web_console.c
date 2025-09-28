/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "web_console.h"

#include <stdatomic.h>
#include <stdint.h>
#include <string.h>

static DX_DECLARE_TIMER_HANDLER(expire_session_handler);
static void (*_client_connected_cb)(void);
static void cleanup_session(void);

#define WS_MAX_MESSAGE_SIZE  512
#define WS_OUTPUT_QUEUE_SIZE 1024
#define WS_FLUSH_CHUNK_SIZE  256
#define WS_INPUT_RING_SIZE   1024

typedef struct
{
    char buffer[WS_INPUT_RING_SIZE];
    _Atomic size_t head;
    _Atomic size_t tail;
} WS_INPUT_RING_T;

typedef struct
{
    char data[WS_OUTPUT_QUEUE_SIZE];
    _Atomic size_t head;
    _Atomic size_t tail;
} WS_CHAR_QUEUE_T;

static WS_INPUT_RING_T ws_input_ring;
static WS_CHAR_QUEUE_T ws_output_queue;
static bool enqueue_input_message(const unsigned char *msg, size_t length);
static void process_input_queue(void);
static bool enqueue_output_character(char character);
static bool output_queue_has_data(void);
static bool publish_character_immediate(char character);

static DX_TIMER_BINDING tmr_expire_session = {
    .name    = "tmr_expire_session",
    .handler = expire_session_handler,
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

static bool enqueue_input_message(const unsigned char *msg, size_t length)
{
    if (msg == NULL || length == 0)
    {
        return false;
    }

    size_t tail = atomic_load_explicit(&ws_input_ring.tail, memory_order_relaxed);
    size_t head = atomic_load_explicit(&ws_input_ring.head, memory_order_acquire);

    size_t used      = tail - head;
    size_t available = WS_INPUT_RING_SIZE - used;

    if (length > available)
    {
        dx_Log_Debug("enqueue_input_message: input buffer overflow (len=%zu available=%zu)\n", length, available);
        return false;
    }

    size_t write_index = tail % WS_INPUT_RING_SIZE;
    size_t first_copy  = WS_INPUT_RING_SIZE - write_index;
    if (first_copy > length)
    {
        first_copy = length;
    }

    memcpy(&ws_input_ring.buffer[write_index], msg, first_copy);

    if (length > first_copy)
    {
        memcpy(ws_input_ring.buffer, msg + first_copy, length - first_copy);
    }

    size_t new_tail = tail + length;
    atomic_store_explicit(&ws_input_ring.tail, new_tail, memory_order_release);

    return true;
}

static void process_input_queue(void)
{
    size_t head = atomic_load_explicit(&ws_input_ring.head, memory_order_relaxed);
    size_t tail = atomic_load_explicit(&ws_input_ring.tail, memory_order_acquire);

    char batch[WS_MAX_MESSAGE_SIZE];

    while (head != tail)
    {
        size_t available = tail - head;
        size_t chunk_len = available > WS_MAX_MESSAGE_SIZE ? WS_MAX_MESSAGE_SIZE : available;

        size_t start_index = head % WS_INPUT_RING_SIZE;
        size_t first_copy  = WS_INPUT_RING_SIZE - start_index;
        if (first_copy > chunk_len)
        {
            first_copy = chunk_len;
        }

        memcpy(batch, &ws_input_ring.buffer[start_index], first_copy);
        if (chunk_len > first_copy)
        {
            memcpy(batch + first_copy, ws_input_ring.buffer, chunk_len - first_copy);
        }

        head += chunk_len;
        atomic_store_explicit(&ws_input_ring.head, head, memory_order_release);

        terminal_handler(batch, chunk_len);

        tail = atomic_load_explicit(&ws_input_ring.tail, memory_order_acquire);
    }
}

static bool enqueue_output_character(char character)
{
    size_t tail      = atomic_load_explicit(&ws_output_queue.tail, memory_order_relaxed);
    size_t head      = atomic_load_explicit(&ws_output_queue.head, memory_order_acquire);
    size_t next_tail = (tail + 1) % WS_OUTPUT_QUEUE_SIZE;

    if (next_tail == head)
    {
        return false; // queue full
    }

    ws_output_queue.data[tail] = character;
    atomic_store_explicit(&ws_output_queue.tail, next_tail, memory_order_release);

    return true;
}

static bool output_queue_has_data(void)
{
    size_t head = atomic_load_explicit(&ws_output_queue.head, memory_order_acquire);
    size_t tail = atomic_load_explicit(&ws_output_queue.tail, memory_order_acquire);
    return head != tail;
}

static bool publish_character_immediate(char character)
{
    size_t head = atomic_load_explicit(&ws_output_queue.head, memory_order_acquire);
    size_t tail = atomic_load_explicit(&ws_output_queue.tail, memory_order_relaxed);

    if (head != tail)
    {
        return false;
    }

    char immediate_char = character;
    publish_message(&immediate_char, 1);
    return true;
}

void publish_message(const void *message, size_t message_length)
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

void send_partial_message(void)
{
    char chunk[WS_FLUSH_CHUNK_SIZE];

    while (true)
    {
        size_t head = atomic_load_explicit(&ws_output_queue.head, memory_order_relaxed);
        size_t tail = atomic_load_explicit(&ws_output_queue.tail, memory_order_acquire);

        if (head == tail)
        {
            break; // queue empty
        }

        size_t chunk_length = 0;
        size_t local_head   = head;

        while (local_head != tail && chunk_length < sizeof(chunk))
        {
            chunk[chunk_length++] = ws_output_queue.data[local_head];
            local_head            = (local_head + 1) % WS_OUTPUT_QUEUE_SIZE;
        }

        atomic_store_explicit(&ws_output_queue.head, local_head, memory_order_release);

        publish_message(chunk, chunk_length);
    }
}

inline void publish_character(char character)
{
    if (publish_character_immediate(character))
    {
        return;
    }

    if (!enqueue_output_character(character))
    {
        printf("publish_character: output queue full, flushing\n");
        send_partial_message();

        if (!enqueue_output_character(character))
        {
            printf("publish_character: character dropped\n");
            return;
        }
    }

    send_partial_msg = true;
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
        printf("onmessage: Invalid message parameters\n");
        return;
    }

    if (!enqueue_input_message(msg, (size_t)size))
    {
        return;
    }

    process_input_queue();
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

    dx_timerStart(&tmr_expire_session);

    atomic_store_explicit(&ws_input_ring.head, 0, memory_order_relaxed);
    atomic_store_explicit(&ws_input_ring.tail, 0, memory_order_relaxed);
    atomic_store_explicit(&ws_output_queue.head, 0, memory_order_relaxed);
    atomic_store_explicit(&ws_output_queue.tail, 0, memory_order_relaxed);

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
DX_TIMER_HANDLER(partial_message_handler)
{
    if (output_queue_has_data())
    {
        send_partial_msg = true;
    }
}
DX_TIMER_HANDLER_END
