/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "cpu_monitor.h"

static const char *too_many_switches =
	"\r\nError: Number of input switches must be less that or equal to 16.\n\r";
static const char *invalid_switches    = "\r\nError: Input switches must be either 0 or 1.\n\r";
static char panel_info[256]            = {0};
static ALTAIR_COMMAND deferred_command = NOP;

// static void process_control_panel_commands(void);

static bool validate_input_data(const char *command)
{
	size_t len = strlen(command);

	if (len > 16)
	{
		publish_message(too_many_switches, strlen(too_many_switches));
		return false;
	}

	for (size_t i = 0; i < len; i++)
	{
		if (!(command[i] == '1' || command[i] == '0'))
		{
			publish_message(invalid_switches, strlen(invalid_switches));
			return false;
		}
	}
	return true;
}

static void publish_virtual_input_data(void)
{
	uint8_t i8080_instruction_size = 0;
	char address_bus_high_byte[9];
	char address_bus_low_byte[9];

	uint8_to_binary((uint8_t)(bus_switches >> 8), address_bus_high_byte, sizeof(address_bus_high_byte));
	uint8_to_binary((uint8_t)(bus_switches), address_bus_low_byte, sizeof(address_bus_low_byte));

	snprintf(panel_info, sizeof(panel_info), "\r\n%15s: %s %s (0x%04x), %s (%d byte instruction)", "Input",
		address_bus_high_byte, address_bus_low_byte, bus_switches,
		get_i8080_instruction_name((uint8_t)bus_switches, &i8080_instruction_size), i8080_instruction_size);
	publish_message(panel_info, strlen(panel_info));
}

static void process_virtual_switches(const char *command)
{
	uint16_t temp_bus_switches = 0;
	uint16_t mask              = 1;

	size_t len = strlen(command);

	if (len == 0)
	{
		return;
	}

	if (validate_input_data(command))
	{
		for (size_t i = len; i > 0; i--)
		{
			temp_bus_switches |= (command[i - 1] == '1' ? mask * 1 : 0);
			mask <<= 1;
		}
		bus_switches = temp_bus_switches;
		publish_virtual_input_data();
	}
}

void process_virtual_input(const char *command)
{
	if (strlen(command) == 0)
	{
		publish_message("\r\nCPU MONITOR> ", 15);
		return;
	}

	if (strcmp(command, "E") == 0)
	{
		cmd_switches = EXAMINE;
		process_control_panel_commands();
	}
	else if (strcmp(command, "EN") == 0)
	{
		cmd_switches = EXAMINE_NEXT;
		process_control_panel_commands();
	}
	else if (strcmp(command, "D") == 0)
	{
		cmd_switches = DEPOSIT;
		process_control_panel_commands();
	}
	else if (strcmp(command, "DN") == 0)
	{
		cmd_switches = DEPOSIT_NEXT;
		process_control_panel_commands();
	}
	else if (strcmp(command, "S") == 0)
	{
		cmd_switches = SINGLE_STEP;
		process_control_panel_commands();
	}
	else if (strcmp(command, "L") == 0)
	{
		cmd_switches = DISASSEMBLE;
		process_control_panel_commands();
	}
	else if (strcmp(command, "T") == 0)
	{
		cmd_switches = TRACE;
		process_control_panel_commands();
	}
	else if (strcmp(command, "R") == 0)
	{
		cmd_switches = RESET;
		process_control_panel_commands();
	}
	else if (strcmp(command, "BASIC") == 0)
	{
		cmd_switches = LOAD_ALTAIR_BASIC;
		process_control_panel_commands();
	}
	else
	{
		process_virtual_switches(command);
		publish_message("\r\nCPU MONITOR> ", 15);
	}
}

