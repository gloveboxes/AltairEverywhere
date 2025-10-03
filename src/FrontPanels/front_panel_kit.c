/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "front_panel_kit.h"

#include "dx_gpio.h"
#include <dx_utilities.h>
#include <spidev_lib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SWITCHES_LOAD        05
#define SWITCHES_CHIP_SELECT 00
#define LED_MASTER_RESET     22
#define LED_STORE            27
#define LED_OUTPUT_ENABLE    17

// clang-format off
static DX_GPIO_BINDING gpio_switches_load = {
    .chip_number = 0,
    .line_number = SWITCHES_LOAD,
    .initial_state = DX_GPIO_HIGH,
    .direction = DX_GPIO_OUTPUT,
    .name = "altair_switches_load"};

static DX_GPIO_BINDING gpio_switches_chip_select = {
    .chip_number = 0,
    .line_number = SWITCHES_CHIP_SELECT,
    .initial_state = DX_GPIO_HIGH,
    .direction = DX_GPIO_OUTPUT,
    .name = "altair_switches_chip_select"};

static DX_GPIO_BINDING gpio_led_master_reset = {
    .chip_number = 0,
    .line_number = LED_MASTER_RESET,
    .initial_state = DX_GPIO_HIGH,
    .direction = DX_GPIO_OUTPUT,
    .name = "altair_led_master_reset"};

static DX_GPIO_BINDING gpio_led_store = {
    .chip_number = 0,
    .line_number = LED_STORE,
    .initial_state = DX_GPIO_HIGH,
    .direction = DX_GPIO_OUTPUT,
    .name = "altair_led_store"};

static DX_GPIO_BINDING gpio_output_enable = {
    .chip_number = 0,
    .line_number = LED_OUTPUT_ENABLE,
    .initial_state = DX_GPIO_LOW,
    .direction = DX_GPIO_OUTPUT,
    .name = "altair_output_enable"};
// clang-format on

static int altair_spi_fd = -1;
static const uint8_t reverse_lut[16] = {0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe, 0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf};

static bool kit_initialized;

static DX_GPIO_BINDING *gpio_bindings[] = {
    &gpio_switches_load,
    &gpio_switches_chip_select,
    &gpio_led_master_reset,
    &gpio_led_store,
    &gpio_output_enable,
};

static void close_resources(void)
{
    for (size_t i = 0; i < NELEMS(gpio_bindings); ++i)
    {
        dx_gpioClose(gpio_bindings[i]);
    }

    if (altair_spi_fd != -1)
    {
        spi_close(altair_spi_fd);
        altair_spi_fd = -1;
    }

    kit_initialized = false;
}

bool kit_front_panel_init(void)
{
    if (kit_initialized)
    {
        return true;
    }

    for (size_t i = 0; i < NELEMS(gpio_bindings); ++i)
    {
        if (!dx_gpioOpen(gpio_bindings[i]))
        {
            close_resources();
            return false;
        }
    }

    spi_config_t spi_config;

    spi_config.mode          = 2;
    spi_config.speed         = 5000000;
    spi_config.delay         = 1;
    spi_config.bits_per_word = 8;

    if ((altair_spi_fd = spi_open("/dev/spidev0.0", spi_config)) == -1)
    {
        close_resources();
        return false;
    }

    kit_front_panel_io(0xff, 0xff, 0xffff, NULL);
    nanosleep(&(struct timespec){0, 500 * ONE_MS}, NULL);
    kit_front_panel_io(0xaa, 0xaa, 0xaaaa, NULL);
    nanosleep(&(struct timespec){0, 500 * ONE_MS}, NULL);

    kit_initialized = true;
    return true;
}

void kit_front_panel_shutdown(void)
{
    close_resources();
}

void kit_front_panel_set_brightness(int brightness)
{
    (void)brightness;
}

bool kit_front_panel_handle_led_matrix_output(int port_number, uint8_t data, char *buffer, size_t buffer_length, size_t *handled_length)
{
    (void)port_number;
    (void)data;
    (void)buffer;
    (void)buffer_length;
    if (handled_length)
    {
        *handled_length = 0;
    }
    return false;
}

void kit_front_panel_io(uint8_t status, uint8_t data, uint16_t bus, void (*process_control_panel_commands)(void))
{
    static ALTAIR_COMMAND last_command = NOP;
    uint16_t address                   = 0;
    uint8_t cmd                        = 0;

    uint8_t in[3];
    uint32_t out = status << 24 | data << 16 | bus;

    if (altair_spi_fd == -1)
    {
        return;
    }

    memset(in, 0x00, sizeof(in));

    dx_gpioStateSet(&gpio_led_store, DX_GPIO_LOW);
    dx_gpioStateSet(&gpio_switches_chip_select, DX_GPIO_LOW);
    dx_gpioStateSet(&gpio_switches_load, DX_GPIO_LOW);
    dx_gpioStateSet(&gpio_switches_load, DX_GPIO_HIGH);

    int bytes = spi_xfer(altair_spi_fd, (uint8_t *)&out, 4, in, 3);

    dx_gpioStateSet(&gpio_switches_chip_select, DX_GPIO_HIGH);
    dx_gpioStateSet(&gpio_led_store, DX_GPIO_HIGH);

    if (bytes == 4)
    {
        out = 0;
        memcpy(&out, in, 3);

        cmd = (out >> 16) & 0xff;

        address = out & 0xffff;
        address = reverse_lut[(address & 0xf000) >> 12] << 8 | reverse_lut[(address & 0x0f00) >> 8] << 12 | reverse_lut[(address & 0xf0) >> 4] |
                  reverse_lut[address & 0xf] << 4;
        address = (uint16_t)~address;

        bus_switches = address;

        if (cmd != last_command)
        {
            last_command = cmd;
            cmd_switches = cmd;
            if (process_control_panel_commands && cmd)
            {
                process_control_panel_commands();
            }
        }
    }
}
