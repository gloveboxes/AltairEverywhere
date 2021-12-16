#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include <applibs/gpio.h>

#include "Grove4DigitDisplay.h"
#include "../Common/Delay.h"


static bool _clockpoint = false;

typedef struct
{
	int ClkFd;
	int DioFd;
	float Brightness;
}
Grove4DigitDisplayInstance;


////////////////////////////////////////////////////////////////////////////////
// TM1637

static void TM1637_Start(Grove4DigitDisplayInstance* this)
{
	GPIO_SetValue(this->ClkFd, GPIO_Value_High);
	GPIO_SetValue(this->DioFd, GPIO_Value_Low);
	usleep(1);
}

static void TM1637_End(Grove4DigitDisplayInstance* this)
{
	GPIO_SetValue(this->ClkFd, GPIO_Value_Low);
	usleep(1);
	GPIO_SetValue(this->ClkFd, GPIO_Value_High);
	usleep(1);
	GPIO_SetValue(this->DioFd, GPIO_Value_High);
	usleep(1);
}

static void TM1637_Write(Grove4DigitDisplayInstance* this, uint8_t data)
{
	for (int i = 0; i < 8; i++)
	{
		GPIO_SetValue(this->ClkFd, GPIO_Value_Low);
		GPIO_SetValue(this->DioFd, data & 1 ? GPIO_Value_High : GPIO_Value_Low);
		data >>= 1;
		usleep(1);

		GPIO_SetValue(this->ClkFd, GPIO_Value_High);
		usleep(1);
	}

	GPIO_SetValue(this->DioFd, GPIO_Value_High);
	GPIO_SetValue(this->ClkFd, GPIO_Value_Low);
	usleep(1);

	GPIO_Value_Type ack;
	GPIO_GetValue(this->DioFd, &ack);
	GPIO_SetValue(this->DioFd, GPIO_Value_Low);
	GPIO_SetValue(this->ClkFd, GPIO_Value_High);
	usleep(1);
}

////////////////////////////////////////////////////////////////////////////////
// Grove4DigitDisplay

#define ADDR_FIXED		(0x44)

static const uint8_t TubeTab[] =
{
	0x3f, 0x06, 0x5b, 0x4f,	// '0', '1', '2', '3',
	0x66, 0x6d, 0x7d, 0x07,	// '4', '5', '6', '7',
	0x7f, 0x6f, 0x77, 0x7c,	// '8', '9', 'A', 'b',
	0x39, 0x5e, 0x79, 0x71,	// 'C', 'd', 'E', 'F',
};

void* Grove4DigitDisplay_Open(GPIO_Id pin_clk, GPIO_Id pin_dio)
{
	Grove4DigitDisplayInstance* this = (Grove4DigitDisplayInstance*)malloc(sizeof(Grove4DigitDisplayInstance));

	this->Brightness = 0.5f;

	this->ClkFd = GPIO_OpenAsOutput(pin_clk, GPIO_OutputMode_PushPull, GPIO_Value_High);
	this->DioFd = GPIO_OpenAsOutput(pin_dio, GPIO_OutputMode_OpenDrain, GPIO_Value_High);
	usleep(1);

	return this;
}

void Grove4DigitDisplay_DisplayOneSegment(void* inst, int bitAddr, int dispData)
{
	Grove4DigitDisplayInstance* this = (Grove4DigitDisplayInstance*)inst;

	uint8_t segData;
	switch (dispData)
	{
	case -1:	// Blank
		segData = 0x00;
		break;
	default:
		if (0 <= dispData && dispData <= 15)
		{
			segData = TubeTab[dispData];
			if (_clockpoint)
			{
				segData |= 0x80;
			}
		}
		else
		{
			segData = 0x00;
		}
	}

	TM1637_Start(this);
	TM1637_Write(this, ADDR_FIXED);
	TM1637_End(this);

	TM1637_Start(this);
	TM1637_Write(this, (uint8_t)(bitAddr | 0xc0));
	TM1637_Write(this, segData);
	TM1637_End(this);

	TM1637_Start(this);
	TM1637_Write(this, (uint8_t)(0x88 + this->Brightness * 7));
	TM1637_End(this);
}

void Grove4DigitDisplay_DisplayValue(void* inst, int value)
{
	Grove4DigitDisplay_DisplayOneSegment(inst, 3, value % 10);
	value /= 10;
	Grove4DigitDisplay_DisplayOneSegment(inst, 2, value % 10);
	value /= 10;
	Grove4DigitDisplay_DisplayOneSegment(inst, 1, value % 10);
	value /= 10;
	Grove4DigitDisplay_DisplayOneSegment(inst, 0, value % 10);
}

void Grove4DigitDisplay_DisplayClockPoint(bool clockpoint)
{
	_clockpoint = clockpoint;
}