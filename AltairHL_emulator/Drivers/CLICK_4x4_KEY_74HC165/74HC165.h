/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <applibs/log.h>
#include <applibs/spi.h>
#include <stdbool.h>
#include <sys/types.h>
#include <time.h>

#define C4X4KEY_RETVAL uint8_t

#define C4X4KEY_OK 0x00
#define C4X4KEY_INIT_ERROR 0xFF

// Button value

#define C4X4KEY_BUTTON_0 0xEFFF
#define C4X4KEY_BUTTON_1 0xFFFE
#define C4X4KEY_BUTTON_2 0xFFFD
#define C4X4KEY_BUTTON_3 0xFFFB
#define C4X4KEY_BUTTON_4 0xFFEF
#define C4X4KEY_BUTTON_5 0xFFDF
#define C4X4KEY_BUTTON_6 0xFFBF
#define C4X4KEY_BUTTON_7 0xFEFF
#define C4X4KEY_BUTTON_8 0xFDFF
#define C4X4KEY_BUTTON_9 0xFBFF
#define C4X4KEY_BUTTON_A 0xFFF7
#define C4X4KEY_BUTTON_B 0xFF7F
#define C4X4KEY_BUTTON_C 0xF7FF
#define C4X4KEY_BUTTON_D 0xBFFF
#define C4X4KEY_BUTTON_STAR 0x7FFF
#define C4X4KEY_BUTTON_HASH 0xDFFF

// Max 16-bit

#define C4X4KEY_MAX_16_BIT 0xFFFF

typedef struct
{
    SPI_InterfaceId interfaceId;
    SPI_ChipSelectId chipSelectId;
    uint32_t busSpeed;
    int handle;
    uint16_t bitmap;
    uint8_t lastButtonPressed;
    int64_t lastButtonPressMilliseconds;
    int debouncePeriodMilliseconds;
    bool initialized;
} key4x4_t;

bool c4x4key_init(key4x4_t *key4x4);
uint8_t c4x4key_get_btn_position(key4x4_t *key4x4);
