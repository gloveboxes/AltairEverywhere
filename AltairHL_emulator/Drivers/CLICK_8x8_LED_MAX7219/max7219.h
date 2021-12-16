/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <applibs/log.h>
#include <applibs/spi.h>
#include <stdbool.h>

#define MAX7219_REG_DECODE 0x09		  // "decode mode" register
#define MAX7219_REG_INTENSITY 0x0a	  // "intensity" register
#define MAX7219_REG_SCAN_LIMIT 0x0b	  // "scan limit" register
#define MAX7219_REG_SHUTDOWN 0x0c	  // "shutdown" register
#define MAX7219_REG_DISPLAY_TEST 0x0f // "display test" register

#define MAX7219_INTENSITY_MIN 0x00 // minimum display intensity
#define MAX7219_INTENSITY_MAX 0x0f // maximum display intensity

#define SCAN_LIMIT 7

typedef struct
{
    SPI_InterfaceId interfaceId;
    SPI_ChipSelectId chipSelectId;
    uint32_t busSpeed;
    int handle;
    bool initialized;
    union
    {
        unsigned char bitmap[8];
        uint64_t bitmap64;
    };

} max7219_t;

void max7219_clear(max7219_t *panel8x8);
void max7219_display_test(max7219_t *panel8x8, bool state);
bool max7219_init(max7219_t *panel8x8, unsigned char intialBrightness);
void max7219_panel_clear(max7219_t *panel8x8);
void max7219_panel_write(max7219_t *panel8x8);
void max7219_set_brightness(max7219_t *panel8x8, unsigned char brightness);
void max7219_write(max7219_t *panel8x8, unsigned char reg_number, unsigned char dataout);
