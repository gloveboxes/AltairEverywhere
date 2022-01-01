/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "utils.h"
#include <applibs/log.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <applibs/i2c.h>

void delay(int ms)
{
	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000;
	nanosleep(&ts, NULL);
}

void delayMicroseconds(int us)
{
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = us * 1000;
	nanosleep(&ts, NULL);
}

void ListI2CDevices(int isuFd)
{
	uint8_t data = 0x00;	// I2C OLED Display 'turn off'
	I2C_DeviceAddress _devAddr = 0x00;

	bool deviceFound = false;

	Log_Debug("-----------------------------------------\n");

	for (unsigned int x = 0; x <= 0x7f; x++)
	{
		_devAddr = x;
		ssize_t ret = I2CMaster_Write(isuFd, _devAddr, &data, 1);
		if (ret != -1)
		{
			deviceFound = true;
			Log_Debug("Found address: 0x%02x\n", x);
		}
	}

	Log_Debug("\n");

	if (!deviceFound)
	{
		Log_Debug("Didn't find any I2C devices\n");
	}

	Log_Debug("\n");
}