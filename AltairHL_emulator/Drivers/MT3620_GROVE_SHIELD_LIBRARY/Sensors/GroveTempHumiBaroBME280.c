#include "GroveTempHumiBaroBME280.h"
#include <stdlib.h>
#include <math.h>
#include "../HAL/GroveI2C.h"

#define BME280_ADDRESS				(0x76 << 1)

#define BME280_REG_DIG_T1			(0x88)
#define BME280_REG_DIG_T2			(0x8A)
#define BME280_REG_DIG_T3			(0x8C)
#define BME280_REG_CHIPID			(0xD0)
#define BME280_REG_CONTROLHUMID		(0xF2)
#define BME280_REG_CONTROL			(0xF4)
#define BME280_REG_TEMPDATA			(0xFA)

typedef struct
{
	int I2cFd;
	float Temperature;
}
GroveTempHumiBaroBME280Instance;

void* GroveTempHumiBaroBME280_Open(int i2cFd)
{
	GroveTempHumiBaroBME280Instance* this = (GroveTempHumiBaroBME280Instance*)malloc(sizeof(GroveTempHumiBaroBME280Instance));

	this->I2cFd = i2cFd;
	this->Temperature = NAN;

	uint8_t val8;
	if (!GroveI2C_ReadReg8(this->I2cFd, BME280_ADDRESS, BME280_REG_CHIPID, &val8)) return NULL;
	if (val8 != 0x60) return NULL;

	GroveI2C_WriteReg8(this->I2cFd, BME280_ADDRESS, BME280_REG_CONTROLHUMID, 0x05);
	GroveI2C_WriteReg8(this->I2cFd, BME280_ADDRESS, BME280_REG_CONTROL, 0xb7);

	return this;
}

void GroveTempHumiBaroBME280_Read(void* inst)
{
	GroveTempHumiBaroBME280Instance* this = (GroveTempHumiBaroBME280Instance*)inst;

	this->Temperature = NAN;

	uint16_t dig_T1;
	int16_t dig_T2;
	int16_t dig_T3;
	if (!GroveI2C_ReadReg16(this->I2cFd, BME280_ADDRESS, BME280_REG_DIG_T1, &dig_T1)) return;
	if (!GroveI2C_ReadReg16(this->I2cFd, BME280_ADDRESS, BME280_REG_DIG_T2, (uint16_t*)&dig_T2)) return;
	if (!GroveI2C_ReadReg16(this->I2cFd, BME280_ADDRESS, BME280_REG_DIG_T3, (uint16_t*)&dig_T3)) return;

	int32_t adc_T;
	if (!GroveI2C_ReadReg24BE(this->I2cFd, BME280_ADDRESS, BME280_REG_TEMPDATA, &adc_T)) return;

	adc_T >>= 4;
	int32_t var1 = (((adc_T >> 3) - ((int32_t)(dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
	int32_t var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;

	int32_t t_fine = var1 + var2;
	this->Temperature = (float)((t_fine * 5 + 128) >> 8) / 100;
}

float GroveTempHumiBaroBME280_GetTemperature(void* inst)
{
	GroveTempHumiBaroBME280Instance* this = (GroveTempHumiBaroBME280Instance*)inst;

	return this->Temperature;
}
