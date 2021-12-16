/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "utils.h"
#include <applibs/log.h>
#include <applibs/networking.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


char* uint8_to_binary(uint8_t bitmap, char* buffer, size_t buffer_length)
{
	uint16_t mask = 1;
	uint8_t bit_number = 8;

	if (buffer_length < 9) { return NULL; }

	while (bit_number-- > 0)
	{
		buffer[bit_number] = bitmap & mask ? '1' : '0';
		mask = (uint16_t)(mask << 1);
	}

	buffer[8] = 0x00;
	return buffer;
}

void delay(int ms)
{
	//#ifdef SHOW_DEBUG_MSGS
	//	Log_Debug(">>> %s\n", __func__);
	//#endif

	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000;
	nanosleep(&ts, NULL);
}

static char ascBuff[17];
void DisplayLineOffset(uint16_t offset);

void DumpBuffer(uint8_t* buffer, uint16_t length)
{
	int LinePos = 0;

	uint16_t lineOffset = 0;

	DisplayLineOffset(lineOffset);
	memset(ascBuff, 0x20, 16);
	for (int x = 0; x < length; x++)
	{
		Log_Debug("%02x ", buffer[x]);
		if (buffer[x] >= 0x20 && buffer[x] <= 0x7f)
			ascBuff[LinePos] = buffer[x];
		else
			ascBuff[LinePos] = '.';

		LinePos++;
		if (LinePos == 0x10)
		{
			Log_Debug("%s", ascBuff);
			Log_Debug("\n");
            lineOffset = (uint16_t)(lineOffset + 16);
			if (x + 1 != length)
			{
				DisplayLineOffset(lineOffset);
			}
			LinePos = 0;
			memset(ascBuff, 0x20, 16);
		}
	}

	if (LinePos != 0)
		Log_Debug("%s", ascBuff);

	Log_Debug("\n");
}

void DisplayLineOffset(uint16_t offset)
{
	Log_Debug("%04x: ", offset);
}

