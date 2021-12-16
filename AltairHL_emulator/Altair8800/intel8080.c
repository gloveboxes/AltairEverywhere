#include "intel8080.h"
#include "op_codes.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <applibs/log.h>

#ifdef ARDUINO
	#include <Arduino.h>
	#include <SPI.h>
#endif
#ifdef WIN32
	#include <windows.h>
#endif

#include "memory.h"

// define CPU stats LEDs
#define STATUS_MEMORY_READ		0x80
#define STATUS_PORT_INPUT		0x40
#define STATUS_OP_CODE_FETCH	0x20
#define STATUS_PORT_OUTPUT		0x10
#define STATUS_HALT				0x08
#define STATUS_STACK			0x04
#define STATUS_WRITE_OUTPUT		0x02	// inverted!
#define STATUS_INTERRUPT		0x01

uint8_t get_parity(uint8_t val)
{
	val ^= val >> 4;
	val &= 0xf;
	return !((0x6996 >> val) & 1);
}

void i8080_reset(intel8080_t *cpu, port_in in, port_out out, read_sense_switches sense,
                        disk_controller_t *disk_controller, azure_sphere_port_in sphere_port_in, azure_sphere_port_out sphere_port_out)
{
	memset(cpu, 0, sizeof(intel8080_t));
	cpu->term_in = in;
	cpu->term_out = out;
	cpu->_sphere_port_in = sphere_port_in;
	cpu->_sphere_port_out = sphere_port_out;
	cpu->disk_controller = *disk_controller;
	cpu->registers.flags = 0x2;
	cpu->sense = sense;
	cpu->cpuStatus = 0x00;
}

int i8080_check_carry(uint16_t a, uint16_t b)
{
	if((a + b) > 0xff)
		return 1;
	else
		return 0;
}

int i8080_check_half_carry(uint16_t a, uint16_t b)
{
	a &= 0xf;
	b &= 0xf;

	if((a + b) > 0xf)
		return 1;
	else
		return 0;
}
void i8080_mwrite(intel8080_t *cpu)
{
	cpu->cpuStatus &= ~(STATUS_MEMORY_READ);
	write8(cpu->address_bus, cpu->data_bus);
}

void i8080_mread(intel8080_t *cpu)
{
	{
		cpu->cpuStatus |= STATUS_MEMORY_READ;
		cpu->data_bus = read8(cpu->address_bus);
	}
}

void i8080_pairwrite(intel8080_t *cpu, uint8_t pair, uint16_t val)
{
	cpu->cpuStatus &= ~(STATUS_MEMORY_READ);
	switch(pair)
	{
	case PAIR_BC:
		cpu->registers.bc = val;
		break;
	case PAIR_DE:
		cpu->registers.de = val;
		break;
	case PAIR_HL:
		cpu->registers.hl = val;
		break;
	case PAIR_SP:
		cpu->registers.sp = val;
		break;
	}
}

uint16_t i8080_pairread(intel8080_t *cpu, uint8_t pair)
{
	cpu->cpuStatus |= STATUS_MEMORY_READ;
	switch(pair)
	{
	case PAIR_BC:
		return cpu->registers.bc;
		break;
	case PAIR_DE:
		return cpu->registers.de;
		break;
	case PAIR_HL:
		return cpu->registers.hl;
		break;
	case PAIR_SP:
		return cpu->registers.sp;
		break;
	default:
		return 0;
	}
}

void i8080_regwrite(intel8080_t *cpu, uint8_t reg, uint8_t val)
{
	switch(reg)
	{
	case REGISTER_A:
		cpu->registers.a = val;
		break;
	case REGISTER_B:
		cpu->registers.b = val;
		break;
	case REGISTER_C:
		cpu->registers.c = val;
		break;
	case REGISTER_D:
		cpu->registers.d = val;
		break;
	case REGISTER_E:
		cpu->registers.e = val;
		break;
	case REGISTER_H:
		cpu->registers.h = val;
		break;
	case REGISTER_L:
		cpu->registers.l = val;
		break;
	case MEMORY_ACCESS:
		cpu->address_bus = cpu->registers.hl;
		cpu->data_bus = val;
		i8080_mwrite(cpu);
	}
}

uint8_t i8080_regread(intel8080_t *cpu, uint8_t reg)
{
	switch(reg)
	{
	case REGISTER_A:
		return cpu->registers.a;
		break;
	case REGISTER_B:
		return cpu->registers.b;
		break;
	case REGISTER_C:
		return cpu->registers.c;
		break;
	case REGISTER_D:
		return cpu->registers.d;
		break;
	case REGISTER_E:
		return cpu->registers.e;
		break;
	case REGISTER_H:
		return cpu->registers.h;
		break;
	case REGISTER_L:
		return cpu->registers.l;
		break;
	case MEMORY_ACCESS:
		cpu->address_bus = cpu->registers.hl;
		i8080_mread(cpu);
		return cpu->data_bus;
	default:
		return 0;
	}
}

uint8_t i8080_check_condition(intel8080_t *cpu, uint8_t condition)
{
	switch(condition)
	{
	case CONDITION_NZ:
		return !(cpu->registers.flags & FLAGS_ZERO);
	case CONDITION_Z:
		return (cpu->registers.flags & FLAGS_ZERO);
	case CONDITION_NC:
		return !(cpu->registers.flags & FLAGS_CARRY);
	case CONDITION_C:
		return (cpu->registers.flags & FLAGS_CARRY);
	case CONDITION_PO:
		return !(cpu->registers.flags & FLAGS_PARITY);
	case CONDITION_PE:
		return (cpu->registers.flags & FLAGS_PARITY);
	case CONDITION_P:
		return !(cpu->registers.flags & FLAGS_SIGN);
	case CONDITION_M:
		return (cpu->registers.flags & FLAGS_SIGN);
	}
	return 0;
}