void disassemble(intel8080_t *cpu)
{
	char address_bus_high_byte[9];
	char address_bus_low_byte[9];
	char data_bus_binary[9];
	uint8_t instruction_length = 0;

	for (size_t instruction_count = 0; instruction_count < 20; instruction_count++)
	{
		uint8_to_binary(
			(uint8_t)(cpu->address_bus >> 8), address_bus_high_byte, sizeof(address_bus_high_byte));
		uint8_to_binary((uint8_t)(cpu->address_bus), address_bus_low_byte, sizeof(address_bus_low_byte));
		uint8_to_binary(cpu->data_bus, data_bus_binary, sizeof(data_bus_binary));

		size_t msg_length = (size_t)snprintf(panel_info, sizeof(panel_info),
			"\r\n%15s: Address bus: %s%s (0x%04x) (0o%06o), Data bus %s (0x%02x) (0o%03o), %-15s  (%d byte instruction)",
			"Disassemble", address_bus_high_byte, 
			address_bus_low_byte, cpu->address_bus,
			cpu->address_bus, data_bus_binary, 
			cpu->data_bus, cpu->data_bus,
			get_i8080_instruction_name(cpu->data_bus, &instruction_length),
			instruction_length);

		publish_message(panel_info, msg_length);

		for (size_t i = 1; i < instruction_length; i++)
		{
			i8080_examine_next(cpu);

			uint8_to_binary(
				(uint8_t)(cpu->address_bus >> 8), address_bus_high_byte, sizeof(address_bus_high_byte));
			uint8_to_binary((uint8_t)(cpu->address_bus), address_bus_low_byte, sizeof(address_bus_low_byte));
			uint8_to_binary(cpu->data_bus, data_bus_binary, sizeof(data_bus_binary));

			msg_length = (size_t)snprintf(panel_info, sizeof(panel_info),
				"\r\n%15s: Address bus: %s%s (0x%04x) (0o%06o), Data bus %s (0x%02x) (0o%03o)", "Disassemble",
				address_bus_high_byte, address_bus_low_byte, 
				cpu->address_bus, cpu->address_bus,
				data_bus_binary, 
				cpu->data_bus, cpu->data_bus);

			publish_message(panel_info, msg_length);
		}
		i8080_examine_next(cpu);
	}
	i8080_examine(cpu, bus_switches);
	bus_switches = cpu->address_bus;
	publish_message("\n\rCPU MONITOR> ", 15);
}

void trace(intel8080_t *cpu)
{
	char address_bus_high_byte[9];
	char address_bus_low_byte[9];
	char data_bus_binary[9];
	uint8_t instruction_length = 0;
	// uint16_t old_address_bus;

	setvbuf(stdout, NULL, _IONBF, 0); // disable stdout buffering to ensure message written
	i8080_cycle(cpu);

	for (size_t instruction_count = 0; instruction_count < 20; instruction_count++)
	{
		uint8_to_binary(
			(uint8_t)(cpu->address_bus >> 8), address_bus_high_byte, sizeof(address_bus_high_byte));
		uint8_to_binary((uint8_t)(cpu->address_bus), address_bus_low_byte, sizeof(address_bus_low_byte));
		uint8_to_binary(cpu->data_bus, data_bus_binary, sizeof(data_bus_binary));

		size_t msg_length = (size_t)snprintf(panel_info, sizeof(panel_info),
			"\r\n%15s: Address bus: %s%s (0x%04x) (0o%06o), Data bus %s (0x%02x) (0o%03o), %-15s  (%d byte instruction)",
			"Trace", address_bus_high_byte, address_bus_low_byte, 
			cpu->address_bus, cpu->address_bus,
			data_bus_binary, 
			cpu->data_bus, cpu->data_bus,
			get_i8080_instruction_name(cpu->data_bus, &instruction_length),
			instruction_length);

		publish_message(panel_info, msg_length);

		// old_address_bus = cpu->address_bus;

		for (size_t i = 1; i < instruction_length; i++)
		{
			i8080_examine_next(cpu);

			uint8_to_binary(
				(uint8_t)(cpu->address_bus >> 8), address_bus_high_byte, sizeof(address_bus_high_byte));
			uint8_to_binary((uint8_t)(cpu->address_bus), address_bus_low_byte, sizeof(address_bus_low_byte));
			uint8_to_binary(cpu->data_bus, data_bus_binary, sizeof(data_bus_binary));

			msg_length = (size_t)snprintf(panel_info, sizeof(panel_info),
				"\r\n%15s: Address bus: %s%s (0x%04x) (0o%06o), Data bus %s (0x%02x) (0o%03o)", "Trace", address_bus_high_byte, address_bus_low_byte, 
				cpu->address_bus, cpu->address_bus,
				data_bus_binary, 
				cpu->data_bus, cpu->data_bus);

			publish_message(panel_info, msg_length);
		}
		// i8080_examine(cpu, old_address_bus);
		i8080_examine_next(cpu);
		i8080_cycle(cpu);
	}
	bus_switches = cpu->address_bus;
	publish_message("\n\rCPU MONITOR> ", 15);
}

void publish_cpu_state(char *command, uint16_t address_bus, uint8_t data_bus)
{
	char address_bus_high_byte[9];
	char address_bus_low_byte[9];
	char data_bus_binary[9];
	uint8_t instruction_length = 0;

	uint8_to_binary((uint8_t)(address_bus >> 8), address_bus_high_byte, sizeof(address_bus_high_byte));
	uint8_to_binary((uint8_t)(address_bus), address_bus_low_byte, sizeof(address_bus_low_byte));
	uint8_to_binary(data_bus, data_bus_binary, sizeof(data_bus_binary));

	size_t msg_length = (size_t)snprintf(panel_info, sizeof(panel_info),
		"\r\n%15s: Address bus: %s%s (0x%04x) (0o%06o), Data bus %s (0x%02x) (0o%03o), %-15s  (%d byte instruction)\n\rCPU "
		"MONITOR> ",
		command, address_bus_high_byte, address_bus_low_byte, 
		address_bus, address_bus,
		data_bus_binary, 
		data_bus, data_bus,
		get_i8080_instruction_name(data_bus, &instruction_length), instruction_length);

	publish_message((const char *)panel_info, msg_length);
}

