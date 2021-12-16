/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "as1115.h"

static bool initialized = false;

static int64_t get_now_milliseconds(void)
{
    struct timespec now = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

static bool c4x4key_get_data(as1115_t *retro_click)
{
    uint8_t buf[2];
    if (!initialized)
    {
        return false;
    }

    static const uint8_t register_address = KEYA_r;
    unsigned char temp;

    if (I2CMaster_WriteThenRead(retro_click->handle, AS1115_I2C_ADDRESS, &register_address, sizeof(register_address), (uint8_t *)buf, sizeof(buf)) != 3)
    {
        Log_Debug("AS1115 Key read failed\n");
        return false;
    }

    // rotate result left by 1 bit
    temp = buf[0];
    buf[0] = (uint8_t)(buf[0] << 1 | (uint8_t)(temp >> 7));

    // rotate result left by 1 bit
    temp = buf[1];
    buf[1] = (uint8_t)(buf[1] << 1 | (uint8_t)(temp >> 7));

    retro_click->keymap = (uint16_t) ~(buf[0] << 8 | (uint16_t)buf[1]);

    return true;
}

uint8_t as1115_get_btn_position(as1115_t *retro_click)
{
    uint8_t position = 0;

    if (c4x4key_get_data(retro_click))
    {
        while (retro_click->keymap)
        {
            position++;
            retro_click->keymap >>= 1;
        }

        // reverse the button numbers so the top left is button 1, bottom right is 16
        if (position > 0)
        {
            position = (uint8_t)(17 - position);
        }

        int64_t now_milliseconds = get_now_milliseconds();

        if (position == 0 || (position == retro_click->lastButtonPressed && (now_milliseconds - retro_click->lastButtonPressMilliseconds) < retro_click->debouncePeriodMilliseconds))
        {
            return 0;
        }

        retro_click->lastButtonPressMilliseconds = now_milliseconds;
        retro_click->lastButtonPressed = position;
    }

    return position;
}

void as1115_write(as1115_t *retro_click, unsigned char reg_number, unsigned char dataout)
{
    if (!initialized)
    {
        return;
    }

    unsigned char data[2];
    data[0] = reg_number;
    data[1] = dataout;

    if (I2CMaster_Write(retro_click->handle, AS1115_I2C_ADDRESS, (const uint8_t *)data, sizeof(data)) != 2)
    {
        Log_Debug("I2C Write Failed\n");
    }
}

void as1115_set_brightness(as1115_t *retro_click, unsigned char brightness)
{
    brightness &= 0x0f; // mask off extra bits
    as1115_write(retro_click, AS1115_REG_INTENSITY, brightness);
}

void as1115_display_test(as1115_t *retro_click, bool state)
{
    // put AS1115 into "display test" mode
    as1115_write(retro_click, AS1115_REG_DISPLAY_TEST, state ? 1 : 0);
}

void as1115_clear(as1115_t *retro_click)
{
    for (char i = 1; i < 9; i++)
    {
        // turn all segments off
        as1115_write(retro_click, i, 0x00);
    }
}

void as1115_panel_write(as1115_t *retro_click)
{
    for (unsigned char i = 0; i < sizeof(retro_click->bitmap); i++)
    {

        // Work around for hardware issue - column addressing is good
        // Row address needs to be rotated as
        // bit 2 was displayed at led 1
        // bit 1 was displayed at led 8.
        unsigned char temp = retro_click->bitmap[i];
        retro_click->bitmap[i] = temp >> 1;
        retro_click->bitmap[i] = retro_click->bitmap[i] | (unsigned char)(temp << 7);

        as1115_write(retro_click, (unsigned char)(i + 1), retro_click->bitmap[i]);
    }
}

void as1115_panel_clear(as1115_t *retro_click)
{
    for (size_t i = 0; i < sizeof(retro_click->bitmap); i++)
    {
        retro_click->bitmap[i] = 0;
    }

    as1115_panel_write(retro_click);
}

bool as1115_init(int i2c_fd, as1115_t *retro_click, unsigned char intialBrightness)
{
    if (i2c_fd == -1)
    {
        Log_Debug("ERROR: I2CMaster_Open: errno=%d (%s)\n", errno, strerror(errno));
        return false;
    }

    initialized = true;

    retro_click->handle = i2c_fd;

    as1115_write(retro_click, AS1115_REG_SCAN_LIMIT, SCAN_LIMIT); // set up to scan all eight digits
    as1115_write(retro_click, AS1115_REG_DECODE, 0x00);           // set to "no decode" for all digits
    as1115_write(retro_click, AS1115_REG_SHUTDOWN, 1);            // put AS1115 into "normal" mode

    as1115_set_brightness(retro_click, intialBrightness);

    return true;
}