void i8080_examine(intel8080_t *cpu, uint16_t address)
{
	// Jump to the supplied address
	cpu->registers.pc = cpu->address_bus = address;
	cpu->data_bus = read8(cpu->address_bus);
}

void i8080_examine_next(intel8080_t *cpu)
{
	cpu->address_bus++;
	cpu->data_bus = read8(cpu->address_bus);
}

void i8080_deposit(intel8080_t *cpu, uint8_t data)
{
	cpu->data_bus = data;
	i8080_mwrite(cpu);
}

void i8080_deposit_next(intel8080_t *cpu, uint8_t data)
{
	i8080_examine_next(cpu);
	cpu->data_bus = data;
	i8080_mwrite(cpu);
}

void i8080_set_flag(intel8080_t *cpu, uint8_t flag)
{
	cpu->registers.flags |= flag;
}

void i8080_clear_flag(intel8080_t *cpu, uint8_t flag)
{
	cpu->registers.flags &= ~flag;
}

void i8080_update_flags(intel8080_t *cpu, uint8_t reg, uint8_t mask)
{
	uint8_t val = i8080_regread(cpu, reg);
	if(mask & FLAGS_PARITY)
	{
		if(get_parity(val))
			i8080_set_flag(cpu, FLAGS_PARITY);
		else
			i8080_clear_flag(cpu, FLAGS_PARITY);
	}
	if(mask & FLAGS_H)
	{
	}
	if(mask & FLAGS_IF)
	{
	}
	if(mask & FLAGS_ZERO)
	{
		if(val == 0)
			i8080_set_flag(cpu, FLAGS_ZERO);
		else
			i8080_clear_flag(cpu, FLAGS_ZERO);
	}
	if(mask & FLAGS_SIGN)
	{
		if(val & 0x80)
			i8080_set_flag(cpu, FLAGS_SIGN);
		else
			i8080_clear_flag(cpu, FLAGS_SIGN);
	}
}

void i8080_gensub(intel8080_t *cpu, uint16_t val)
{
	uint16_t a, b;
	// Subtract by adding with two-complement of val. Carry-flag meaning becomes inverted since we add.
	a = cpu->registers.a;
	b = 0x100 - val;

	if(i8080_check_half_carry(a, b))
		i8080_set_flag(cpu, FLAGS_H);
	else
		i8080_clear_flag(cpu, FLAGS_H);

	if(i8080_check_carry(a, b))
		i8080_clear_flag(cpu, FLAGS_CARRY);
	else
		i8080_set_flag(cpu, FLAGS_CARRY);

	a += b ;

	cpu->registers.a = a & 0xff;

	i8080_update_flags(cpu, REGISTER_A, FLAGS_ZERO | FLAGS_SIGN | FLAGS_PARITY | FLAGS_CARRY | FLAGS_H);
}

void i8080_compare(intel8080_t *cpu, uint8_t val)
{
	uint8_t tmp_a = cpu->registers.a;
	i8080_gensub(cpu, val);
	cpu->registers.a = tmp_a;
}

uint8_t i8080_mov(intel8080_t *cpu)
{
	uint8_t dest = DESTINATION(cpu->current_op_code);
	uint8_t source = SOURCE(cpu->current_op_code);
	uint8_t val;
	uint8_t cycles;

	if(dest == MEMORY_ACCESS || source == MEMORY_ACCESS)
		cycles = CYCLES_MOV_MEM;
	else
		cycles = CYCLES_MOV_REG;

	val = i8080_regread(cpu, source);
	i8080_regwrite(cpu, dest, val);
	cpu->registers.pc++;

	return cycles;
}

uint8_t i8080_mvi(intel8080_t *cpu)
{
	uint8_t dest = DESTINATION(cpu->current_op_code);
	uint8_t cycles;

	if(dest == MEMORY_ACCESS)
		cycles = CYCLES_MVI_MEM;
	else
		cycles = CYCLES_MVI_REG;

	i8080_regwrite(cpu, dest, read8(cpu->registers.pc+1));

	cpu->registers.pc+=2;

	return cycles;
}

uint8_t i8080_lxi(intel8080_t *cpu)
{
	uint8_t pair = RP(cpu->current_op_code);

	i8080_pairwrite(cpu, pair, read16(cpu->registers.pc+1));
	cpu->registers.pc+=3;

	return CYCLES_LXI;
}

uint8_t i8080_lda(intel8080_t *cpu)
{
	cpu->address_bus = read16(cpu->registers.pc+1);
	i8080_mread(cpu);
	cpu->registers.a = cpu->data_bus;

	cpu->registers.pc+=3;
	return CYCLES_LDA;
}

uint8_t i8080_sta(intel8080_t *cpu)
{
	cpu->address_bus = read16(cpu->registers.pc+1);
	cpu->data_bus = cpu->registers.a;
	i8080_mwrite(cpu);

	cpu->registers.pc+=3;
	return CYCLES_STA;
}

uint8_t i8080_lhld(intel8080_t *cpu)
{
	cpu->registers.hl = read16(read16(cpu->registers.pc+1));
	cpu->registers.pc+=3;
	return CYCLES_LHLD;
}

uint8_t i8080_shld(intel8080_t *cpu)
{
	write16(read16(cpu->registers.pc+1), cpu->registers.hl);
	cpu->registers.pc+=3;
	return CYCLES_SHLD;
}

