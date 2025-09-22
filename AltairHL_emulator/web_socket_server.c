/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "web_socket_server.h"

static DX_DECLARE_TIMER_HANDLER(expire_session_handler);
static void (*_client_connected_cb)(void);
static void cleanup_session(void);

static char output_buffer[512];
static WS_INPUT_BLOCK_T ws_input_block;

static DX_TIMER_BINDING tmr_expire_session    = {.name = "tmr_expire_session", .handler = expire_session_handler};
static bool cleanup_required                  = false;
static const int session_minutes              = 1 * 60 * 30; // 30 minutes
static volatile uint32_t output_buffer_length = 0;
ws_cli_conn_t current_client                  = 0;

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

void publish_message(const void *message, size_t message_length)
{
    // Validate input parameters
    if (message == NULL || message_length == 0)
    {
        dx_Log_Debug("publish_message: Invalid message parameters\n");
        return;
    }

    if (current_client != 0)
    {
        if (ws_sendframe(current_client, message, message_length, WS_FR_OP_TXT) == -1)
        {
            dx_Log_Debug("ws_sendframe failed - connection may be broken\n");
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
    publish_message(output_buffer, output_buffer_length);
    output_buffer_length = 0;
}

inline void publish_character(char character)
{
    // Ensure we don't overflow the output buffer
    if (output_buffer_length >= sizeof(output_buffer))
    {
        dx_Log_Debug("Output buffer full, sending partial message\n");
        publish_message(output_buffer, output_buffer_length);
        output_buffer_length = 0;
    }

    output_buffer[output_buffer_length++] = character;

    if (output_buffer_length >= sizeof(output_buffer))
    {
        publish_message(output_buffer, output_buffer_length);
        output_buffer_length = 0;
    }
}

void onopen(ws_cli_conn_t client)
{
    // Validate client connection
    if (client == 0)
    {
        dx_Log_Debug("onopen: Invalid client connection\n");
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
    dx_Log_Debug("New WebSocket session established\n");

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
        dx_Log_Debug("onclose: Closing client does not match current client\n");
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
    size_t len = 0;

    // Validate input parameters
    if (msg == NULL || size == 0)
    {
        dx_Log_Debug("onmessage: Invalid message parameters\n");
        return;
    }

    if (pthread_mutex_lock(&ws_input_block.block_lock) != 0)
    {
        dx_Log_Debug("onmessage: Failed to acquire input mutex\n");
        return;
    }

    // Calculate safe length to copy, ensuring we don't overflow
    size_t available_space = sizeof(ws_input_block.buffer) - ws_input_block.length;
    len = (size_t)size > available_space ? available_space : (size_t)size;

    if (len > 0)
    {
        memcpy(ws_input_block.buffer + ws_input_block.length, msg, len);
        ws_input_block.length += len;
    }
    else
    {
        dx_Log_Debug("onmessage: Input buffer full, dropping message\n");
    }

    pthread_mutex_unlock(&ws_input_block.block_lock);

    if (len > 0)
    {
        terminal_handler(&ws_input_block);
    }
}

void init_web_socket_server(void (*client_connected_cb)(void))
{
    // Validate input parameter
    if (client_connected_cb == NULL)
    {
        dx_Log_Debug("init_web_socket_server: client_connected_cb callback is NULL\n");
        return;
    }

    _client_connected_cb = client_connected_cb;

    dx_timerStart(&tmr_expire_session);

    if (pthread_mutex_init(&ws_input_block.block_lock, NULL) != 0)
    {
        dx_Log_Debug("Failed to initialize WebSocket input mutex\n");
        // Continue without mutex - not ideal but better than crashing
        // TODO: Consider returning error status from this function
    }

    ws_input_block.length = 0;

    struct ws_server ws_srv = {
        .host = NULL,  // NULL means bind to all interfaces
        .port = 8082,
        .thread_loop = 1,
        .timeout_ms = 250,
        .evs = {
            .onopen = &onopen,
            .onclose = &onclose,
            .onmessage = &onmessage
        },
        .context = NULL
    };
    
    ws_socket(&ws_srv);
}

/// <summary>
/// Partial message check callback
/// </summary>
/// <param name="eventLoopTimer"></param>
DX_TIMER_HANDLER(partial_message_handler)
{
    if (output_buffer_length)
    {
        send_partial_msg = true;
    }
}
DX_TIMER_HANDLER_END