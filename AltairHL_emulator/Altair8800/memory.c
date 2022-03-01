#include "memory.h"

extern uint8_t memory[64 * 1024];

uint8_t read8(uint16_t address)
{
    return memory[address];
}

void write8(uint16_t address, uint8_t val)
{
    memory[address] = val;
}

// #endif

uint16_t read16(uint16_t address)
{
    uint16_t result = read8(address);
    result |= read8(address + 1) << 8;
    return result;
}

void write16(uint16_t address, uint16_t val)
{
    write8(address, val & 0xff);
    write8(address + 1, (val >> 8) & 0xff);
}
