/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "rgb_lcd.h"

// Device I2C Addresses
#define LCD_ADDRESS 0x3e
#define RGB_ADDRESS 0x62

static void textCommand(rgb_lcd_16x2_t *rgb_lcd_16x2, uint8_t cmd);
static void write_byte_data(rgb_lcd_16x2_t *rgb_lcd_16x2, uint8_t b0, uint8_t b1);

static int WriteI2CData(rgb_lcd_16x2_t *rgb_lcd_16x2, void *data, size_t len)
{
    int ret = -1;

    if (!rgb_lcd_16x2->initialized)
    {
        return -1;
    }

    ret = I2CMaster_Write(rgb_lcd_16x2->fd, LCD_ADDRESS, data, len);
    if (ret != len)
    {
        Log_Debug("%s: I2CMaster_Write: errno=%d (%s)\n", __func__, errno, strerror(errno));
        ret = -1;
    }

    return ret;
}

static void write_byte_data(rgb_lcd_16x2_t *rgb_lcd_16x2, uint8_t b0, uint8_t b1)
{
    uint8_t data[2] = {b0, b1};
    WriteI2CData(rgb_lcd_16x2, data, 2);
}

static void textCommand(rgb_lcd_16x2_t *rgb_lcd_16x2, uint8_t cmd)
{
    uint8_t data[2] = {0x80, cmd};
    WriteI2CData(rgb_lcd_16x2, data, 2);
}

bool RGBLCD_Init(rgb_lcd_16x2_t *rgb_lcd_16x2)
{
    if (rgb_lcd_16x2->initialized)
    {
        return true;
    }

    if (rgb_lcd_16x2->fd < 0)
    {
        return false;
    }

    rgb_lcd_16x2->initialized = true;

    textCommand(rgb_lcd_16x2, 0x01);
    delay(50);
    textCommand(rgb_lcd_16x2, 0x08 | 0x04); // display on, no cursor
    textCommand(rgb_lcd_16x2, 0x28);        // 2 lines
    delay(50);

    // ListI2CDevices(rgb_lcd_16x2->fd);

    return true;
}

void RGBLCD_SetColor(rgb_lcd_16x2_t *rgb_lcd_16x2, uint8_t r, uint8_t g, uint8_t b)
{
    write_byte_data(rgb_lcd_16x2, 0, 0);
    write_byte_data(rgb_lcd_16x2, 1, 0);
    write_byte_data(rgb_lcd_16x2, 0x08, 0xaa);
    write_byte_data(rgb_lcd_16x2, 4, r);
    write_byte_data(rgb_lcd_16x2, 3, g);
    write_byte_data(rgb_lcd_16x2, 2, b);
}

void RGBLCD_SetText(rgb_lcd_16x2_t *rgb_lcd_16x2, char *data)
{
    textCommand(rgb_lcd_16x2, 0x01);

    int count = 0;
    int row = 0;

    size_t len = strlen(data);
    uint8_t chr = 0x00;

    for (size_t x = 0; x < 32; x++)
    {
        if (x >= len)
            chr = 0x20;
        else
            chr = data[x];

        if (chr == '\n' || count == 16)
        {
            count = 0;
            row += 1;
            if (row == 2)
                break;

            textCommand(rgb_lcd_16x2, 0xc0);
        }
        else
        {
            write_byte_data(rgb_lcd_16x2, 0x40, chr);
        }
        count += 1;
    }
}