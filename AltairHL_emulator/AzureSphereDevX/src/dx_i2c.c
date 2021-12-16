#include "dx_i2c.h"

bool dx_i2cOpen(DX_I2C_BINDING *i2c_binding)
{
    if (i2c_binding->opened) {
        return true;
    }

    i2c_binding->fd = I2CMaster_Open(i2c_binding->interfaceId);
    if (i2c_binding->fd < 0) {
        Log_Debug("ERROR: I2CMaster_Open: errno=%d (%s)\n", errno, strerror(errno));
        dx_terminate(DX_ExitCode_I2C_Open_Failed);
        return false;
    }

    int result = I2CMaster_SetBusSpeed(i2c_binding->fd, i2c_binding->speedInHz);
    if (result != 0) {
        Log_Debug("ERROR: I2CMaster_SetBusSpeed: errno=%d (%s)\n", errno, strerror(errno));
        dx_terminate(DX_ExitCode_I2C_SetBusSpeed_Failed);
        return false;
    }

    result = I2CMaster_SetTimeout(i2c_binding->fd, 100);
    if (result != 0) {
        Log_Debug("ERROR: I2CMaster_SetTimeout: errno=%d (%s)\n", errno, strerror(errno));
        dx_terminate(DX_ExitCode_I2C_SetTimeout_Failed);
        return false;
    }

    i2c_binding->opened = true;

    return true;
}

bool dx_i2cClose(DX_I2C_BINDING *i2c_binding)
{
    if (i2c_binding->opened) {

        i2c_binding->opened = false;

        if (close(i2c_binding->fd) != 0) {
            Log_Debug("ERROR: Could not close fd %s: %s (%d).\n", i2c_binding->name, strerror(errno), errno);
            return false;
        }
    }
    return true;
}

void dx_i2cSetOpen(DX_I2C_BINDING **i2cSet, size_t i2cSetCount) {
    for (int i = 0; i < i2cSetCount; i++) {
        if (!dx_i2cOpen(i2cSet[i])) {
            break;
        }
    }
}

void dx_i2cSetClose(DX_I2C_BINDING **i2cSet, size_t i2cSetCount) {
    for (int i = 0; i < i2cSetCount; i++) {
        dx_i2cClose(i2cSet[i]);
    }
}