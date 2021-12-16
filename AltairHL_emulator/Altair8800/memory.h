#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "types.h"

//#ifdef ARDUINO
//
//#include "../arduino/pins.h"
//
//inline void led_out(uint16_t address, uint8_t data, uint8_t status)
//{
//	// This is needed since the data comes on the MISO line and we need it on the MOSI line.
//	// TODO: Do something smart about it
//	SPI.setBitOrder(LSBFIRST);
//	digitalWrite(LEDS_LATCH, LOW);
//        SPI.transfer(address & 0xff); // 24 bit address
//	SPI.transfer((address >> 8) & 0xff);
//	SPI.transfer(data);
//	SPI.transfer(0x00); // Status LEDs
//	digitalWrite(LEDS_LATCH, HIGH); // Disable LED shift registers
//	SPI.setBitOrder(MSBFIRST);
//}
//
//inline uint8_t read8(uint16_t address)
//{
//        uint8_t data;
//        digitalWrite(MEMORY_CS, LOW);
//
//        SPI.transfer(3); // read byte
//        SPI.transfer(0);
//        SPI.transfer((address >> 8) & 0xff);
//        SPI.transfer(address & 0xff); // 24 bit address
//        data = SPI.transfer(0x00); // data
//        digitalWrite(MEMORY_CS, HIGH);
//
//	led_out(address, data, 0x00);
//
//        return data;
//}
//
//inline void write8(uint16_t address, uint8_t val)
//{
//        digitalWrite(MEMORY_CS, LOW);
//
//        SPI.transfer(2); // write byte
//        SPI.transfer(0);
//        SPI.transfer((address >> 8) & 0xff);
//        SPI.transfer(address & 0xff); // 24 bit address
//        SPI.transfer(val); // data
//        digitalWrite(MEMORY_CS, HIGH);
//
//	led_out(address, val, 0x00);
//}
//
//#else
#include "sphere_panel.h"


void load4kRom(uint16_t address);
uint8_t read8(uint16_t address);
void write8(uint16_t address, uint8_t val);
uint16_t read16(uint16_t address);
void write16(uint16_t address, uint16_t val);

#endif
