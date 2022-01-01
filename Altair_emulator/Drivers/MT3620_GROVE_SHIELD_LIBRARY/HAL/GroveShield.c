#include "GroveShield.h"
#include "GroveUART.h"
#include "GroveI2C.h"
#include "../Common/Delay.h"

#include <applibs/log.h>
#include <unistd.h>
#include <string.h>

#include "hw/azure_sphere_learning_path.h"


/**
	Set bauud rate for SC18IM700
*/
const uint8_t baudrate_230400_conf[4] = { 0x00, 0x10, 0x01, 0x00 };
const uint8_t baudrate_115200_conf[4] = { 0x00, 0x30, 0x01, 0x00};
const uint8_t baudrate_19200_conf[4] = { 0x00, 0x70, 0x01, 0x01};
const uint8_t baudrate_14400_conf[4] = { 0x00, 0xF4, 0x01, 0x01};
const uint8_t baudrate_9600_conf[4] = { 0x00, 0xF0, 0x01, 0x02};

static void baudrate_conf(int *fd, UART_BaudRate_Type baudrate)
{
	static uint8_t trial = 0;
	uint8_t d0, d1;
	uint8_t conf[4] = { 0 };

	close(*fd);
	*fd = GroveUART_Open(MT3620_RDB_HEADER2_ISU0_UART, 9600);

	while (true)
	{
		d0 = 0, d1 = 0;
		while (d0 != baudrate_9600_conf[1])
		{
			SC18IM700_ReadReg(*fd, 0x00, &d0);			
		}
		while (d1 != baudrate_9600_conf[3])
		{
			SC18IM700_ReadReg(*fd, 0x01, &d1);
		}		
		//Log_Debug("BR1: %x %x\n", d0, d1);
		if (d0 == baudrate_9600_conf[1] && d1 == baudrate_9600_conf[3]) break;
		usleep(500000);
	}

	/** Change UART baudrate for SC18IM700 */		
	if (baudrate == 230400) memcpy(conf, baudrate_230400_conf, 4);
	else if(baudrate == 115200) memcpy(conf, baudrate_115200_conf, 4);
	else if (baudrate == 19200) memcpy(conf, baudrate_19200_conf, 4);
	else if (baudrate == 14400) memcpy(conf, baudrate_14400_conf, 4);
	else if (baudrate == 9600) memcpy(conf, baudrate_9600_conf, 4);
	else {
		Log_Debug("[error] Baudrate not found.");
		return;
	}
	SC18IM700_WriteRegBytes(*fd, conf, 4);

	close(*fd);
	*fd = GroveUART_Open(MT3620_RDB_HEADER2_ISU0_UART, baudrate);

	SC18IM700_ReadReg(*fd, 0x00, &d0);
	SC18IM700_ReadReg(*fd, 0x01, &d1);

	//Log_Debug("BR2: %x %x\n", d0, d1);

	if ((d0 != conf[1]) || (d1 != conf[3]))
	{
		trial++;
		if (trial > 10) return;
		baudrate_conf(fd, baudrate);
	}
	trial = 0;
}

void GroveShield_Initialize(int* fd, uint32_t baudrate)
{
	/**fd = GroveUART_Open(MT3620_RDB_HEADER2_ISU0_UART, 9600);*/
	baudrate_conf(fd, baudrate);
}