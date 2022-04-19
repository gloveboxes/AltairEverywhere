/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "web_socket_server.h"

void (*_client_connected_cb)(void);
static DX_DECLARE_TIMER_HANDLER(expire_session_handler);

static char output_buffer[512];
static ws_cli_conn_t *ws_ledger[MAX_CLIENTS];
static ws_cli_conn_t *current_ws   = NULL;
static size_t output_buffer_length = 0;
static bool cleanup_required       = false;

pthread_mutex_t session_mutex    = PTHREAD_MUTEX_INITIALIZER;
static bool active_session       = false;
static const int session_minutes = 1 * 60 * 30; // 30 minutes

static DX_TIMER_BINDING tmr_expire_session = {
    .name = "tmr_expire_session", .handler = expire_session_handler};

static DX_TIMER_HANDLER(expire_session_handler)
{
    active_session = false;
}
DX_TIMER_HANDLER_END

static void cleanup_session(void)
{
#ifdef ALTAIR_CLOUD
    cpu_operating_mode = CPU_STOPPED;

    // Sleep this thread so the Altair CPU thread can complete current instruction
    nanosleep(&(struct timespec){0, 250 * ONE_MS}, NULL);

    load_boot_disk();

    clear_difference_disk();
#endif

    cleanup_required = false;
}

void fd_ledger_init(void)
{
    for (int i = 0; i < NELEMS(ws_ledger); i++)
    {
        ws_ledger[i] = NULL;
    }
}

void fd_ledger_close_all(void)
{
    current_ws = NULL;
    active_session = false;

    for (int i = 0; i < NELEMS(ws_ledger); i++)
    {
        if (ws_ledger[i] != NULL)
        {
            ws_close_client(ws_ledger[i]);
            ws_ledger[i] = NULL;
        }
    }
}

void fd_ledger_add(ws_cli_conn_t *client)
{
    for (int i = 0; i < NELEMS(ws_ledger); i++)
    {
        if (ws_ledger[i] == NULL)
        {
            ws_ledger[i] = client;
            current_ws   = client;
            break;
        }
    }
}

void fd_ledger_delete(ws_cli_conn_t *client)
{
    for (int i = 0; i < NELEMS(ws_ledger); i++)
    {
        if (client == ws_ledger[i])
        {
            if (current_ws == ws_ledger[i])
            {
                active_session = false;
                current_ws = NULL;
            }
            ws_ledger[i] = NULL;
            break;
        }
    }
}

void publish_message(const void *message, size_t message_length)
{
    if (current_ws != NULL)
    {
        if (ws_sendframe(current_ws, message, message_length, WS_FR_OP_TXT) == -1)
        {
            fd_ledger_close_all();
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
    output_buffer[output_buffer_length++] = character;

    if (output_buffer_length < sizeof(output_buffer))
    {
        return;
    }

    publish_message(output_buffer, output_buffer_length);
    output_buffer_length = 0;
}

void onopen(ws_cli_conn_t *client)
{
    pthread_mutex_lock(&session_mutex);

    if (!active_session)
    {
        if (cleanup_required)
        {
            cleanup_session();
        }

        fd_ledger_close_all();
        fd_ledger_add(client);

#ifdef ALTAIR_CLOUD
        active_session   = true;
        cleanup_required = true;
        dx_timerOneShotSet(&tmr_expire_session, &(struct timespec){session_minutes, 0});
#endif
        (*(int *)dt_new_sessions.propertyValue)++;
        _client_connected_cb();
    }
    else
    {
        ws_close_client(client);
    }

    pthread_mutex_unlock(&session_mutex);
}

/**
 * @brief Called when a client disconnects to the server.
 *
 * @param fd File Descriptor belonging to the client. The @p fd parameter
 * is used in order to send messages and retrieve informations
 * about the client.
 */
void onclose(ws_cli_conn_t *client)
{
    // char *cli;
    // cli = ws_getaddress(fd);
    // printf("Connection closed, client: %d | addr: %s\n", fd, cli);
    // free(cli);

    fd_ledger_delete(client);
}

void onmessage(ws_cli_conn_t *client, const unsigned char *msg, uint64_t size, int type)
{
    // marshall the incoming message off the socket thread
    if (!ws_input_block.active)
    {
        ws_input_block.active = true;
        ws_input_block.length =
            size > sizeof(ws_input_block.buffer) ? sizeof(ws_input_block.buffer) : (size_t)size;
        memcpy(ws_input_block.buffer, msg, ws_input_block.length);

        dx_timerOneShotSet(&tmr_deferred_input, &(struct timespec){0, 100 * ONE_MS});
    }
}

void init_web_socket_server(void (*client_connected_cb)(void))
{
    _client_connected_cb = client_connected_cb;

    fd_ledger_init();

    dx_timerStart(&tmr_expire_session);

    struct ws_events evs;
    evs.onopen    = &onopen;
    evs.onclose   = &onclose;
    evs.onmessage = &onmessage;
    ws_socket(&evs, 8082, 1);
}

/// <summary>
/// Partial message check callback
/// </summary>
/// <param name="eventLoopTimer"></param>
DX_TIMER_HANDLER(partial_message_handler)
{
    if (output_buffer_length > 0)
    {
        send_partial_msg = true;
    }
    dx_timerOneShotSet(&tmr_partial_message, &(struct timespec){0, 250 * ONE_MS});
}
DX_TIMER_HANDLER_END