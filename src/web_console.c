/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "web_console.h"

#include <stdint.h>
#include <string.h>
#include <time.h>

static DX_DECLARE_TIMER_HANDLER(expire_session_handler);
static void (*_client_connected_cb)(void);
static void cleanup_session(void);
static void publish_message_direct(const char *message, size_t message_length);

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
    publish_message_direct(&character, 1);
}

void publish_message(const void *message, size_t message_length)
{
    if (message == NULL || message_length == 0)
    {
        return;
    }

    publish_message_direct((const char *)message, message_length);
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
