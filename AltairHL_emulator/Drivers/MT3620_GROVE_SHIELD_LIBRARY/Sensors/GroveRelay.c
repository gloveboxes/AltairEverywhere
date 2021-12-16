#include "GroveRelay.h"
#include <stdlib.h>

#include <applibs/gpio.h>

typedef struct
{
	int PinFd;
}
GroveRelayInstance;

void* GroveRelay_Open(GPIO_Id pinId)
{
	GroveRelayInstance* this = (GroveRelayInstance*)malloc(sizeof(GroveRelayInstance));

	this->PinFd = GPIO_OpenAsOutput(pinId, GPIO_OutputMode_PushPull, GPIO_Value_Low);

	return this;
}

void GroveRelay_On(void* inst)
{
	GroveRelayInstance* this = (GroveRelayInstance*)inst;

	GPIO_SetValue(this->PinFd, GPIO_Value_High);
}

void GroveRelay_Off(void* inst)
{
	GroveRelayInstance* this = (GroveRelayInstance*)inst;

	GPIO_SetValue(this->PinFd, GPIO_Value_Low);
}
