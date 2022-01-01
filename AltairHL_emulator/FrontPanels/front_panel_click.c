/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "front_panel_click.h"

#ifdef ALTAIR_FRONT_PANEL_CLICK

void init_altair_hardware(void) {
	max7219_init(&panel8x8, 1);

	if (panel8x8.handle == -1) {
		dx_terminate(APP_EXIT_PANEL8X8_SPI_OPEN);
		return;
	}

	max7219_clear(&panel8x8);

	c4x4key_init(&key4x4);
	if (key4x4.handle == -1) {
		dx_terminate(APP_EXIT_KEY4X4_SPI_OPEN);
		return;
	}
}

/// <summary>
/// Check what cpu_operating_mode click 4x4 keys should be in. 
/// </summary>
void check_click_4x4key_mode_button(void) {
	static GPIO_Value_Type buttonBState = GPIO_Value_High;

	if (dx_gpioStateGet(&buttonB, &buttonBState)) {
		click_4x4_key_mode = (click_4x4_key_mode == CONTROL_MODE && cpu_operating_mode == CPU_STOPPED) ? INPUT_MODE : CONTROL_MODE;

		renderText = true;

		gfx_load_character(click_4x4_key_mode == CONTROL_MODE ? 'C' : 'I', panel8x8.bitmap);
		gfx_reverse_panel(panel8x8.bitmap);
		gfx_rotate_counterclockwise(panel8x8.bitmap, 1, 1, panel8x8.bitmap);
		gfx_reverse_panel(panel8x8.bitmap);
		max7219_panel_write(&panel8x8);

		dx_timerOneShotSet(&turnOffNotificationsTimer, &(struct timespec){1, 0});
	}
}

void turn_off_notifications_handler(EventLoopTimer* eventLoopTimer) {
	if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0) {
		dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
		return;
	}
	renderText = false;
}

void read_altair_panel_switches(void (*process_control_panel_commands)(void)) {
	const uint8_t buttonMap[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 15, 16, 13 };
	const ALTAIR_COMMAND commandMap[] = { STOP_CMD, SINGLE_STEP, EXAMINE, DEPOSIT, RUN_CMD, NOP, EXAMINE_NEXT, DEPOSIT_NEXT };
	const char notifyMap[] = { 's', 'S', 'E', 'D', 'r', 'n', 'e', 'd' };
	char controlChar = ' ';

	check_click_4x4key_mode_button();

	uint8_t button_pressed = c4x4key_get_btn_position(&key4x4);

	if (button_pressed != 0 && click_4x4_key_mode == INPUT_MODE && cpu_operating_mode == CPU_STOPPED) {
		button_pressed = buttonMap[button_pressed - 1];		// remap the button inputs

		//Log_Debug("0x%08lx\n", key4x4.bitmap);
		//Log_Debug("Button %d pressed.\n", buttonMap[button_pressed - 1]);

		if (bus_switches & (1UL << (16 - button_pressed)))	// mask in the address
			bus_switches &= ~(1UL << (16 - button_pressed));
		else
			bus_switches |= (1UL << (16 - button_pressed));

		//Log_Debug("Input data: 0x%04lx\n", bus_switches);

		uint8_t i8080_instruction_size = 0;
		char address_bus_high_byte[9];
		char address_bus_low_byte[9];

		uint8_to_binary((uint8_t)(bus_switches >> 8), address_bus_high_byte, sizeof(address_bus_high_byte));
		uint8_to_binary((uint8_t)(bus_switches), address_bus_low_byte, sizeof(address_bus_low_byte));

		snprintf(msgBuffer, sizeof(msgBuffer), "\r\n%15s: %s %s (0x%04x), %s (%d byte instruction)", "Input",
			address_bus_high_byte, address_bus_low_byte, bus_switches,
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

			gfx_load_character(controlChar, panel8x8.bitmap);
			gfx_reverse_panel(panel8x8.bitmap);
			gfx_rotate_counterclockwise(panel8x8.bitmap, 1, 1, panel8x8.bitmap);
			gfx_reverse_panel(panel8x8.bitmap);
			max7219_panel_write(&panel8x8);

			process_control_panel_commands();

			dx_timerOneShotSet(&turnOffNotificationsTimer, &(struct timespec){0, OneMS * 300});
		}
	}
}

void update_panel_status_leds(uint8_t status, uint8_t data, uint16_t bus) {
	if (!renderText) {
		switch (click_4x4_key_mode) {
		case CONTROL_MODE:
			panel8x8.bitmap[0] = status;
			panel8x8.bitmap[1] = 0;
			panel8x8.bitmap[2] = 0;
			panel8x8.bitmap[3] = data;
			panel8x8.bitmap[4] = 0;
			panel8x8.bitmap[5] = 0;
			panel8x8.bitmap[6] = (uint8_t)(bus >> 8);
			panel8x8.bitmap[7] = (uint8_t)bus;

			gfx_rotate_counterclockwise(panel8x8.bitmap, 1, 1, panel8x8.bitmap);
			max7219_panel_write(&panel8x8);
			break;
		case INPUT_MODE:
			panel8x8.bitmap[6] = (unsigned char)(bus_switches >> 8);
			panel8x8.bitmap[7] = (unsigned char)bus_switches;
			panel8x8.bitmap[0] = panel8x8.bitmap[1] = panel8x8.bitmap[2] = panel8x8.bitmap[3] = panel8x8.bitmap[4] = panel8x8.bitmap[5] = 0;

			gfx_reverse_panel(panel8x8.bitmap);
			gfx_rotate_counterclockwise(panel8x8.bitmap, 1, 1, panel8x8.bitmap);
			max7219_panel_write(&panel8x8);
			break;
		default:
			break;
		}
	}
}

#endif // ALTAIR_FRONT_PANEL_CLICK