#include "GroveUART.h"
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>

#include <applibs/uart.h>

int GroveUART_Open(UART_Id id, UART_BaudRate_Type baudRate)
{
	UART_Config uartConfig;
	UART_InitConfig(&uartConfig);
	uartConfig.baudRate = baudRate;

	return UART_Open(id, &uartConfig);
}

void GroveUART_Write(int fd, const uint8_t* data, int dataSize)
{	
	write(fd, data, (size_t)dataSize);
}

bool GroveUART_Read(int fd, uint8_t* data, int dataSize)
{
	int totalReadSize = 0;
	do
	{
		int readSize = read(fd, &data[totalReadSize], (size_t)(dataSize - totalReadSize));
		if (readSize < 0) return false;
		totalReadSize += readSize;
	} while (totalReadSize < dataSize);

	return true;
}
