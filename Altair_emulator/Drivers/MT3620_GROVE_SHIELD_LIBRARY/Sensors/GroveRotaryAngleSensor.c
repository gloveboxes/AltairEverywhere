#include <stdlib.h>

#include "GroveRotaryAngleSensor.h"
#include "GroveAD7992.h"
#include <applibs/gpio.h>

typedef struct
{
	void *inst;
	int pinId;
}
GroveRotaryAngleSensorInstance;

void* GroveRotaryAngleSensor_Init(int i2cFd, int analog_pin)
{
	GroveRotaryAngleSensorInstance* this = (GroveRotaryAngleSensorInstance*)malloc(sizeof(GroveRotaryAngleSensorInstance));
	this->inst = GroveAD7992_Open(i2cFd);
	this->pinId = analog_pin;

	return this;
}

float GroveRotaryAngleSensor_Read(void* inst)
{
	GroveRotaryAngleSensorInstance* this = (GroveRotaryAngleSensorInstance*)inst;
	return GroveAD7992_Read(this->inst, this->pinId);
}

