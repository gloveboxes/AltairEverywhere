#include "GroveI2C.h"
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "GroveUART.h"

////////////////////////////////////////////////////////////////////////////////
// SC18IM700

static void wait_for_i2cState_ok(int fd)
{
	uint8_t i2cState;
	SC18IM700_ReadReg(fd, 0x0A, &i2cState);
	while (i2cState != I2C_OK)
	{
		SC18IM700_ReadReg(fd, 0x0A, &i2cState);
	}
}

static void SC18IM700_I2cWrite(int fd, uint8_t address, const uint8_t* data, int dataSize)
{	
	// Send
	uint8_t send[3 + dataSize + 1];

	send[0] = 'S';
	send[1] = address & 0xfe;
	send[2] = (uint8_t)dataSize;
	memcpy(&send[3], data, (size_t)dataSize);
	send[3 + dataSize] = 'P';

	GroveUART_Write(fd, send, (int)sizeof(send));

	// wait for I2C state OK
	wait_for_i2cState_ok(fd);
}

static bool SC18IM700_I2cRead(int fd, uint8_t address, uint8_t* data, int dataSize)
{
	// Send

	uint8_t send[4];

	send[0] = 'S';
	send[1] = address | 0x01;
	send[2] = (uint8_t)dataSize;
	send[3] = 'P';

	GroveUART_Write(fd, send, sizeof(send));

	// Receive

	if (!GroveUART_Read(fd, data, dataSize)) return false;

	return true;
}

bool SC18IM700_ReadReg(int fd, uint8_t reg, uint8_t* data)
{
	// Send

	uint8_t send[3];

	send[0] = 'R';
	send[1] = reg;
	send[2] = 'P';
	
	GroveUART_Write(fd, send, 3);

	// Receive

	if (!GroveUART_Read(fd, data, 1)) return false;

	return true;
}
void SC18IM700_WriteReg(int fd, uint8_t reg, uint8_t data)
{
	// Send

	uint8_t send[4];

	send[0] = 'W';
	send[1] = reg;
	send[2] = data;
	send[3] = 'P';

	GroveUART_Write(fd, send, (int)sizeof(send));
}

void SC18IM700_WriteRegBytes(int fd, uint8_t *data, uint8_t dataSize)
{
	// Send

	uint8_t send[2 + dataSize];

	send[0] = 'W';
	memcpy(&send[1], data, (uint8_t)dataSize);
	send[dataSize+1] = 'P';

	GroveUART_Write(fd, send, (uint8_t)(dataSize+2));
}


////////////////////////////////////////////////////////////////////////////////
// GroveI2C

void(*GroveI2C_Write)(int fd, uint8_t address, const uint8_t* data, int dataSize) = SC18IM700_I2cWrite;
bool(*GroveI2C_Read)(int fd, uint8_t address, uint8_t* data, int dataSize) = SC18IM700_I2cRead;

void GroveI2C_WriteReg8(int fd, uint8_t address, uint8_t reg, uint8_t val)
{
	uint8_t send[2];
	send[0] = reg;
	send[1] = val;
	GroveI2C_Write(fd, address, send, sizeof(send));
}

void GroveI2C_WriteBytes(int fd, uint8_t address, uint8_t *data, uint8_t dataSize)
{
	uint8_t send[dataSize];
	memcpy(send, data, dataSize);

	GroveI2C_Write(fd, address, send, (int)sizeof(send));
}

bool GroveI2C_ReadReg8(int fd, uint8_t address, uint8_t reg, uint8_t* val)
{
	GroveI2C_Write(fd, address, &reg, 1);

	uint8_t recv[1];
	if (!GroveI2C_Read(fd, address, recv, sizeof(recv))) return false;

	*val = recv[0];

	return true;
}

bool GroveI2C_ReadReg16(int fd, uint8_t address, uint8_t reg, uint16_t* val)
{
	GroveI2C_Write(fd, address, &reg, 1);

	uint8_t recv[2];
	if (!GroveI2C_Read(fd, address, recv, sizeof(recv))) return false;

	*val = (uint16_t)(recv[1] << 8 | recv[0]);

	return true;
}

bool GroveI2C_ReadReg24BE(int fd, uint8_t address, uint8_t reg, uint32_t* val)
{
	GroveI2C_Write(fd, address, &reg, 1);

	uint8_t recv[3];
	if (!GroveI2C_Read(fd, address, recv, sizeof(recv))) return false;

	*val = (uint32_t)(recv[0] << 16 | recv[1] << 8 | recv[2]);

	return true;
}


////////////////////////////////////////////////////////////////////////////////
