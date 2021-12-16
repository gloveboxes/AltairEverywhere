/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "applibs/spi.h"
#include "applibs/log.h"
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

typedef struct {
    int fd;
    bool initialized;
    SPI_InterfaceId interfaceId;
    SPI_ChipSelectId chipSelectId;
} eeprom2_t;

bool eeprom2_init(eeprom2_t * eeprom2);
int eeprom2_read(eeprom2_t * eeprom2, uint32_t memory_address, uint8_t *value);
int eeprom2_memory_enable(eeprom2_t * eeprom2);
int eeprom2_read_bytes(eeprom2_t * eeprom2, uint32_t memory_address, uint8_t *value, uint8_t count);
int eeprom2_write(eeprom2_t * eeprom2, uint32_t memory_address, uint8_t value);
int eeprom2_write_bytes(eeprom2_t * eeprom2, uint32_t memory_address, uint8_t *value, uint8_t count);