// TODO: only BC and DE allowed for indirect
uint8_t i8080_ldax(intel8080_t *cpu)
{
	uint8_t pair = RP(cpu->current_op_code);

	cpu->registers.a = read8(i8080_pairread(cpu, pair));
	cpu->registers.pc++;
	return CYCLES_LDAX;
}

// TODO: only BC and DE allowed for indirect
uint8_t i8080_stax(intel8080_t *cpu)
{
	uint8_t pair = RP(cpu->current_op_code);
	write8(i8080_pairread(cpu, pair), cpu->registers.a);
	cpu->registers.pc++;
	return CYCLES_STAX;
}

uint8_t i8080_xchg(intel8080_t *cpu)
{
	uint16_t tmp = cpu->registers.hl;
	cpu->registers.hl = cpu->registers.de;
	cpu->registers.de = tmp;
	cpu->registers.pc++;

	return CYCLES_XCHG;
}

void i8080_genadd(intel8080_t *cpu, uint16_t val)
{
	uint8_t a;

	a = i8080_regread(cpu, REGISTER_A);

	if(i8080_check_half_carry(a, val))
		i8080_set_flag(cpu, FLAGS_H);
	else
		i8080_clear_flag(cpu, FLAGS_H);

	if(i8080_check_carry(a, val))
		i8080_set_flag(cpu, FLAGS_CARRY);
	else
		i8080_clear_flag(cpu, FLAGS_CARRY);

	cpu->registers.a += val;

	i8080_update_flags(cpu, REGISTER_A, FLAGS_ZERO | FLAGS_SIGN | FLAGS_PARITY | FLAGS_CARRY | FLAGS_H);
}


uint8_t i8080_add(intel8080_t *cpu)
{
	uint8_t source = SOURCE(cpu->current_op_code);
	uint8_t val = i8080_regread(cpu, source);
	i8080_genadd(cpu, val);
	cpu->registers.pc++;
	return CYCLES_ADD;
}

uint8_t i8080_adi(intel8080_t *cpu)
{
	i8080_genadd(cpu, read8(cpu->registers.pc+1));
	cpu->registers.pc+=2;
	return CYCLES_ADI;
}

uint8_t i8080_adc(intel8080_t *cpu)
{
	uint8_t source = SOURCE(cpu->current_op_code);
	uint16_t val = i8080_regread(cpu, source);

	if(cpu->registers.flags & FLAGS_CARRY)
		val++;
	i8080_genadd(cpu, val);
	cpu->registers.pc++;
	return CYCLES_ADC;
}

uint8_t i8080_aci(intel8080_t *cpu)
{
	uint16_t val;
	val = read8(cpu->registers.pc+1);
	if(cpu->registers.flags & FLAGS_CARRY)
		val++;
	i8080_genadd(cpu, val);
	cpu->registers.pc+=2;
	return CYCLES_ACI;
}

uint8_t i8080_sub(intel8080_t *cpu)
{
	uint8_t source = SOURCE(cpu->current_op_code);
	uint8_t val = i8080_regread(cpu, source);
	i8080_gensub(cpu, val);
	cpu->registers.pc++;
	return CYCLES_SUB;
}

uint8_t i8080_sui(intel8080_t *cpu)
{
	i8080_gensub(cpu, read8(cpu->registers.pc+1));
	cpu->registers.pc+=2;
	return CYCLES_SUI;
}

uint8_t i8080_sbb(intel8080_t *cpu)
{
	uint8_t source = SOURCE(cpu->current_op_code);
	uint16_t val = i8080_regread(cpu, source);

	if(cpu->registers.flags & FLAGS_CARRY)
		val++;

	i8080_gensub(cpu, val);
	cpu->registers.pc++;
	return CYCLES_SBB;
}

uint8_t i8080_sbi(intel8080_t *cpu)
{
	uint16_t val;
	val = read8(cpu->registers.pc+1);
	if(cpu->registers.flags & FLAGS_CARRY)
		val++;
	i8080_gensub(cpu, val);
	cpu->registers.pc+=2;
	return CYCLES_SBI;
}

uint8_t i8080_inr(intel8080_t *cpu)
{
	uint8_t dest = DESTINATION(cpu->current_op_code);
	uint8_t val = i8080_regread(cpu, dest);

	if(i8080_check_half_carry(val, 1))
		i8080_set_flag(cpu, FLAGS_H);
	else
		i8080_clear_flag(cpu, FLAGS_H);

	i8080_regwrite(cpu, dest, val + 1);

	i8080_update_flags(cpu, dest, FLAGS_ZERO | FLAGS_PARITY | FLAGS_SIGN | FLAGS_H);
	cpu->registers.pc++;
	return CYCLES_INR;
}

uint8_t i8080_dcr(intel8080_t *cpu)
{
	uint8_t dest = DESTINATION(cpu->current_op_code);
	uint8_t val = i8080_regread(cpu, dest);

	if(i8080_check_half_carry(val, 0xff))
		i8080_set_flag(cpu, FLAGS_H);
	else
		i8080_clear_flag(cpu, FLAGS_H);

	i8080_regwrite(cpu, dest, val + 0xff);
	i8080_update_flags(cpu, dest, FLAGS_ZERO | FLAGS_PARITY | FLAGS_SIGN | FLAGS_H);
	cpu->registers.pc++;
	return CYCLES_DCR;
}

uint8_t i8080_inx(intel8080_t *cpu)
{
	uint8_t rp = RP(cpu->current_op_code);
	i8080_pairwrite(cpu, rp, i8080_pairread(cpu, rp) + 1);
	cpu->registers.pc++;
	return CYCLES_INX;
}

uint8_t i8080_dcx(intel8080_t *cpu)
{
	uint8_t rp = RP(cpu->current_op_code);
	cpu->registers.pc++;
	i8080_pairwrite(cpu, rp, i8080_pairread(cpu, rp) - 1);
	return CYCLES_DCX;
}

