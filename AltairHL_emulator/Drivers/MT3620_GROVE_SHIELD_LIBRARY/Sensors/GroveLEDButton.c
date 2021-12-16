#include <stdlib.h>
#include <applibs/gpio.h>

#include "GroveLEDButton.h"

typedef struct
{
	int btnFd;
	int ledFd;
}
GroveLEDButtonInstance;

void *GroveLEDButton_Init(GPIO_Id btn, GPIO_Id led)
{
	GroveLEDButtonInstance *this = (GroveLEDButtonInstance*)malloc(sizeof(GroveLEDButtonInstance));
	this->btnFd = GPIO_OpenAsInput(btn);
	this->ledFd = GPIO_OpenAsOutput(led, GPIO_OutputMode_PushPull, GPIO_Value_Low);
	
	return this;
}

GPIO_Value_Type GroveLEDButton_GetBtnState(void *inst)
{
	GPIO_Value_Type state;
	GroveLEDButtonInstance *this = (GroveLEDButtonInstance*)inst;
	GPIO_GetValue(this->btnFd, &state);

	return state;
}

void GroveLEDButton_LedOn(void *inst)
{
	GroveLEDButtonInstance *this = (GroveLEDButtonInstance*)inst;
	GPIO_SetValue(this->ledFd, GPIO_Value_High);
}

void GroveLEDButton_LedOff(void *inst)
{
	GroveLEDButtonInstance *this = (GroveLEDButtonInstance*)inst;
	GPIO_SetValue(this->ledFd, GPIO_Value_Low);
}