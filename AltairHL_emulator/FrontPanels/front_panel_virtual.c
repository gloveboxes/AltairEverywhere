/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "front_panel_virtual.h"

static const char* too_many_switches = "\r\nError: Number of input switches must be less that or equal to 16.";
static const char* invalid_switches = "\r\nError: Input switches must be either 0 or 1.";

static bool validate_input_data(const char* command) {
	size_t len = strlen(command);
	if (len > 16) {
		queue_mqtt_message(too_many_switches, strlen(too_many_switches));
		return false;
	}
	for (size_t i = 0; i < len; i++) {
		if (!(command[i] == '1' || command[i] == '0')) {
			queue_mqtt_message(invalid_switches, strlen(invalid_switches));
			return false;
		}
	}
	return true;
}

static void publish_virtual_input_data(void) {
	uint8_t i8080_instruction_size = 0;
	char address_bus_high_byte[9];
	char address_bus_low_byte[9];

	uint8_to_binary((uint8_t)(bus_switches >> 8), address_bus_high_byte, sizeof(address_bus_high_byte));
	uint8_to_binary((uint8_t)(bus_switches), address_bus_low_byte, sizeof(address_bus_low_byte));

	snprintf(msgBuffer, sizeof(msgBuffer), "\r\n%15s: %s %s (0x%04x), %s (%d byte instruction)", "Input",
		address_bus_high_byte, address_bus_low_byte, bus_switches,
		get_i8080_instruction_name((uint8_t)bus_switches, &i8080_instruction_size), i8080_instruction_size);
	queue_mqtt_message(msgBuffer, strlen(msgBuffer));
}

static void process_virtual_switches(const char* command, void (*process_control_panel_commands)(void)) {
	uint16_t temp_bus_switches = 0;
	uint16_t mask = 1;

	if (validate_input_data(command)) {
		size_t len = strlen(command);
		for (size_t i = len; i > 0; i--) {
			temp_bus_switches |= (command[i - 1] == '1' ? mask * 1 : 0);
			mask <<= 1;
		}
		bus_switches = temp_bus_switches;
		publish_virtual_input_data();
	}
}

void process_virtual_input(const char* command, void (*process_control_panel_commands)(void)) {
	if (strcmp(command, "E") == 0) {
		cmd_switches = EXAMINE;
		process_control_panel_commands();
	} else if (strcmp(command, "EN") == 0) {
		cmd_switches = EXAMINE_NEXT;
		process_control_panel_commands();
	} else if (strcmp(command, "D") == 0) {
		cmd_switches = DEPOSIT;
		process_control_panel_commands();
	} else if (strcmp(command, "DN") == 0) {
		cmd_switches = DEPOSIT_NEXT;
		process_control_panel_commands();
	} else if (strcmp(command, "S") == 0) {
		cmd_switches = SINGLE_STEP;
		process_control_panel_commands();
	} else {
		process_virtual_switches(command, process_control_panel_commands);
	}

	queue_mqtt_message("\r\nCPU MONITOR> ", 15);
}

void publish_cpu_state(char* command, uint16_t address_bus, uint8_t data_bus) {
	char address_bus_high_byte[9];
	char address_bus_low_byte[9];
	char data_bus_binary[9];
	uint8_t instruction_length = 0;
	char panel_info[128] = { 0 };

	uint8_to_binary((uint8_t)(address_bus >> 8), address_bus_high_byte, sizeof(address_bus_high_byte));
	uint8_to_binary((uint8_t)(address_bus), address_bus_low_byte, sizeof(address_bus_low_byte));
	uint8_to_binary(data_bus, data_bus_binary, sizeof(data_bus_binary));

	size_t msg_length = snprintf(panel_info, sizeof(panel_info), "\r\n%15s: Address bus: %s %s (0x%04x), Data bus %s (0x%02x), %-15s  (%d byte instruction)",
		command, address_bus_high_byte, address_bus_low_byte, address_bus, data_bus_binary, data_bus,
		get_i8080_instruction_name(data_bus, &instruction_length), instruction_length);

	queue_mqtt_message(panel_info, msg_length);
	//publish_message(msgBuffer, strlen(msgBuffer), pub_topic_data);

	//if (consoleFd != -1) {
	//	write(consoleFd, msgBuffer, strlen(msgBuffer));
	//	delay(1);
	//}
}