uint8_t i8080_dad(intel8080_t *cpu)
{
	uint8_t rp = RP(cpu->current_op_code);
	uint32_t val = i8080_pairread(cpu, rp);
	val += i8080_pairread(cpu, PAIR_HL);

	if(val > 0xffff)
		i8080_set_flag(cpu, FLAGS_CARRY);
	else
		i8080_clear_flag(cpu, FLAGS_CARRY);

	i8080_pairwrite(cpu, PAIR_HL, val & 0xffff);

	cpu->registers.pc++;

	return CYCLES_DAD;
}

uint8_t i8080_ana(intel8080_t *cpu)
{
	uint8_t source = SOURCE(cpu->current_op_code);

	cpu->registers.a &= i8080_regread(cpu, source);
	i8080_clear_flag(cpu, FLAGS_CARRY);
	i8080_update_flags(cpu, REGISTER_A, FLAGS_ZERO | FLAGS_SIGN | FLAGS_PARITY | FLAGS_CARRY | FLAGS_H);

	cpu->registers.pc++;
	return CYCLES_ANA;
}

uint8_t i8080_ani(intel8080_t *cpu)
{
	cpu->registers.a &= read8(cpu->registers.pc+1);
	i8080_clear_flag(cpu, FLAGS_CARRY);
	i8080_clear_flag(cpu, FLAGS_H);
	i8080_update_flags(cpu, REGISTER_A, FLAGS_ZERO | FLAGS_SIGN | FLAGS_PARITY | FLAGS_CARRY | FLAGS_H);

	cpu->registers.pc+=2;
	return CYCLES_ANI;
}

uint8_t i8080_ora(intel8080_t *cpu)
{
	uint8_t source = SOURCE(cpu->current_op_code);

	cpu->registers.a |= i8080_regread(cpu, source);
	i8080_clear_flag(cpu, FLAGS_CARRY);
	i8080_clear_flag(cpu, FLAGS_H);
	i8080_update_flags(cpu, REGISTER_A, FLAGS_ZERO | FLAGS_SIGN | FLAGS_PARITY | FLAGS_CARRY | FLAGS_H);

	cpu->registers.pc++;
	return CYCLES_ORA;
}

uint8_t i8080_ori(intel8080_t *cpu)
{
	cpu->registers.a |= read8(cpu->registers.pc+1);
	i8080_clear_flag(cpu, FLAGS_CARRY);
	i8080_clear_flag(cpu, FLAGS_H);
	i8080_update_flags(cpu, REGISTER_A, FLAGS_ZERO | FLAGS_SIGN | FLAGS_PARITY | FLAGS_CARRY | FLAGS_H);

	cpu->registers.pc+=2;
	return CYCLES_ORI;
}

uint8_t i8080_xra(intel8080_t *cpu)
{
	uint8_t source = SOURCE(cpu->current_op_code);

	cpu->registers.a ^= i8080_regread(cpu, source);
	i8080_clear_flag(cpu, FLAGS_CARRY);
	i8080_clear_flag(cpu, FLAGS_H);
	i8080_update_flags(cpu, REGISTER_A, FLAGS_ZERO | FLAGS_SIGN | FLAGS_PARITY | FLAGS_CARRY | FLAGS_H);

	cpu->registers.pc++;
	return CYCLES_XRA;
}

uint8_t i8080_xri(intel8080_t *cpu)
{
	cpu->registers.a ^= read8(cpu->registers.pc+1);
	i8080_clear_flag(cpu, FLAGS_CARRY);
	i8080_clear_flag(cpu, FLAGS_H);
	i8080_update_flags(cpu, REGISTER_A, FLAGS_ZERO | FLAGS_SIGN | FLAGS_PARITY | FLAGS_CARRY | FLAGS_H);

	cpu->registers.pc+=2;
	return CYCLES_XRI;
}

uint8_t i8080_ei(intel8080_t *cpu)
{
	cpu->registers.pc++;
	i8080_set_flag(cpu, FLAGS_IF);
	return CYCLES_EI;
}

uint8_t i8080_di(intel8080_t *cpu)
{
	cpu->registers.pc++;
	i8080_clear_flag(cpu, FLAGS_IF);
	return CYCLES_DI;
}

uint8_t i8080_xthl(intel8080_t *cpu)
{
	uint16_t temp = read16(cpu->registers.sp);

	write16(cpu->registers.sp, cpu->registers.hl);
	cpu->registers.hl = temp;
	cpu->registers.pc++;
	return CYCLES_XTHL;
}

uint8_t i8080_sphl(intel8080_t *cpu)
{
	cpu->registers.sp = cpu->registers.hl;
	cpu->registers.pc++;
	return CYCLES_SPHL;
}

uint8_t i8080_in(intel8080_t *cpu)
{
	static uint8_t character = 0;
	uint8_t port = read8(cpu->registers.pc + 1);

	switch(port)
	{
	case 0x00:
		cpu->registers.a = 0x00;
		break;
	case 0x1:
		cpu->cpuStatus |= STATUS_PORT_INPUT;
		cpu->registers.a = cpu->term_in();
		// cpu->term_out(cpu->registers.a);
		break;
	case 0x8:
		cpu->registers.a = cpu->disk_controller.disk_status();
		break;
	case 0x9:
		cpu->registers.a = cpu->disk_controller.sector();
		break;
	case 0xa:
		cpu->registers.a = cpu->disk_controller.read();
		break;
	case 0x10: // 2SIO port 1, status
		cpu->registers.a = 0x2; // bit 1 == transmit buffer empty
		if(!character)
		{
			character = cpu->term_in();
		}
		if(character)
		{
			cpu->registers.a |= 0x1;
		}
		break;
	case 0x11: // 2SIO port 1, read
		if(character)
		{
			cpu->registers.a = character;
			character = 0;
		}
		else
		{
			cpu->registers.a = cpu->term_in();
		}
		break;
	case 0xff: // Front panel switches
		cpu->registers.a = cpu->sense();
		break;
	default:
		cpu->registers.a = 0xff;
		cpu->registers.a = cpu->_sphere_port_in(port);
		//printf("IN PORT %x\n", cpu->data_bus);
		break;
	}

	cpu->registers.pc+=2;
	return CYCLES_IN;
}

