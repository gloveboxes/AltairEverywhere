#ifndef _OP_CODES_H_
#define _OP_CODES_H_

#define DESTINATION(x)	(x >> 3 & 7)
#define SOURCE(x)		(x & 7)
#define CONDITION(x)	(x >> 3 & 7)
#define VECTOR(x)		(x >> 3 & 7)
#define RP(x)			(x >> 4 & 3)

#define REGISTER_A		7
#define REGISTER_B		0
#define REGISTER_C		1
#define REGISTER_D		2
#define REGISTER_E		3
#define REGISTER_H		4
#define REGISTER_L		5
#define MEMORY_ACCESS	6

#define PAIR_BC			0
#define PAIR_DE			1
#define PAIR_HL			2
#define PAIR_SP			3

#define CONDITION_NZ	0
#define CONDITION_Z		1
#define CONDITION_NC	2
#define CONDITION_C		3
#define CONDITION_PO	4
#define CONDITION_PE	5
#define CONDITION_P		6
#define CONDITION_M		7

#define CYCLES_JMP		10
#define CYCLES_NOP		4
#define CYCLES_MOV_REG	5
#define CYCLES_MOV_MEM	7
#define CYCLES_MVI_REG	7
#define CYCLES_MVI_MEM	10
#define CYCLES_LXI		10
#define CYCLES_LDA		13
#define CYCLES_STA		13
#define CYCLES_LHLD		16
#define CYCLES_SHLD		16
#define CYCLES_LDAX		7
#define CYCLES_STAX		7
#define CYCLES_XCHG		5
#define CYCLES_ADD		4
#define CYCLES_ADI		7
#define CYCLES_ADC		4
#define CYCLES_ACI		7
#define CYCLES_SUB		4
#define CYCLES_SUI		7
#define CYCLES_SBB		4
#define CYCLES_SBI		7
#define CYCLES_INR		5
#define CYCLES_DCR		5
#define CYCLES_INX		5
#define CYCLES_DCX		5
#define CYCLES_DAD		10
#define CYCLES_ANA		4
#define CYCLES_ANI		7
#define CYCLES_ORA		4
#define CYCLES_ORI		7
#define CYCLES_XRA		4
#define CYCLES_XRI		7
#define CYCLES_EI		4
#define CYCLES_DI		4
#define CYCLES_XTHL		18
#define CYCLES_SPHL		5
#define CYCLES_IN		10
#define CYCLES_OUT		10
#define CYCLES_PUSH		11
#define CYCLES_POP		10
#define CYCLES_RLC		4
#define CYCLES_RRC		4
#define CYCLES_RAL		4
#define CYCLES_RAR		4
#define CYCLES_RET		5
#define CYCLES_CALL		17
#define CYCLES_RST		11
#define CYCLES_CMP		4
#define CYCLES_CPI		7
#define CYCLES_STC		1
#define CYCLES_CMC		2
#define CYCLES_CMA		2
#define CYCLES_PCHL		5
#define CYCLES_DAA		5

#endif