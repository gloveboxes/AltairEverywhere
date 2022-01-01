#include "GroveAD7992.h"
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "../HAL/GroveI2C.h"

#include <applibs/gpio.h>

#define AD7992_ADDRESS					(0x20 << 1)

#define AD7992_REG_CONVERSION_RESULT	(0x0)
#define AD7992_REG_CONFIGURATION		(0x2)

#define CONVST_PIN   58
#define ALART_PIN    57

#define REF_VOL  3300

typedef struct
{
	int I2cFd;
	int ConvstFd;
	int AlertFd;
}
GroveAD7992Instance;

void* GroveAD7992_Open(int i2cFd)
{
	GroveAD7992Instance* this = (GroveAD7992Instance*)malloc(sizeof(GroveAD7992Instance));

	this->I2cFd = i2cFd;
	this->ConvstFd = GPIO_OpenAsOutput(CONVST_PIN, GPIO_OutputMode_PushPull, GPIO_Value_High);
	this->AlertFd = GPIO_OpenAsInput(ALART_PIN);

	return this;
}

float GroveAD7992_Read(void* inst, int channel)
{
	GroveAD7992Instance* this = (GroveAD7992Instance*)inst;

	// Select channel
	GroveI2C_WriteReg8(this->I2cFd, AD7992_ADDRESS, AD7992_REG_CONFIGURATION, (channel == 0 ? 0x10 : 0x20) | 0x08);

	// Start conversion
	GPIO_SetValue(this->ConvstFd, GPIO_Value_Low);

	// Wait for converted
	const struct timespec t_convert = { 0, 2000 };
	nanosleep(&t_convert, NULL);

	// Read value
	uint16_t val;
	GroveI2C_ReadReg16(this->I2cFd, AD7992_ADDRESS, AD7992_REG_CONVERSION_RESULT, &val);

	// Stop conversion
	GPIO_SetValue(this->ConvstFd, GPIO_Value_High);

	val = (uint16_t)((val & 0x00ff) << 8 | (val & 0xff00) >> 8);
	val &= 0x0fff;

	return (float)val / 0x0fff;
}

float GroveAD7992_ConvertToMillisVolt(float value)
{
	return (REF_VOL * value);
}