uint8_t i8080_out(intel8080_t *cpu)
{
	uint8_t port = read8(cpu->registers.pc + 1);
	switch(port)
	{
	case 0x1:
		cpu->cpuStatus |= STATUS_PORT_OUTPUT;
		cpu->term_out(cpu->registers.a);
		break;
	case 0x8:
		cpu->disk_controller.disk_select(cpu->registers.a);
		break;
	case 0x9:
		cpu->disk_controller.disk_function(cpu->registers.a);
		break;
	case 0xa:
		cpu->disk_controller.write(cpu->registers.a);
		break;
	case 0x10:  // 2SIO port 1 control
		break;
	case 0x11: // 2sio port 1 write
		cpu->term_out(cpu->registers.a);
		break;
	default:
		cpu->_sphere_port_out(port,cpu->registers.a);
		// printf("OUT PORT %x, DATA: %x\n", read8(cpu->registers.pc + 1), cpu->registers.a);
		break;
	}
	cpu->registers.pc+=2;
	return CYCLES_OUT;
}

uint8_t i8080_push(intel8080_t *cpu)
{
	cpu->cpuStatus |= STATUS_STACK;
	uint8_t pair = RP(cpu->current_op_code);
	uint16_t val;

	if(pair == PAIR_SP)
		val = cpu->registers.af;
	else
		val = i8080_pairread(cpu, pair);

	cpu->registers.sp-=2;
	write16(cpu->registers.sp, val);

	cpu->registers.pc++;
	return CYCLES_PUSH;
}

uint8_t i8080_pop(intel8080_t *cpu)
{
	cpu->cpuStatus |= STATUS_STACK;
	uint8_t pair = RP(cpu->current_op_code);
	uint16_t val = read16(cpu->registers.sp);
	cpu->registers.sp+=2;
	if(pair == PAIR_SP)
		cpu->registers.af = val;
	else
		i8080_pairwrite(cpu, pair, val);

	cpu->registers.pc++;
	return CYCLES_POP;
}

uint8_t i8080_stc(intel8080_t *cpu)
{
	i8080_set_flag(cpu, FLAGS_CARRY);
	cpu->registers.pc++;
	return CYCLES_STC;
}

uint8_t i8080_cmc(intel8080_t *cpu)
{
	cpu->registers.flags ^= FLAGS_CARRY;
	cpu->registers.pc++;
	return CYCLES_CMC;
}

uint8_t i8080_rlc(intel8080_t *cpu)
{
	uint8_t high_bit = cpu->registers.a & 0x80;

	cpu->registers.a <<= 1;
	if(high_bit)
	{
		i8080_set_flag(cpu, FLAGS_CARRY);
		cpu->registers.a |= 1;
	}
	else
	{
		i8080_clear_flag(cpu, FLAGS_CARRY);
		cpu->registers.a &= ~1;
	}

	cpu->registers.pc++;
	return CYCLES_RAL;
}

uint8_t i8080_rrc(intel8080_t *cpu)
{
	uint8_t low_bit = cpu->registers.a & 1;

	cpu->registers.a >>= 1;

	if(low_bit)
	{
		i8080_set_flag(cpu, FLAGS_CARRY);
		cpu->registers.a |= 0x80;
	}
	else
	{
		cpu->registers.a &= ~0x80;
		i8080_clear_flag(cpu, FLAGS_CARRY);
	}

	cpu->registers.pc++;
	return CYCLES_RAR;
}

uint8_t i8080_ral(intel8080_t *cpu)
{
	uint8_t high_bit = cpu->registers.a & 0x80;
	cpu->registers.a <<= 1;

	if(cpu->registers.flags & FLAGS_CARRY)
		cpu->registers.a |= 1;
	else
		cpu->registers.a &= ~1;

	if(high_bit)
		i8080_set_flag(cpu, FLAGS_CARRY);
	else
		i8080_clear_flag(cpu, FLAGS_CARRY);

	cpu->registers.pc++;
	return CYCLES_RLC;
}

uint8_t i8080_rar(intel8080_t *cpu)
{
	uint8_t low_bit = cpu->registers.a & 1;

	cpu->registers.a >>= 1;

	if(cpu->registers.flags & FLAGS_CARRY)
		cpu->registers.a |= 0x80;
	else
		cpu->registers.a &= ~0x80;

	if(low_bit)
		i8080_set_flag(cpu, FLAGS_CARRY);
	else
		i8080_clear_flag(cpu, FLAGS_CARRY);

	cpu->registers.pc++;
	return CYCLES_RRC;
}

uint8_t i8080_jmp(intel8080_t *cpu)
{
	cpu->registers.pc = read16(cpu->registers.pc+1);
	return CYCLES_JMP;
}

uint8_t i8080_jccc(intel8080_t *cpu)
{
	uint8_t condition = CONDITION(cpu->current_op_code);

	if(i8080_check_condition(cpu, condition))
	{
		i8080_jmp(cpu);
	}
	else
	{
		cpu->registers.pc+=3;
	}

	return CYCLES_JMP;
}