#ifdef AZURE_SPHERE
bool loadRomImage(char *romImageName, uint16_t loadAddress)
{
	int romFd = -1;
	romFd     = Storage_OpenFileInImagePackage(romImageName);
	if (romFd == -1)
		return false;

	off_t length = lseek(romFd, 0, SEEK_END);
	lseek(romFd, 0, SEEK_SET);

	ssize_t bytes = read(romFd, &memory[loadAddress], (size_t)length);
	close(romFd);

	return bytes == length;
}
#else
bool loadRomImage(char *romImageName, uint16_t loadAddress)
{
	int romFd = -1;
	romFd     = open(romImageName, O_RDONLY);
	if (romFd == -1)
		return false;

	off_t length = lseek(romFd, 0, SEEK_END);
	lseek(romFd, 0, SEEK_SET);

	ssize_t bytes = read(romFd, &memory[loadAddress], (size_t)length);
	close(romFd);

	return bytes == length;
}
#endif

void load_boot_disk(void)
{
	memset(memory, 0x00, 64 * 1024); // clear altair memory.
	// load Disk Loader at 0xff00
	if (!loadRomImage(DISK_LOADER, 0xff00))
	{
		Log_Debug("Failed to open %s disk load ROM image\n", DISK_LOADER);
	}
	// print_console_banner();

	i8080_examine(&cpu, 0xff00); // 0xff00 loads from disk boot loader
}

/// <summary>
/// Process Altair front panel commands
/// </summary>
void altair_panel_command_handler(void)
{
	switch (deferred_command)
	{
		case SINGLE_STEP:
			i8080_cycle(&cpu);
			publish_cpu_state("Single step", cpu.address_bus, cpu.data_bus);
			bus_switches = cpu.address_bus;
			break;
		case EXAMINE:
			i8080_examine(&cpu, bus_switches);
			publish_cpu_state("Examine", cpu.address_bus, cpu.data_bus);
			bus_switches = cpu.address_bus;
			break;
		case EXAMINE_NEXT:
			i8080_examine_next(&cpu);
			publish_cpu_state("Examine next", cpu.address_bus, cpu.data_bus);
			bus_switches = cpu.address_bus;
			break;
		case DEPOSIT:
			i8080_deposit(&cpu, (uint8_t)(bus_switches & 0xff));
			publish_cpu_state("Deposit", cpu.address_bus, cpu.data_bus);
			break;
		case DEPOSIT_NEXT:
			i8080_deposit_next(&cpu, (uint8_t)(bus_switches & 0xff));
			publish_cpu_state("Deposit next", cpu.address_bus, cpu.data_bus);
			bus_switches = cpu.address_bus;
			break;
		case DISASSEMBLE:
			i8080_examine(&cpu, bus_switches);
			disassemble(&cpu);
			break;
		case TRACE:
			i8080_examine(&cpu, bus_switches);
			trace(&cpu);
			break;
		case RESET:
			load_boot_disk();
			cpu_operating_mode = CPU_RUNNING;
			break;
		case LOAD_ALTAIR_BASIC:
			memset(memory, 0x00, 64 * 1024); // clear altair memory.
			// load Altair BASIC at 0xff00
			if (!loadRomImage(ALTAIR_BASIC_ROM, 0x0000))
			{
				Log_Debug("Failed to open %s disk load ROM image\n", ALTAIR_BASIC_ROM);
			}
			print_console_banner();

			i8080_examine(&cpu, 0x0000); // 0x0000 loads Altair BASIC
			cpu_operating_mode = CPU_RUNNING;
			break;
		default:
			break;
	}
}

void process_control_panel_commands(void)
{
	if (cpu_operating_mode == CPU_STOPPED || cmd_switches == STOP_CMD)
	{
		switch (cmd_switches)
		{
			case RUN_CMD:
				cpu_operating_mode = CPU_RUNNING;
				break;
			case STOP_CMD:
				cpu_operating_mode = CPU_STOPPED;
				i8080_examine(&cpu, cpu.registers.pc);
				bus_switches = cpu.address_bus;
				break;
			default:
				deferred_command = cmd_switches;
				altair_panel_command_handler();
				break;
		}
	}

	// if (cmd_switches & STOP_CMD)
	// {
	// 	cpu_operating_mode = CPU_STOPPED;
	// }
	cmd_switches = 0x00;
}