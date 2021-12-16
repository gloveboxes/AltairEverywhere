#include "GroveTempHumiSHT31.h"
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "../HAL/GroveI2C.h"
#include "../Common/Delay.h"


#define SHT31_ADDRESS		(0x44 << 1)

#define POLYNOMIAL			(0x31)

#define CMD_SOFT_RESET		(0x30a2)
#define CMD_SINGLE_HIGH		(0x2400)
#define CMD_HEATER_ENABLE	(0x306d)
#define CMD_HEATER_DISABLE	(0x3066)

typedef struct
{
	int I2cFd;
	float Temperature;
	float Humidity;
}
GroveTempHumiSHT31Instance;


static void SendCommand(GroveTempHumiSHT31Instance* this, uint16_t cmd)
{
	uint8_t writeData[2];
	writeData[0] = (uint8_t)(cmd >> 8);
	writeData[1] = (uint8_t)(cmd & 0xff);
	GroveI2C_Write(this->I2cFd, SHT31_ADDRESS, writeData, sizeof(writeData));
}

static uint8_t CalcCRC8(const uint8_t* data, int dataSize)
{
	uint8_t crc = 0xff;

	for (int j = dataSize; j > 0; j--)
	{
		crc ^= *data++;

		for (int i = 8; i > 0; i--)
		{
			crc = (crc & 0x80) != 0 ? (uint8_t)(crc << 1 ^ POLYNOMIAL) : (uint8_t)(crc << 1);
		}
	}

	return crc;
}

void* GroveTempHumiSHT31_Open(int i2cFd)
{
	GroveTempHumiSHT31Instance* this = (GroveTempHumiSHT31Instance*)malloc(sizeof(GroveTempHumiSHT31Instance));

	this->I2cFd = i2cFd;
	this->Temperature = NAN;
	this->Humidity = NAN;

	SendCommand(this, CMD_SOFT_RESET);
	usleep(1000);

	return this;
}

void GroveTempHumiSHT31_Read(void* inst)
{
	GroveTempHumiSHT31Instance* this = (GroveTempHumiSHT31Instance*)inst;

	this->Temperature = NAN;

	SendCommand(this, CMD_SINGLE_HIGH);
	usleep(20000);

	uint8_t readData[6];
	if (!GroveI2C_Read(this->I2cFd, SHT31_ADDRESS, readData, sizeof(readData))) return;

	if (readData[2] != CalcCRC8(&readData[0], 2)) return;
	if (readData[5] != CalcCRC8(&readData[3], 2)) return;

	uint16_t ST;
	ST = readData[0];
	ST = (uint16_t)(ST << 8);
	ST = (uint16_t)(ST | readData[1]);

	uint16_t SRH;
	SRH = readData[3];
	SRH = (uint16_t)(SRH << 8);
	SRH = (uint16_t)(SRH | readData[4]);

	this->Temperature = (float)ST * 175 / 0xffff - 45;
	this->Humidity = (float)SRH * 100 / 0xffff;
}

void GroveTempHumiSHT31_EnableHeater(void* inst)
{
	GroveTempHumiSHT31Instance* this = (GroveTempHumiSHT31Instance*)inst;

	SendCommand(this, CMD_HEATER_ENABLE);
	usleep(20000);
}

void GroveTempHumiSHT31_DisableHeater(void* inst)
{
	GroveTempHumiSHT31Instance* this = (GroveTempHumiSHT31Instance*)inst;

	SendCommand(this, CMD_HEATER_DISABLE);
	usleep(20000);
}

float GroveTempHumiSHT31_GetTemperature(void* inst)
{
	GroveTempHumiSHT31Instance* this = (GroveTempHumiSHT31Instance*)inst;

	return this->Temperature;
}

float GroveTempHumiSHT31_GetHumidity(void* inst)
{
	GroveTempHumiSHT31Instance* this = (GroveTempHumiSHT31Instance*)inst;

	return this->Humidity;
}
