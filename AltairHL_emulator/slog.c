/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "slog.h"
#include <stdbool.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> // for sleep()
#include "utils.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <stdarg.h>
#include "utilities.h"



#define PORT 1824

static int 	sock=-1;

static int yes = 1;
static struct sockaddr_in broadcast_addr;
struct sockaddr_in server_addr;
static int addr_len;
//static int count;
static int ret;
static char sock_buffer[2048];
//static int i;

static bool slogInit = false;

int initSlog(void);

char deviceGuid[40];

int Log_Debug(const char *fmt, ...)
{
	if (!dx_isNetworkReady())
	{
		return -1;
	}

	if (!slogInit)
	{
		initSlog();
	}

	memset(sock_buffer, 0x00, 2048);
	va_list args;
	va_start(args, fmt);

	uint32_t deviceHash = 0xffffffff;

	sock_buffer[3] = deviceHash & 0xff;	deviceHash /= 0x100;
	sock_buffer[2] = deviceHash & 0xff;	deviceHash /= 0x100;
	sock_buffer[1] = deviceHash & 0xff;	deviceHash /= 0x100;
	sock_buffer[0] = deviceHash & 0xff;

	vsnprintf(sock_buffer + 4, 2048, fmt, args);		// first four bytes is device Id Hash.

	va_end(args);

	ret = sendto(sock, sock_buffer, strlen(sock_buffer), 0, (struct sockaddr*) &broadcast_addr, addr_len);
	return ret;
}

int initSlog(void)
{
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		perror("sock error");
		return -1;
	}
	ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&yes, sizeof(yes));
	if (ret == -1) {
		perror("setsockopt error");
		return 0;
	}

	addr_len = sizeof(struct sockaddr_in);

	memset((void*)&broadcast_addr, 0, addr_len);
	broadcast_addr.sin_family = AF_INET;
	broadcast_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	broadcast_addr.sin_port = htons(PORT);

	slogInit = true;
}