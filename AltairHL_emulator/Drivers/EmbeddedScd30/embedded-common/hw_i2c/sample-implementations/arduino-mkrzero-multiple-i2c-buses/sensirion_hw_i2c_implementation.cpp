/*
 * Copyright (c) 2019, Sensirion AG
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Sensirion AG nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "sensirion_arch_config.h"
#include "sensirion_i2c.h"

// needed for delay() routine
#include <Arduino.h>
#include <Wire.h>

/*
 * INSTRUCTIONS
 * ============
 *
 * Check out the following link to create other i2c buses on SAMD21 hardware:
 * https://learn.adafruit.com/using-atsamd21-sercom-to-add-more-spi-i2c-serial-ports/creating-a-new-wire#
 *
 * ERRATA
 * ============
 * This implementation was built and tested with "Arduino SAMD Boards" 1.6.20
 * -- THE SECOND BUS DOES NOT WORK WHEN BUILT WITH 1.8.1 --
 */

#define I2C_1_PIN_SDA 26
#define I2C_1_PIN_SCL 27

TwoWire Wire2(&sercom2, I2C_1_PIN_SDA, I2C_1_PIN_SCL);
static TwoWire *const i2c_bus[] = {&Wire, &Wire2};
static uint8_t selected_i2c_bus = 0;

void SERCOM2_Handler(void) {
    Wire2.onService();
}

/**
 * Select the current i2c bus by index.
 * All following i2c operations will be directed at that bus.
 *
 * THE IMPLEMENTATION IS OPTIONAL ON SINGLE-BUS SETUPS (all sensors on the same
 * bus)
 *
 * @param bus_idx   Bus index to select
 * @returns         0 on success, an error code otherwise
 */
int16_t sensirion_i2c_select_bus(uint8_t bus_idx) {
    if (bus_idx >= (sizeof(i2c_bus) / sizeof(i2c_bus[0])))
        return -1;
    selected_i2c_bus = bus_idx;
    return 0;
}

/**
 * Initialize all hard- and software components that are needed for the I2C
 * communication. After this function has been called, the functions
 * i2c_read() and i2c_write() must succeed.
 */
void sensirion_i2c_init(void) {
    switch (selected_i2c_bus) {
        case 0:
            i2c_bus[selected_i2c_bus]->begin();
            break;

        case 1:
            pinPeripheral(I2C_1_PIN_SDA, PIO_SERCOM);
            pinPeripheral(I2C_1_PIN_SCL, PIO_SERCOM);
            i2c_bus[selected_i2c_bus]->begin();
            break;
    }
}

/**
 * Release all resources initialized by sensirion_i2c_init().
 */
void sensirion_i2c_release(void) {
    i2c_bus[selected_i2c_bus]->end();
}

int8_t sensirion_i2c_read(uint8_t address, uint8_t *data, uint16_t count) {
    uint8_t rxByteCount = 0;

    i2c_bus[selected_i2c_bus]->requestFrom(address, count);

    while (i2c_bus[selected_i2c_bus]->available()) {
        data[rxByteCount++] = i2c_bus[selected_i2c_bus]->read();
        if (rxByteCount >= count)
            break;
    }

    return 0;
}

int8_t sensirion_i2c_write(uint8_t address, const uint8_t *data,
                           uint16_t count) {
    i2c_bus[selected_i2c_bus]->beginTransmission(address);
    i2c_bus[selected_i2c_bus]->write(data, count);
    i2c_bus[selected_i2c_bus]->endTransmission();

    return 0;
}

/**
 * Sleep for a given number of microseconds. The function should delay the
 * execution for at least the given time, but may also sleep longer.
 *
 * @param useconds the sleep time in microseconds
 */
void sensirion_sleep_usec(uint32_t useconds) {
    delay((useconds / 1000) + 1);
}
