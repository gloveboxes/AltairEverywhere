#include <stdlib.h>

#include <applibs/gpio.h>

#include "GroveLightSensor.h"
#include "GroveAD7992.h"


typedef struct
{
	void *inst;
	int pinId;
}
GroveLightSensorInstance;

void* GroveLightSensor_Init(int i2cFd, int analog_pin)
{
	GroveLightSensorInstance* this = (GroveLightSensorInstance*)malloc(sizeof(GroveLightSensorInstance));
	this->inst = GroveAD7992_Open(i2cFd);
	this->pinId = analog_pin;

	return this;
}

float GroveLightSensor_Read(void* inst)
{
	GroveLightSensorInstance* this = (GroveLightSensorInstance*)inst;
	return GroveAD7992_Read(this->inst, this->pinId);
}

