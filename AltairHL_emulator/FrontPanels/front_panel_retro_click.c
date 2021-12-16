/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "front_panel_retro_click.h"

#ifdef ALTAIR_FRONT_PANEL_RETRO_CLICK

void init_altair_hardware(void)
{

    if (!as1115_init(i2c_as1115_retro.fd,  &retro_click, 1)) {
        dx_terminate(APP_EXIT_RETRO_CLICK_OPEN);
        return;
    }

    as1115_clear(&retro_click);
}

/// <summary>
/// Check what cpu_operating_mode click 4x4 keys should be in.
/// </summary>
void check_click_4x4key_mode_button(void)
{
    static GPIO_Value_Type buttonBState = GPIO_Value_High;

    if (dx_gpioStateGet(&buttonB, &buttonBState)) {
        click_4x4_key_mode = (click_4x4_key_mode == CONTROL_MODE && cpu_operating_mode == CPU_STOPPED) ? INPUT_MODE : CONTROL_MODE;

        renderText = true;

        gfx_load_character(click_4x4_key_mode == CONTROL_MODE ? 'C' : 'I', retro_click.bitmap);

        gfx_rotate_counterclockwise(retro_click.bitmap, 1, 1, retro_click.bitmap);
        gfx_reverse_panel(retro_click.bitmap);
        gfx_rotate_counterclockwise(retro_click.bitmap, 1, 1, retro_click.bitmap);

        as1115_panel_write(&retro_click);

        dx_timerOneShotSet(&turnOffNotificationsTimer, &(struct timespec){1, 0});
    }
}

void turn_off_notifications_handler(EventLoopTimer *eventLoopTimer)
{
    if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0) {
        dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
        return;
    }
    renderText = false;
}

void read_altair_panel_switches(void (*process_control_panel_commands)(void))
{
    const ALTAIR_COMMAND commandMap[] = {STOP_CMD, SINGLE_STEP, EXAMINE, DEPOSIT, RUN_CMD, NOP, EXAMINE_NEXT, DEPOSIT_NEXT};
    const char notifyMap[] = {'s', 'S', 'E', 'D', 'r', 'n', 'e', 'd'};
    char controlChar = ' ';

    check_click_4x4key_mode_button();

    uint8_t button_pressed = as1115_get_btn_position(&retro_click);

    if (button_pressed != 0 && click_4x4_key_mode == INPUT_MODE && cpu_operating_mode == CPU_STOPPED) {
        //button_pressed = buttonMap[button_pressed - 1]; // remap the button inputs

        // Log_Debug("0x%08lx\n", key4x4.bitmap);
        // Log_Debug("Button %d pressed.\n", buttonMap[button_pressed - 1]);

        if (bus_switches & (1UL << (16 - button_pressed))) // mask in the address
            bus_switches &= (uint16_t)~(1UL << (16 - button_pressed));
        else
            bus_switches |= (uint16_t)(1UL << (16 - button_pressed));

        // Log_Debug("Input data: 0x%04lx\n", bus_switches);

        uint8_t i8080_instruction_size = 0;
        char address_bus_high_byte[9];
        char address_bus_low_byte[9];

        uint8_to_binary((uint8_t)(bus_switches >> 8), address_bus_high_byte, sizeof(address_bus_high_byte));
        uint8_to_binary((uint8_t)(bus_switches), address_bus_low_byte, sizeof(address_bus_low_byte));

        snprintf(msgBuffer, sizeof(msgBuffer), "\r\n%15s: %s %s (0x%04x), %s (%d byte instruction)", "Input", address_bus_high_byte, address_bus_low_byte, bus_switches,
                 get_i8080_instruction_name((uint8_t)bus_switches, &i8080_instruction_size), i8080_instruction_size);
        queue_mqtt_message(msgBuffer, strlen(msgBuffer));

        if (console_fd != -1) {
            write(console_fd, msgBuffer, strlen(msgBuffer));
        }
    }

    if (button_pressed != 0 && click_4x4_key_mode == CONTROL_MODE && button_pressed < 9) {
        if (cpu_operating_mode == CPU_STOPPED && button_pressed != 1) // if CPU stopped then ignore new attempt to stop
        {
            cmd_switches = commandMap[button_pressed - 1];
            controlChar = notifyMap[button_pressed - 1];
        } else if (cpu_operating_mode == CPU_RUNNING && button_pressed == 1) {
            cmd_switches = commandMap[button_pressed - 1];
            controlChar = notifyMap[button_pressed - 1];
        } else {
            cmd_switches = 0x00;
        }

        if (controlChar != ' ') {
            renderText = true;

            gfx_load_character(controlChar, retro_click.bitmap);

            gfx_rotate_counterclockwise(retro_click.bitmap, 1, 1, retro_click.bitmap);
            gfx_reverse_panel(retro_click.bitmap);
            gfx_rotate_counterclockwise(retro_click.bitmap, 1, 1, retro_click.bitmap);

            as1115_panel_write(&retro_click);

            process_control_panel_commands();

            dx_timerOneShotSet(&turnOffNotificationsTimer, &(struct timespec){0, OneMS * 300});
        }
    }
}

void update_panel_status_leds(uint8_t status, uint8_t data, uint16_t bus)
{
    if (!renderText) {
        switch (click_4x4_key_mode) {
        case CONTROL_MODE:
            retro_click.bitmap[0] = status;
            retro_click.bitmap[1] = 0;
            retro_click.bitmap[2] = 0;
            retro_click.bitmap[3] = data;
            retro_click.bitmap[4] = 0;
            retro_click.bitmap[5] = 0;
            retro_click.bitmap[6] = (uint8_t)(bus >> 8);
            retro_click.bitmap[7] = (uint8_t)bus;

            gfx_reverse_panel(retro_click.bitmap);

            as1115_panel_write(&retro_click);
            break;
        case INPUT_MODE:
            retro_click.bitmap[6] = (unsigned char)(bus_switches >> 8);
            retro_click.bitmap[7] = (unsigned char)bus_switches;
            retro_click.bitmap[0] = retro_click.bitmap[1] = retro_click.bitmap[2] = retro_click.bitmap[3] = retro_click.bitmap[4] = retro_click.bitmap[5] = 0;

            //gfx_reverse_panel(retro_click.bitmap);

            as1115_panel_write(&retro_click);
            break;
        default:
            break;
        }
    }
}

#endif // ALTAIR_FRONT_PANEL_RETRO_CLICK