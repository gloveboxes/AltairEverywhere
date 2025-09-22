/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "altair_config.h"
#include <stdio.h>

// Usage text for command line arguments in application manifest.
static const char *cmdLineArgsUsageText =
	"Altair 8800 Emulator - Command Line Options:\n"
	"\n"
	"MQTT Configuration (optional):\n"
	"  -m, --MqttHost <host>          MQTT broker hostname (required for MQTT)\n"
	"  -p, --MqttPort <port>          MQTT broker port (default: 1883)\n"
	"  -c, --MqttClientId <client_id> MQTT client ID (default: AltairEmulator)\n"
	"\n"
	"Network Configuration:\n"
	"  -n, --NetworkInterface <iface> Network interface to use\n"
	"\n"
	"External Services:\n"
	"  -o, --OpenWeatherMapKey <key>  OpenWeatherMap API key\n"
	"  -u, --CopyXUrl <url>           CopyX service URL\n"
	"  -a, --OpenAIKey <key>          OpenAI API key\n"
	"\n"
	"Help:\n"
	"  -h, --help                     Show this help message\n"
	"\n"
	"Example:\n"
	"  ./Altair_emulator -m mqtt.example.com -p 1883 -c MyAltair\n";

bool parse_altair_cmd_line_arguments(int argc, char *argv[], ALTAIR_CONFIG_T *altair_config)
{
	bool result                                 = true;
	int option                                  = 0;
	static const struct option cmdLineOptions[] = {
		{.name = "MqttHost", .has_arg = required_argument, .flag = NULL, .val = 'm'},
		{.name = "MqttPort", .has_arg = required_argument, .flag = NULL, .val = 'p'},
		{.name = "MqttClientId", .has_arg = required_argument, .flag = NULL, .val = 'c'},
		{.name = "NetworkInterface", .has_arg = required_argument, .flag = NULL, .val = 'n'},
		{.name = "OpenWeatherMapKey", .has_arg = required_argument, .flag = NULL, .val = 'o'},
		{.name = "CopyXUrl", .has_arg = required_argument, .flag = NULL, .val = 'u'},
		{.name = "OpenAIKey", .has_arg = required_argument, .flag = NULL, .val = 'a'},
		{.name = "help", .has_arg = no_argument, .flag = NULL, .val = 'h'}};

	altair_config->user_config.connectionType = DX_CONNECTION_TYPE_MQTT;
	
	// Set default values - no default MQTT host, only connect when explicitly specified
	altair_config->user_config.mqtt_host = NULL;
	altair_config->user_config.mqtt_port = "1883";
	altair_config->user_config.mqtt_client_id = "AltairEmulator";

	// Loop over all of the options.
	while ((option = getopt_long(argc, argv, "m:p:c:n:o:u:a:h", cmdLineOptions, NULL)) != -1)
	{
		// Check if arguments are missing. Every option requires an argument.
		if (optarg != NULL && optarg[0] == '-')
		{
			printf("WARNING: Option %c requires an argument\n", option);
			continue;
		}
		switch (option)
		{
			case 'm':
				altair_config->user_config.mqtt_host = optarg;
				break;
			case 'p':
				altair_config->user_config.mqtt_port = optarg;
				break;
			case 'c':
				altair_config->user_config.mqtt_client_id = optarg;
				break;
			case 'n':
				altair_config->user_config.network_interface = optarg;
				break;
			case 'o':
				altair_config->open_weather_map_api_key = optarg;
				break;
			case 'u':
				altair_config->copy_x_url = optarg;
				break;
			case 'a':
				altair_config->openai_api_key = optarg;
				break;
			case 'h':
				printf("%s\n", cmdLineArgsUsageText);
				return false;
				break;
			default:
				// Unknown options are ignored.
				break;
		}
	}

	switch (altair_config->user_config.connectionType)
	{
		case DX_CONNECTION_TYPE_NOT_DEFINED:
			result = false;
			break;
		case DX_CONNECTION_TYPE_MQTT:
			// MQTT connection is valid - defaults are set if not specified
			break;
	}

	if (!result)
	{
		printf("%s\n", cmdLineArgsUsageText);
	}

	return result;
}