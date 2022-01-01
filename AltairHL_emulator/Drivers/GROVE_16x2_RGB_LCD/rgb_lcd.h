/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "errno.h"
#include "time.h"
#include "utils.h"
#include <applibs/i2c.h>
#include <applibs/log.h>
#include <inttypes.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

typedef struct{
    int fd;
    bool initialized;
} rgb_lcd_16x2_t;

bool RGBLCD_Init(rgb_lcd_16x2_t* rgb_lcd_16x2);
void RGBLCD_SetColor(rgb_lcd_16x2_t *rgb_lcd_16x2, uint8_t r, uint8_t g, uint8_t b);
void RGBLCD_SetText(rgb_lcd_16x2_t *rgb_lcd_16x2, char *data);
void RGBLCD_Clear(rgb_lcd_16x2_t *rgb_lcd_16x2);