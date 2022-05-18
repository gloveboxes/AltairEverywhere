/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "web_socket_server.h"

static DX_DECLARE_TIMER_HANDLER(expire_session_handler);
static void (*_client_connected_cb)(void);
static void cleanup_session(void);

static char output_buffer[512];
static WS_INPUT_BLOCK_T ws_input_block;

static DX_TIMER_BINDING tmr_expire_session = {
	.name = "tmr_expire_session", .handler = expire_session_handler};
static bool cleanup_required                  = false;
static const int session_minutes              = 1 * 60 * 30; // 30 minutes
static int session_count                      = 0;
static volatile uint32_t output_buffer_length = 0;
ws_cli_conn_t *current_client                 = NULL;

#ifdef ALTAIR_CLOUD
static struct timeval ws_timeout = {0, 250 * 1000};
#endif

static DX_TIMER_HANDLER(expire_session_handler)
{
	if (session_count > 0 && current_client)
	{
		ws_close_client(current_client);
	}
	cleanup_session();
}
DX_TIMER_HANDLER_END

DX_TIMER_HANDLER(ws_ping_pong_handler)
{
	if (session_count > 0)
	{
		ws_ping(NULL, 2);
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
	cpu_operating_mode = CPU_STOPPED;

	// Sleep this thread so the Altair CPU thread can complete current instruction
	nanosleep(&(struct timespec){0, 250 * ONE_MS}, NULL);

	load_boot_disk();
	clear_difference_disk();
#endif
	cleanup_required = false;
}

void publish_message(const void *message, size_t message_length)
{
	if (session_count > 0)
	{
		if (ws_sendframe(NULL, message, message_length, WS_FR_OP_TXT) == -1)
		{
			dx_Log_Debug("ws_sendframe failed\n");
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
	printf("New session\n");
	session_count++;
	current_client = client;

	if (cleanup_required)
	{
		cleanup_session();
	}

#ifdef ALTAIR_CLOUD
	cleanup_required = true;
	dx_asyncSend(&async_expire_session, NULL);
#endif

	(*(int *)dt_new_sessions.propertyValue)++;
	_client_connected_cb();
}

void onclose(ws_cli_conn_t *client)
{
	printf("Session closed\n");
	session_count--;
	current_client = NULL;

	if (cleanup_required)
	{
		cleanup_session();
	}
}

void onmessage(ws_cli_conn_t *client, const unsigned char *msg, uint64_t size, int type)
{
	size_t len = 0;

	pthread_mutex_lock(&ws_input_block.block_lock);

	len = (size_t)size > sizeof(ws_input_block.buffer) - ws_input_block.length
			  ? sizeof(ws_input_block.buffer) - ws_input_block.length
			  : (size_t)size;

	memcpy(ws_input_block.buffer + ws_input_block.length, msg, len);

	ws_input_block.length += len;

	dx_asyncSend(&async_terminal, (void *)&ws_input_block);

	pthread_mutex_unlock(&ws_input_block.block_lock);
}

void init_web_socket_server(void (*client_connected_cb)(void))
{
	_client_connected_cb = client_connected_cb;

	dx_timerStart(&tmr_expire_session);

	if (pthread_mutex_init(&ws_input_block.block_lock, NULL) != 0)
	{
		printf("mutex_lock");
		exit(1);
	}

	ws_input_block.length = 0;

	struct ws_events evs;
	evs.onopen    = &onopen;
	evs.onclose   = &onclose;
	evs.onmessage = &onmessage;
	ws_socket(&evs, 8082, 1, 250);
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