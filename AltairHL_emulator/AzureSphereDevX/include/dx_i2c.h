#pragma once

#include "dx_terminate.h"
#include <applibs/i2c.h>
#include <applibs/log.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    int fd;
    I2C_InterfaceId interfaceId;
    uint32_t speedInHz;
    char *name;
    bool opened;
} DX_I2C_BINDING;

bool dx_i2cClose(DX_I2C_BINDING *i2c_binding);
bool dx_i2cOpen(DX_I2C_BINDING *i2c_binding);
void dx_i2cSetClose(DX_I2C_BINDING **i2cSet, size_t i2cSetCount);
void dx_i2cSetOpen(DX_I2C_BINDING **i2cSet, size_t i2cSetCount);