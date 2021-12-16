/* Copyright (c) Avnet Inc. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "parson.h"
#include <applibs/uart.h>
#include <applibs/log.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "dx_terminate.h"
#include "dx_timer.h"

typedef struct _uartBinding {
    int fd;
    int uart;
    char *name;
    bool opened;
    UART_Config uartConfig;
    void (*handler)(struct _uartBinding *uartBinding);
} DX_UART_BINDING;

bool dx_uartOpen(DX_UART_BINDING *uart);
void dx_uartClose(DX_UART_BINDING *uart);
void dx_uartSetClose(DX_UART_BINDING **uartSet, size_t uartSetCount);
void dx_uartSetOpen(DX_UART_BINDING **uartSet, size_t uartSetCount);
int dx_uartWrite(DX_UART_BINDING *uart_binding, char *writeBuffer, size_t dataLength);
int dx_uartRead(DX_UART_BINDING *uart_binding, char *rxBuffer, size_t bufferSize);