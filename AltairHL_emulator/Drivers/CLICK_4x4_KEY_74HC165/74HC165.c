/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "74HC165.h"

static int64_t get_now_milliseconds(void)
{
    struct timespec now = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

static bool c4x4key_get_data(key4x4_t *key4x4)
{
    uint8_t rx_buf[2];
    //uint16_t result;
    SPIMaster_Transfer transfers;

    if (!key4x4->initialized)
    {
        return false;
    }

    SPIMaster_InitTransfers(&transfers, 1);
    transfers.flags = SPI_TransferFlags_Read;
    transfers.length = 2;
    transfers.readData = rx_buf;

    if (SPIMaster_TransferSequential(key4x4->handle, &transfers, 1) != transfers.length)
    {
        // Log_Debug("SPI Read Failed");
        return false;
    }

    key4x4->bitmap = rx_buf[0];
    key4x4->bitmap <<= 8;
    key4x4->bitmap |= rx_buf[1];

    return true;
}

uint8_t c4x4key_get_btn_position(key4x4_t *key4x4)
{
    const uint8_t buttonMap[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 15, 16, 13};

    if (c4x4key_get_data(key4x4))
    {
        uint16_t result;
        uint8_t position = 0;

        result = (uint16_t)(C4X4KEY_MAX_16_BIT - key4x4->bitmap);

        while (result)
        {
            position++;
            result >>= 1;
        }

        int64_t now_milliseconds = get_now_milliseconds();

        if (position == 0 || (position == key4x4->lastButtonPressed && (now_milliseconds - key4x4->lastButtonPressMilliseconds) < key4x4->debouncePeriodMilliseconds))
        {
            return 0;
        }

        key4x4->lastButtonPressMilliseconds = now_milliseconds;

        
        // key 13, 14, 15, 16 need to be remapped
        position = buttonMap[position - 1];

        key4x4->lastButtonPressed = position;

        return position;
    }
    return 0;
}

bool c4x4key_init(key4x4_t *key4x4)
{
    if (key4x4->initialized)
    {
        return true;
    }

    SPIMaster_Config key4x4Config;

    SPIMaster_InitConfig(&key4x4Config);
    key4x4Config.csPolarity = SPI_ChipSelectPolarity_ActiveHigh;

    if ((key4x4->handle = SPIMaster_Open(key4x4->interfaceId, key4x4->chipSelectId, &key4x4Config)) == -1)
    {
        return false;
    };

    key4x4->initialized = true;

    SPIMaster_SetBusSpeed(key4x4->handle, key4x4->busSpeed);
    SPIMaster_SetBitOrder(key4x4->handle, SPI_BitOrder_MsbFirst);
    SPIMaster_SetMode(key4x4->handle, SPI_Mode_2);

    return true;
}