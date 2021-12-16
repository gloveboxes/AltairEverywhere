/* Copyright (c) Avnet Inc. All rights reserved.
   Licensed under the MIT License. */

#include "dx_uart.h"

// Forward declarations
static void UartEventHandler(EventLoop *el, int fd, EventLoop_IoEvents events, void *context);
static bool ProcessUartEvent(DX_UART_BINDING *uart_binding);

static EventRegistration *uartEventReg = NULL;

void dx_uartSetOpen(DX_UART_BINDING **uartSet, size_t uartSetCount)
{
    for (int i = 0; i < uartSetCount; i++) {
        if (!dx_uartOpen(uartSet[i])) {
            break;
        }
    }
}

bool dx_uartOpen(DX_UART_BINDING *uart_binding)
{
    if (uart_binding == NULL || uart_binding->uart < 0) {
        return false;
    }

    if (uart_binding->opened) {
        return true;
    }

    UART_InitConfig(&uart_binding->uartConfig);
    uart_binding->fd = UART_Open(uart_binding->uart, &uart_binding->uartConfig);
    if (uart_binding->fd < 0) {
        Log_Debug(
            "Error opening UART: %s (%d). Check that app_manifest.json includes the UART "
            "used.\n",
            strerror(errno), errno);
        dx_terminate(DX_ExitCode_Uart_Open_Failed);
        return false;
    }

    // Register handler for incoming messages from this uart
    uartEventReg = EventLoop_RegisterIo(dx_timerGetEventLoop(), uart_binding->fd, EventLoop_Input, UartEventHandler, uart_binding);
    if (uartEventReg == NULL) {
        Log_Debug("ERROR: Unable to register uart event: %d (%s)\n", errno, strerror(errno));
        return false;
    }

    uart_binding->opened = true;
    return true;
}

/// <summary>
///     Handle uart event by reading incoming data from the uart.
/// </summary>
static void UartEventHandler(EventLoop *el, int fd, EventLoop_IoEvents events, void *context)
{
    if (!ProcessUartEvent((DX_UART_BINDING *)context)) {
        dx_terminate(DX_ExitCode_UartHandler);
    }
}

/// <summary>
///     Handle uart event by passing control to the uart binding handler
/// </summary>
static bool ProcessUartEvent(DX_UART_BINDING *uart_binding)
{
    if (uart_binding->handler == NULL) {
        return false;
    } else {
        // call the binding specific handler to read and process the data
        uart_binding->handler(uart_binding);
        return true;
    }
}

void dx_uartSetClose(DX_UART_BINDING **uartSet, size_t uartSetCount)
{
    for (int i = 0; i < uartSetCount; i++) {
        dx_uartClose(uartSet[i]);
    }
}

void dx_uartClose(DX_UART_BINDING *uart_binding)
{
    if (uart_binding->opened && (uart_binding->fd >= 0)) {
        int result = close(uart_binding->fd);
        if (result != 0) {
            Log_Debug("ERROR: Could not close uart %s: %s (%d).\n", uart_binding->name == NULL ? "No name" : uart_binding->name,
                      strerror(errno), errno);
        }
    }
    uart_binding->fd = -1;
    uart_binding->opened = false;
}

int dx_uartWrite(DX_UART_BINDING *uart_binding, char *writeBuffer, size_t dataLength)
{
    if (!uart_binding->opened) {
        return -1;
    }

    return write(uart_binding->fd, writeBuffer, dataLength);
}

int dx_uartRead(DX_UART_BINDING *uart_binding, char *rxBuffer, size_t bufferSize)
{
    if (!uart_binding->opened) {
        return -1;
    }

    return read(uart_binding->fd, rxBuffer, bufferSize);
}