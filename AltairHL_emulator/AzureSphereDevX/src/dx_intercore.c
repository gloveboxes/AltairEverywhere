/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "dx_intercore.h"

static void SocketEventHandler(EventLoop *el, int fd, EventLoop_IoEvents events, void *context);
static bool ProcessMsg(DX_INTERCORE_BINDING *intercore_binding);
static EventRegistration *socketEventReg = NULL;

static bool initialise_inter_core_communications(DX_INTERCORE_BINDING *intercore_binding)
{
    if (intercore_binding->initialized) // Already initialised
    {
        return true;
    }

    if (intercore_binding->rtAppComponentId == NULL) {
        dx_terminate(DX_ExitCode_MissingRealTimeComponentId);
        return false;
    }

    // Open connection to real-time capable application.
    intercore_binding->sockFd = Application_Connect(intercore_binding->rtAppComponentId);
    if (intercore_binding->sockFd == -1) {
        // Log_Debug("ERROR: Unable to create socket: %d (%s)\n", errno, strerror(errno));
        return false;
    }

    // Set timeout, to handle case where real-time capable application does not respond.
    const struct timeval recvTimeout = {.tv_sec = 5, .tv_usec = 0};
    int result = setsockopt(intercore_binding->sockFd, SOL_SOCKET, SO_RCVTIMEO, &recvTimeout,
                            sizeof(recvTimeout));
    if (result == -1) {
        Log_Debug("ERROR: Unable to set socket timeout: %d (%s)\n", errno, strerror(errno));
        return false;
    }

    if (intercore_binding->interCoreCallback != NULL) {
        // Register handler for incoming messages from real-time capable application.
        socketEventReg =
            EventLoop_RegisterIo(dx_timerGetEventLoop(), intercore_binding->sockFd, EventLoop_Input, SocketEventHandler, intercore_binding);
        if (socketEventReg == NULL) {
            Log_Debug("ERROR: Unable to register socket event: %d (%s)\n", errno, strerror(errno));
            return false;
        }
    }

    intercore_binding->initialized = true;
    return true;
}

bool dx_intercoreConnect(DX_INTERCORE_BINDING *intercore_binding)
{
    return initialise_inter_core_communications(intercore_binding);
}

/// <summary>
///     Nonblocking send intercore message
///		https://linux.die.net/man/2/send - Nonblocking = MSG_DONTWAIT.
/// </summary>
bool dx_intercorePublish(DX_INTERCORE_BINDING *intercore_binding, void *control_block,
                             size_t message_length)
{
    if (message_length > 1024) {
        Log_Debug("Message too long. Max length is 1024\n");
    }

    // lazy initialise intercore socket
    if (!initialise_inter_core_communications(intercore_binding)) {
        return false;
    };

    // https://linux.die.net/man/2/send - Nonblocking.
    // Returns EAGAIN if socket is full

    // send is blocking on socket full
    int bytesSent = send(intercore_binding->sockFd, control_block, message_length,
                         intercore_binding->nonblocking_io ? MSG_DONTWAIT : 0);
    if (bytesSent == -1) {
        Log_Debug("ERROR: Unable to send message: %d (%s)\n", errno, strerror(errno));
        return false;
    }

    return true;
}

bool dx_intercorePublishThenReadTimeout(DX_INTERCORE_BINDING *intercore_binding, suseconds_t timeoutInMicroseconds)
{
    if (!intercore_binding->initialized) // Not initialised
    {
        return false;
    }

    suseconds_t seconds = timeoutInMicroseconds / 1000000;
    timeoutInMicroseconds %= 1000000;

    const struct timeval recvTimeout = {.tv_sec = seconds, .tv_usec = timeoutInMicroseconds};

    if (setsockopt(intercore_binding->sockFd, SOL_SOCKET, SO_RCVTIMEO, &recvTimeout, sizeof(recvTimeout)) == -1) {
        Log_Debug("ERROR: Unable to set socket timeout: %d (%s)\n", errno, strerror(errno));
        return false;
    }
    return true;
}

ssize_t dx_intercorePublishThenRead(DX_INTERCORE_BINDING *intercore_binding, void *control_block, size_t message_length)
{
    if (dx_intercorePublish(intercore_binding, control_block, message_length)) {

        return recv(intercore_binding->sockFd, (void *)intercore_binding->intercore_recv_block,
                    intercore_binding->intercore_recv_block_length, 0);
    }
    return -1;
}

/// <summary>
///     Handle socket event by reading incoming data from real-time capable application.
/// </summary>
static void SocketEventHandler(EventLoop *el, int fd, EventLoop_IoEvents events, void *context)
{
    if (!ProcessMsg((DX_INTERCORE_BINDING *)context)) {
        dx_terminate(DX_ExitCode_InterCoreHandler);
    }
}

/// <summary>
///     Handle socket event by reading incoming data from real-time capable application.
/// </summary>
static bool ProcessMsg(DX_INTERCORE_BINDING *intercore_binding)
{
    if (intercore_binding->intercore_recv_block == NULL) {
        return false;
    }

    ssize_t bytesReceived = recv(intercore_binding->sockFd, (void *)intercore_binding->intercore_recv_block,
                                 intercore_binding->intercore_recv_block_length, 0);

    if (bytesReceived == -1) {
        dx_terminate(DX_ExitCode_InterCoreReceiveFailed);
        return false;
    }

    intercore_binding->interCoreCallback(intercore_binding->intercore_recv_block, (ssize_t)bytesReceived);

    return true;
}