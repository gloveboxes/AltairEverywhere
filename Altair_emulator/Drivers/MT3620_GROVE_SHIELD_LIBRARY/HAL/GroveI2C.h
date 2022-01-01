#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "../applibs_versions.h"

#define I2C_OK								0xF0
#define I2C_NACK_ON_ADDRESS		0xF1
#define I2C_NACK_ON_DATA			0xF2
#define I2C_TIME_OUT					0xF8

bool SC18IM700_ReadReg(int fd, uint8_t reg, uint8_t* data);
void SC18IM700_WriteReg(int fd, uint8_t reg, uint8_t data);
void SC18IM700_WriteRegBytes(int fd, uint8_t *data, uint8_t dataSize);

void(*GroveI2C_Write)(int fd, uint8_t address, const uint8_t* data, int dataSize);
bool(*GroveI2C_Read)(int fd, uint8_t address, uint8_t* data, int dataSize);

void GroveI2C_WriteReg8(int fd, uint8_t address, uint8_t reg, uint8_t val);
void GroveI2C_WriteBytes(int fd, uint8_t address, uint8_t *data, uint8_t dataSize);

bool GroveI2C_ReadReg8(int fd, uint8_t address, uint8_t reg, uint8_t* val);
bool GroveI2C_ReadReg16(int fd, uint8_t address, uint8_t reg, uint16_t* val);
bool GroveI2C_ReadReg24BE(int fd, uint8_t address, uint8_t reg, uint32_t* val);
