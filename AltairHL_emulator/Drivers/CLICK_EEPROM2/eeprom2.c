/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "eeprom2.h"

// https://github.com/MikroElektronika/mikrosdk_click_v2/blob/57e012d58e966f394a92cadf2551bbca4070e4bd/clicks/eeprom2/lib/src/eeprom2.c

static int64_t last_write_time = UINT64_MAX;

static int64_t dx_getNowMilliseconds(void)
{
    struct timespec now = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

bool eeprom2_init(eeprom2_t * eeprom2)
{
    if (eeprom2->initialized){
        return true;
    }

    SPIMaster_Config eeprom2_config;

    SPIMaster_InitConfig(&eeprom2_config);
    eeprom2_config.csPolarity = SPI_ChipSelectPolarity_ActiveLow;

    if ((eeprom2->fd = SPIMaster_Open(eeprom2->interfaceId, eeprom2->chipSelectId, &eeprom2_config)) == -1)
    {
        Log_Debug("Failed to open EEPROM2 on SPI interface :%d\n", eeprom2->interfaceId);
        return false;
    };
    
    eeprom2->initialized = true;

    SPIMaster_SetBusSpeed(eeprom2->fd, 1000000);
    SPIMaster_SetBitOrder(eeprom2->fd, SPI_BitOrder_MsbFirst);
    SPIMaster_SetMode(eeprom2->fd, SPI_Mode_0);

    return true;
}

int eeprom2_memory_enable(eeprom2_t * eeprom2)
{
    if (!eeprom2->initialized)
    {
        return -1;
    }

    SPIMaster_Transfer transfer;

    uint8_t temp;
    temp = 0x06;

    SPIMaster_InitTransfers(&transfer, 1);
    transfer.flags = SPI_TransferFlags_Write;
    transfer.length = 1;
    transfer.writeData = &temp;
    transfer.readData = NULL;

    if (SPIMaster_TransferSequential(eeprom2->fd, &transfer, 1) != 1)
    {
        Log_Debug("SPI Write Failed");
        return -1;
    }

    return 0;
}

int eeprom2_write_bytes(eeprom2_t * eeprom2, uint32_t memory_address, uint8_t *value, uint8_t count)
{
    if (!eeprom2->initialized)
    {
        return -1;
    }

    if (count == 0)
    {
        return 0;
    }

    SPIMaster_Transfer transfer[2];

    uint8_t tx_buf[4];

    eeprom2_memory_enable(eeprom2);

    tx_buf[0] = 0x02;
    tx_buf[1] = (uint8_t)((memory_address >> 16) & 0x000000FF);
    tx_buf[2] = (uint8_t)((memory_address >> 8) & 0x000000FF);
    tx_buf[3] = (uint8_t)(memory_address & 0x000000FF);

    SPIMaster_InitTransfers(transfer, 2);
    transfer[0].flags = SPI_TransferFlags_Write;
    transfer[0].length = 4;
    transfer[0].writeData = tx_buf;
    transfer[0].readData = NULL;

    transfer[1].flags = SPI_TransferFlags_Write;
    transfer[1].length = count;
    transfer[1].writeData = value;
    transfer[1].readData = NULL;

    if (SPIMaster_TransferSequential(eeprom2->fd, transfer, 2) != transfer[0].length + transfer[1].length)
    {
        Log_Debug("SPI Write Failed");
        return -1;
    }

    last_write_time = dx_getNowMilliseconds();

    return count;
}

int eeprom2_write(eeprom2_t * eeprom2, uint32_t memory_address, uint8_t value)
{
    return eeprom2_write_bytes(eeprom2, memory_address, &value, 1);
}

int eeprom2_read_bytes(eeprom2_t * eeprom2, uint32_t memory_address, uint8_t *value, uint8_t count)
{
    if (!eeprom2->initialized)
    {
        return -1;
    }

    if (count == 0)
    {
        return 0;
    }

    uint8_t tx_buf[4];
    int64_t time_diff = dx_getNowMilliseconds() - last_write_time;

    if (time_diff < 100)
    {
        int64_t delay = 100 - time_diff;
        nanosleep(&(struct timespec){0, delay * 1000000}, NULL);
    }

    eeprom2_memory_enable(eeprom2);

    tx_buf[0] = 0x03;
    tx_buf[1] = (uint8_t)((memory_address >> 16) & 0x000000FF);
    tx_buf[2] = (uint8_t)((memory_address >> 8) & 0x000000FF);
    tx_buf[3] = (uint8_t)(memory_address & 0x000000FF);

    if (SPIMaster_WriteThenRead(eeprom2->fd, tx_buf, 4, value, count) != 4 + count)
    {
        return -1;
    }

    return count;
}

int eeprom2_read(eeprom2_t * eeprom2, uint32_t memory_address, uint8_t *value)
{
    return eeprom2_read_bytes(eeprom2, memory_address, value, 1);
}