char* get_i8080_instruction_name(uint8_t opcode, uint8_t* i8080_instruction_size)
{
	// http://www.emulator101.com/reference/8080-by-opcode.html
	static char* i8080_instruction[] = { "NOP", "LXI B,D16", "STAX B", "INX B", "INR B", "DCR B", "MVI B, D8", "RLC", "-", "DAD B", "LDAX B", "DCX B", "INR C", "DCR C", "MVI C,D8", "RRC", "-", "LXI D,D16", "STAX D", "INX D", "INR D", "DCR D", "MVI D, D8", "RAL", "-", "DAD D", "LDAX D", "DCX D", "INR E", "DCR E", "MVI E,D8", "RAR", "-", "LXI H,D16", "SHLD adr", "INX H", "INR H", "DCR H", "MVI H,D8", "DAA", "-", "DAD H", "LHLD adr", "DCX H", "INR L", "DCR L", "MVI L, D8", "CMA", "-", "LXI SP, D16", "STA adr", "INX SP", "INR M", "DCR M", "MVI M,D8", "STC", "-", "DAD SP", "LDA adr", "DCX SP", "INR A", "DCR A", "MVI A,D8", "CMC", "MOV B,B", "MOV B,C", "MOV B,D", "MOV B,E", "MOV B,H", "MOV B,L", "MOV B,M", "MOV B,A", "MOV C,B", "MOV C,C", "MOV C,D", "MOV C,E", "MOV C,H", "MOV C,L", "MOV C,M", "MOV C,A", "MOV D,B", "MOV D,C", "MOV D,D", "MOV D,E", "MOV D,H", "MOV D,L", "MOV D,M", "MOV D,A", "MOV E,B", "MOV E,C", "MOV E,D", "MOV E,E", "MOV E,H", "MOV E,L", "MOV E,M", "MOV E,A", "MOV H,B", "MOV H,C", "MOV H,D", "MOV H,E", "MOV H,H", "MOV H,L", "MOV H,M", "MOV H,A", "MOV L,B", "MOV L,C", "MOV L,D", "MOV L,E", "MOV L,H", "MOV L,L", "MOV L,M", "MOV L,A", "MOV M,B", "MOV M,C", "MOV M,D", "MOV M,E", "MOV M,H", "MOV M,L", "HLT", "MOV M,A", "MOV A,B", "MOV A,C", "MOV A,D", "MOV A,E", "MOV A,H", "MOV A,L", "MOV A,M", "MOV A,A", "ADD B", "ADD C", "ADD D", "ADD E", "ADD H", "ADD L", "ADD M", "ADD A", "ADC B", "ADC C", "ADC D", "ADC E", "ADC H", "ADC L", "ADC M", "ADC A", "SUB B", "SUB C", "SUB D", "SUB E", "SUB H", "SUB L", "SUB M", "SUB A", "SBB B", "SBB C", "SBB D", "SBB E", "SBB H", "SBB L", "SBB M", "SBB A", "ANA B", "ANA C", "ANA D", "ANA E", "ANA H", "ANA L", "ANA M", "ANA A", "XRA B", "XRA C", "XRA D", "XRA E", "XRA H", "XRA L", "XRA M", "XRA A", "ORA B", "ORA C", "ORA D", "ORA E", "ORA H", "ORA L", "ORA M", "ORA A", "CMP B", "CMP C", "CMP D", "CMP E", "CMP H", "CMP L", "CMP M", "CMP A", "RNZ", "POP B", "JNZ adr", "JMP adr", "CNZ adr", "PUSH B", "ADI D8", "RST 0", "RZ", "RET", "JZ adr", "-", "CZ adr", "CALL adr", "ACI D8", "RST 1", "RNC", "POP D", "JNC adr", "OUT D8", "CNC adr", "PUSH D", "SUI D8", "RST 2", "RC", "-", "JC adr", "IN D8", "CC adr", "-", "SBI D8", "RST 3", "RPO", "POP H", "JPO adr", "XTHL", "CPO adr", "PUSH H", "ANI D8", "RST 4", "RPE", "PCHL", "JPE adr", "XCHG", "CPE adr", "-", "XRI D8", "RST 5", "RP", "POP PSW", "JP adr", "DI", "CP adr", "PUSH PSW", "ORI D8", "RST 6", "RM", "SPHL", "JM adr", "EI", "CM adr", "-", "CPI D8", "RST 7" };
	static uint8_t i8080_instruction_length[] = { 1, 3, 1, 1, 1, 1, 2, 1, 0, 1, 1, 1, 1, 1, 2, 1, 0, 3, 1, 1, 1, 1, 2, 1, 0, 1, 1, 1, 1, 1, 2, 1, 0, 3, 3, 1, 1, 1, 2, 1, 0, 1, 3, 1, 1, 1, 2, 1, 0, 3, 3, 1, 1, 1, 2, 1, 0, 1, 3, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 3, 1, 2, 1, 1, 1, 3, 0, 3, 3, 2, 1, 1, 1, 3, 2, 3, 1, 2, 1, 1, 0, 3, 2, 3, 0, 2, 1, 1, 1, 3, 1, 3, 1, 2, 1, 1, 1, 3, 1, 3, 0, 2, 1, 1, 1, 3, 1, 3, 1, 2, 1, 1, 1, 3, 1, 3, 0, 2, 1 };
	//int index = 0;

	*i8080_instruction_size = i8080_instruction_length[opcode];
	return i8080_instruction[opcode];
}

char* log_memory_usage(char* buffer, size_t buffer_size, const char* message) {
	snprintf(buffer, buffer_size, "%s: TMU %dKB, PUMU %dKB, UMM %dKB \n",
		message,
		Applications_GetTotalMemoryUsageInKB(),
		Applications_GetPeakUserModeMemoryUsageInKB(),
		Applications_GetUserModeMemoryUsageInKB());

	return buffer;
}