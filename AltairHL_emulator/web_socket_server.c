/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "web_socket_server.h"

void (*_client_connected_cb)(void);

static bool ws_connected = false;
static char output_buffer[1024];
static int client_fd = -1;
static size_t output_buffer_length = 0;
static volatile bool dirty_buffer = false;

void publish_message(const void *application_message, size_t application_message_length)
{
    if (ws_connected) {
        ws_sendframe(client_fd, application_message, application_message_length, false, WS_FR_OP_TXT);
    }
}

void send_partial_message(void)
{
    if (output_buffer_length > 0) {
        publish_message(output_buffer, output_buffer_length);
        output_buffer_length = 0;
    }
    dirty_buffer = false;
}

inline void publish_character(char character)
{
    dirty_buffer = true;
    
    output_buffer[output_buffer_length++] = character;

    if (output_buffer_length < sizeof(output_buffer)) {
        return;
    }

    publish_message(output_buffer, output_buffer_length);
    output_buffer_length = 0;
    dirty_buffer = false;
}

void onopen(int fd)
{
    if (ws_connected){
        ws_close_client(client_fd);
    }

    client_fd = fd;
    ws_connected = true;

    char *cli;
    cli = ws_getaddress(fd);
#ifndef DISABLE_VERBOSE
    printf("Connection opened, client: %d | addr: %s\n", fd, cli);
#endif
    free(cli);

    _client_connected_cb();
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
    char *cli;
    cli = ws_getaddress(fd);
#ifndef DISABLE_VERBOSE
    printf("Connection closed, client: %d | addr: %s\n", fd, cli);
#endif
    free(cli);
    ws_connected = false;
}

void onmessage(int fd, const unsigned char *msg, uint64_t size, int type)
{
    // marshall the incoming message off the socket thread
    if (!ws_input_block.active) {
        ws_input_block.active = true;
        ws_input_block.length = size > sizeof(ws_input_block.buffer) ? sizeof(ws_input_block.buffer) : size;
        memcpy(ws_input_block.buffer, msg, ws_input_block.length);

        dx_timerOneShotSet(&tmr_deferred_input, &(struct timespec){0, 1});
    }
}

void init_web_socket_server(void (*client_connected_cb)(void))
{
    _client_connected_cb = client_connected_cb;

    struct ws_events evs;
    evs.onopen = &onopen;
    evs.onclose = &onclose;
    evs.onmessage = &onmessage;
    ws_socket(&evs, 8082, 1); /* Never returns. */
}

/// <summary>
/// Partial message check callback
/// </summary>
/// <param name="eventLoopTimer"></param>
DX_TIMER_HANDLER(partial_message_handler)
{
    if (dirty_buffer) {
        send_partial_msg = true;
    }
}
DX_TIMER_HANDLER_END