uint8_t i8080_ret(intel8080_t *cpu)
{
	cpu->cpuStatus |= STATUS_STACK;
	cpu->registers.pc = read16(cpu->registers.sp);
	cpu->registers.sp+=2;
	return CYCLES_RET;
}

uint8_t i8080_rccc(intel8080_t *cpu)
{
	uint8_t condition = CONDITION(cpu->current_op_code);

	if(i8080_check_condition(cpu, condition))
	{
		i8080_ret(cpu);
	}
	else
	{
		cpu->registers.pc++;
	}
	return CYCLES_RET;
}

uint8_t i8080_rst(intel8080_t *cpu)
{
	cpu->cpuStatus |= STATUS_STACK;
	uint8_t vec = DESTINATION(cpu->current_op_code);

	cpu->registers.sp-=2;
	write16(cpu->registers.sp, cpu->registers.pc + 1);

	cpu->registers.pc = vec*8;

	return CYCLES_RET;
}

uint8_t i8080_call(intel8080_t *cpu)
{
	cpu->cpuStatus |= STATUS_STACK;
	cpu->registers.sp-=2;
	write16(cpu->registers.sp, cpu->registers.pc + 3);

	cpu->registers.pc = read16(cpu->registers.pc + 1);
	return CYCLES_JMP;
}

uint8_t i8080_cccc(intel8080_t *cpu)
{
	uint8_t condition = CONDITION(cpu->current_op_code);

	if(i8080_check_condition(cpu, condition))
	{
		i8080_call(cpu);
	}
	else
	{
		cpu->registers.pc+=3;
	}

	return CYCLES_CALL;
}

uint8_t i8080_pchl(intel8080_t *cpu)
{
	cpu->registers.pc = cpu->registers.hl;
	return CYCLES_PCHL;
}

uint8_t i8080_nop(intel8080_t *cpu)
{
	cpu->registers.pc++;

	return CYCLES_NOP;
}

uint8_t i8080_cma(intel8080_t *cpu)
{
	cpu->registers.a = ~cpu->registers.a;
	cpu->registers.pc++;

	return CYCLES_CMA;
}

uint8_t i8080_cmp(intel8080_t *cpu)
{
	uint8_t reg = SOURCE(cpu->current_op_code);

	i8080_compare(cpu, i8080_regread(cpu, reg));

	cpu->registers.pc++;
	return CYCLES_CMP;
}

uint8_t i8080_cpi(intel8080_t *cpu)
{
	i8080_compare(cpu, read8(cpu->registers.pc+1));
	cpu->registers.pc+=2;
	return CYCLES_CPI;
}

void i8080_fetch_next_op(intel8080_t *cpu)
{
	cpu->address_bus = cpu->registers.pc;
	i8080_mread(cpu);
}

uint8_t i8080_daa(intel8080_t *cpu)
{
	uint8_t val, add = 0;
	val = i8080_regread(cpu, REGISTER_A);

	if((val & 0xf) > 9 || cpu->registers.flags & FLAGS_H)
		add += 0x06;

	val += add;

	if(((val & 0xf0) >> 4) > 9 || cpu->registers.flags & FLAGS_CARRY)
		add += 0x60;

	i8080_genadd(cpu, add);

	cpu->registers.pc++;
	return CYCLES_DAA;
}

