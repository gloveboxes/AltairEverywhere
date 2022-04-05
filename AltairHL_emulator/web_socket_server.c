/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "web_socket_server.h"

void (*_client_connected_cb)(void);
static DX_DECLARE_TIMER_HANDLER(expire_session_handler);

static char output_buffer[512];
static int fd_ledger[MAX_CLIENTS];
static int client_fd               = -1;
static size_t output_buffer_length = 0;
static bool cleanup_required       = false;

pthread_mutex_t session_mutex       = PTHREAD_MUTEX_INITIALIZER;
static bool active_session = false;
static const int session_minutes    = 1 * 60 * 30; // 30 minutes

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
    for (int i = 0; i < NELEMS(fd_ledger); i++)
    {
        fd_ledger[i] = -1;
    }
}

void fd_ledger_close_all(void)
{
    client_fd = -1;

    for (int i = 0; i < NELEMS(fd_ledger); i++)
    {
        if (fd_ledger[i] != -1)
        {
            ws_close_client(fd_ledger[i]);
            fd_ledger[i] = -1;
        }
    }
}

void fd_ledger_add(int fd)
{
    for (int i = 0; i < NELEMS(fd_ledger); i++)
    {
        if (fd_ledger[i] == -1)
        {
            fd_ledger[i] = fd;
            client_fd    = fd;
            break;
        }
    }
}

void fd_ledger_delete(int fd)
{
    for (int i = 0; i < NELEMS(fd_ledger); i++)
    {
        if (fd == fd_ledger[i])
        {
            if (client_fd == fd_ledger[i])
            {
                active_session = false;
            }
            fd_ledger[i] = -1;
            break;
        }
    }
}

void publish_message(const void *message, size_t message_length)
{
    if (client_fd != -1)
    {
        if (ws_sendframe(client_fd, message, message_length, false, WS_FR_OP_TXT) == -1)
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

void onopen(int fd)
{
    pthread_mutex_lock(&session_mutex);

    if (!active_session)
    {
        if (cleanup_required)
        {
            cleanup_session();
        }

        fd_ledger_close_all();
        fd_ledger_add(fd);

#ifdef ALTAIR_CLOUD
        active_session = true;
        cleanup_required = true;
        dx_timerOneShotSet(&tmr_expire_session, &(struct timespec){session_minutes, 0});
#endif
        (*(int *)dt_new_sessions.propertyValue)++;
        _client_connected_cb();
    }
    else
    {
        ws_close_client(fd);
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
void onclose(int fd)
{
    // char *cli;
    // cli = ws_getaddress(fd);
    // printf("Connection closed, client: %d | addr: %s\n", fd, cli);
    // free(cli);

    fd_ledger_delete(fd);
}

void onmessage(int fd, const unsigned char *msg, uint64_t size, int type)
{
    // marshall the incoming message off the socket thread
    if (!ws_input_block.active)
    {
        ws_input_block.active = true;
        ws_input_block.length = size > sizeof(ws_input_block.buffer) ? sizeof(ws_input_block.buffer) : (size_t)size;
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