void i8080_cycle(intel8080_t *cpu)
{
	uint8_t op_code;
	cpu->cpuStatus = 0;
	i8080_fetch_next_op(cpu);

	op_code = cpu->current_op_code = cpu->data_bus;
	switch(op_code)
	{
		case 0x00: i8080_nop(cpu); break;
		case 0x01: i8080_lxi(cpu); break;
		case 0x02: i8080_stax(cpu); break;
		case 0x03: i8080_inx(cpu); break;
		case 0x04: i8080_inr(cpu); break;
		case 0x05: i8080_dcr(cpu); break;
		case 0x06: i8080_mvi(cpu); break;
		case 0x07: i8080_rlc(cpu); break;
		case 0x09: i8080_dad(cpu); break;
		case 0x0a: i8080_ldax(cpu); break;
		case 0x0b: i8080_dcx(cpu); break;
		case 0x0c: i8080_inr(cpu); break;
		case 0x0d: i8080_dcr(cpu); break;
		case 0x0e: i8080_mvi(cpu); break;
		case 0x0f: i8080_rrc(cpu); break;
		case 0x11: i8080_lxi(cpu); break;
		case 0x12: i8080_stax(cpu); break;
		case 0x13: i8080_inx(cpu); break;
		case 0x14: i8080_inr(cpu); break;
		case 0x15: i8080_dcr(cpu); break;
		case 0x16: i8080_mvi(cpu); break;
		case 0x17: i8080_ral(cpu); break;
		case 0x19: i8080_dad(cpu); break;
		case 0x1a: i8080_ldax(cpu); break;
		case 0x1b: i8080_dcx(cpu); break;
		case 0x1c: i8080_inr(cpu); break;
		case 0x1d: i8080_dcr(cpu); break;
		case 0x1e: i8080_mvi(cpu); break;
		case 0x1f: i8080_rar(cpu); break;
//		case 0x20: i8080_rim(cpu); break;
		case 0x21: i8080_lxi(cpu); break;
		case 0x22: i8080_shld(cpu); break;
		case 0x23: i8080_inx(cpu); break;
		case 0x24: i8080_inr(cpu); break;
		case 0x25: i8080_dcr(cpu); break;
		case 0x26: i8080_mvi(cpu); break;
		case 0x27: i8080_daa(cpu); break;
		case 0x29: i8080_dad(cpu); break;
		case 0x2a: i8080_lhld(cpu); break;
		case 0x2b: i8080_dcx(cpu); break;
		case 0x2c: i8080_inr(cpu); break;
		case 0x2d: i8080_dcr(cpu); break;
		case 0x2e: i8080_mvi(cpu); break;
		case 0x2f: i8080_cma(cpu); break;
//		case 0x30: i8080_sim(cpu); break;
		case 0x31: i8080_lxi(cpu); break;
		case 0x32: i8080_sta(cpu); break;
		case 0x33: i8080_inx(cpu); break;
		case 0x34: i8080_inr(cpu); break;
		case 0x35: i8080_dcr(cpu); break;
		case 0x36: i8080_mvi(cpu); break;
		case 0x37: i8080_stc(cpu); break;
		case 0x39: i8080_dad(cpu); break;
		case 0x3a: i8080_lda(cpu); break;
		case 0x3b: i8080_dcx(cpu); break;
		case 0x3c: i8080_inr(cpu); break;
		case 0x3d: i8080_dcr(cpu); break;
		case 0x3e: i8080_mvi(cpu); break;
		case 0x3f: i8080_cmc(cpu); break;
		case 0x40: i8080_mov(cpu); break;
		case 0x41: i8080_mov(cpu); break;
		case 0x42: i8080_mov(cpu); break;
		case 0x43: i8080_mov(cpu); break;
		case 0x44: i8080_mov(cpu); break;
		case 0x45: i8080_mov(cpu); break;
		case 0x46: i8080_mov(cpu); break;
		case 0x47: i8080_mov(cpu); break;
		case 0x48: i8080_mov(cpu); break;
		case 0x49: i8080_mov(cpu); break;
		case 0x4a: i8080_mov(cpu); break;
		case 0x4b: i8080_mov(cpu); break;
		case 0x4c: i8080_mov(cpu); break;
		case 0x4d: i8080_mov(cpu); break;
		case 0x4e: i8080_mov(cpu); break;
		case 0x4f: i8080_mov(cpu); break;
		case 0x50: i8080_mov(cpu); break;
		case 0x51: i8080_mov(cpu); break;
		case 0x52: i8080_mov(cpu); break;
		case 0x53: i8080_mov(cpu); break;
		case 0x54: i8080_mov(cpu); break;
		case 0x55: i8080_mov(cpu); break;
		case 0x56: i8080_mov(cpu); break;
		case 0x57: i8080_mov(cpu); break;
		case 0x58: i8080_mov(cpu); break;
		case 0x59: i8080_mov(cpu); break;
		case 0x5a: i8080_mov(cpu); break;
		case 0x5b: i8080_mov(cpu); break;
		case 0x5c: i8080_mov(cpu); break;
		case 0x5d: i8080_mov(cpu); break;
		case 0x5e: i8080_mov(cpu); break;
		case 0x5f: i8080_mov(cpu); break;
		case 0x60: i8080_mov(cpu); break;
		case 0x61: i8080_mov(cpu); break;
		case 0x62: i8080_mov(cpu); break;
		case 0x63: i8080_mov(cpu); break;
		case 0x64: i8080_mov(cpu); break;
		case 0x65: i8080_mov(cpu); break;
		case 0x66: i8080_mov(cpu); break;
		case 0x67: i8080_mov(cpu); break;
		case 0x68: i8080_mov(cpu); break;
		case 0x69: i8080_mov(cpu); break;
		case 0x6a: i8080_mov(cpu); break;
		case 0x6b: i8080_mov(cpu); break;
		case 0x6c: i8080_mov(cpu); break;
		case 0x6d: i8080_mov(cpu); break;
		case 0x6e: i8080_mov(cpu); break;
		case 0x6f: i8080_mov(cpu); break;
		case 0x70: i8080_mov(cpu); break;
		case 0x71: i8080_mov(cpu); break;
		case 0x72: i8080_mov(cpu); break;
		case 0x73: i8080_mov(cpu); break;
		case 0x74: i8080_mov(cpu); break;
		case 0x75: i8080_mov(cpu); break;
//		case 0x76: i8080_hlt(cpu); break;
		case 0x77: i8080_mov(cpu); break;
		case 0x78: i8080_mov(cpu); break;
		case 0x79: i8080_mov(cpu); break;
		case 0x7a: i8080_mov(cpu); break;
		case 0x7b: i8080_mov(cpu); break;
		case 0x7c: i8080_mov(cpu); break;
		case 0x7d: i8080_mov(cpu); break;
		case 0x7e: i8080_mov(cpu); break;
		case 0x7f: i8080_mov(cpu); break;
		case 0x80: i8080_add(cpu); break;
		case 0x81: i8080_add(cpu); break;
		case 0x82: i8080_add(cpu); break;
		case 0x83: i8080_add(cpu); break;
		case 0x84: i8080_add(cpu); break;
		case 0x85: i8080_add(cpu); break;
		case 0x86: i8080_add(cpu); break;
		case 0x87: i8080_add(cpu); break;
		case 0x88: i8080_adc(cpu); break;
		case 0x89: i8080_adc(cpu); break;
		case 0x8a: i8080_adc(cpu); break;
		case 0x8b: i8080_adc(cpu); break;
		case 0x8c: i8080_adc(cpu); break;
		case 0x8d: i8080_adc(cpu); break;
		case 0x8e: i8080_adc(cpu); break;
		case 0x8f: i8080_adc(cpu); break;
		case 0x90: i8080_sub(cpu); break;
		case 0x91: i8080_sub(cpu); break;
		case 0x92: i8080_sub(cpu); break;
		case 0x93: i8080_sub(cpu); break;
		case 0x94: i8080_sub(cpu); break;
		case 0x95: i8080_sub(cpu); break;
		case 0x96: i8080_sub(cpu); break;
		case 0x97: i8080_sub(cpu); break;
		case 0x98: i8080_sbb(cpu); break;
		case 0x99: i8080_sbb(cpu); break;
		case 0x9a: i8080_sbb(cpu); break;
		case 0x9b: i8080_sbb(cpu); break;
		case 0x9c: i8080_sbb(cpu); break;
		case 0x9d: i8080_sbb(cpu); break;
		case 0x9e: i8080_sbb(cpu); break;
		case 0x9f: i8080_sbb(cpu); break;
		case 0xa0: i8080_ana(cpu); break;
		case 0xa1: i8080_ana(cpu); break;
		case 0xa2: i8080_ana(cpu); break;
		case 0xa3: i8080_ana(cpu); break;
		case 0xa4: i8080_ana(cpu); break;
		case 0xa5: i8080_ana(cpu); break;
		case 0xa6: i8080_ana(cpu); break;
		case 0xa7: i8080_ana(cpu); break;
		case 0xa8: i8080_xra(cpu); break;
		case 0xa9: i8080_xra(cpu); break;
		case 0xaa: i8080_xra(cpu); break;
		case 0xab: i8080_xra(cpu); break;
		case 0xac: i8080_xra(cpu); break;
		case 0xad: i8080_xra(cpu); break;
		case 0xae: i8080_xra(cpu); break;
		case 0xaf: i8080_xra(cpu); break;
		case 0xb0: i8080_ora(cpu); break;
		case 0xb1: i8080_ora(cpu); break;
		case 0xb2: i8080_ora(cpu); break;
		case 0xb3: i8080_ora(cpu); break;
		case 0xb4: i8080_ora(cpu); break;
		case 0xb5: i8080_ora(cpu); break;
		case 0xb6: i8080_ora(cpu); break;
		case 0xb7: i8080_ora(cpu); break;
		case 0xb8: i8080_cmp(cpu); break;
		case 0xb9: i8080_cmp(cpu); break;
		case 0xba: i8080_cmp(cpu); break;
		case 0xbb: i8080_cmp(cpu); break;
		case 0xbc: i8080_cmp(cpu); break;
		case 0xbd: i8080_cmp(cpu); break;
		case 0xbe: i8080_cmp(cpu); break;
		case 0xbf: i8080_cmp(cpu); break;
		case 0xc0: i8080_rccc(cpu); break;
		case 0xc1: i8080_pop(cpu); break;
		case 0xc2: i8080_jccc(cpu); break;
		case 0xc3: i8080_jmp(cpu); break;
		case 0xc4: i8080_cccc(cpu); break;
		case 0xc5: i8080_push(cpu); break;
		case 0xc6: i8080_adi(cpu); break;
		case 0xc7: i8080_rst(cpu); break;
		case 0xc8: i8080_rccc(cpu); break;
		case 0xc9: i8080_ret(cpu); break;
		case 0xca: i8080_jccc(cpu); break;
		case 0xcc: i8080_cccc(cpu); break;
		case 0xcd: i8080_call(cpu); break;
		case 0xce: i8080_aci(cpu); break;
		case 0xcf: i8080_rst(cpu); break;
		case 0xd0: i8080_rccc(cpu); break;
		case 0xd1: i8080_pop(cpu); break;
		case 0xd2: i8080_jccc(cpu); break;
		case 0xd3: i8080_out(cpu); break;
		case 0xd4: i8080_cccc(cpu); break;
		case 0xd5: i8080_push(cpu); break;
		case 0xd6: i8080_sui(cpu); break;
		case 0xd7: i8080_rst(cpu); break;
		case 0xd8: i8080_rccc(cpu); break;
		case 0xda: i8080_jccc(cpu); break;
		case 0xdb: i8080_in(cpu); break;
		case 0xdc: i8080_cccc(cpu); break;
		case 0xde: i8080_sbi(cpu); break;
		case 0xdf: i8080_rst(cpu); break;
		case 0xe0: i8080_rccc(cpu); break;
		case 0xe1: i8080_pop(cpu); break;
		case 0xe2: i8080_jccc(cpu); break;
		case 0xe3: i8080_xthl(cpu); break;
		case 0xe4: i8080_cccc(cpu); break;
		case 0xe5: i8080_push(cpu); break;
		case 0xe6: i8080_ani(cpu); break;
		case 0xe7: i8080_rst(cpu); break;
		case 0xe8: i8080_rccc(cpu); break;
		case 0xe9: i8080_pchl(cpu); break;
		case 0xea: i8080_jccc(cpu); break;
		case 0xeb: i8080_xchg(cpu); break;
		case 0xec: i8080_cccc(cpu); break;
		case 0xee: i8080_xri(cpu); break;
		case 0xef: i8080_rst(cpu); break;
		case 0xf0: i8080_rccc(cpu); break;
		case 0xf1: i8080_pop(cpu); break;
		case 0xf2: i8080_jccc(cpu); break;
		case 0xf3: i8080_di(cpu); break;
		case 0xf4: i8080_cccc(cpu); break;
		case 0xf5: i8080_push(cpu); break;
		case 0xf6: i8080_ori(cpu); break;
		case 0xf7: i8080_rst(cpu); break;
		case 0xf8: i8080_rccc(cpu); break;
		case 0xf9: i8080_sphl(cpu); break;
		case 0xfa: i8080_jccc(cpu); break;
		case 0xfb: i8080_ei(cpu); break;
		case 0xfc: i8080_cccc(cpu); break;
		case 0xfe: i8080_cpi(cpu); break;
		case 0xff: i8080_rst(cpu); break;
	}
}
