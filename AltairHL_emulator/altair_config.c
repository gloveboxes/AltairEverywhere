/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "altair_config.h"
#include <stdio.h>

// Usage text for command line arguments in application manifest.
static const char *cmdLineArgsUsageText =
	"DPS connection type: \"CmdArgs:\" -s \"<your_scope_id>\" -d \"<your_device_id>\" -k "
	"\"<your_device_key>\"\n"
	"Connection string type: \"CmdArgs:\" -c \"<iot_hub_central_connection_string>\"\n";

bool parse_altair_cmd_line_arguments(int argc, char *argv[], ALTAIR_CONFIG_T *altair_config)
{
	bool result                                 = true;
	int option                                  = 0;
	static const struct option cmdLineOptions[] = {
		{.name = "ScopeId", .has_arg = required_argument, .flag = NULL, .val = 's'},
		{.name = "DeviceId", .has_arg = required_argument, .flag = NULL, .val = 'd'},
		{.name = "DeviceKey", .has_arg = required_argument, .flag = NULL, .val = 'k'},
		{.name = "ConnectionString", .has_arg = required_argument, .flag = NULL, .val = 'c'},
		{.name = "Hostname", .has_arg = required_argument, .flag = NULL, .val = 'h'},
		{.name = "NetworkInterface", .has_arg = required_argument, .flag = NULL, .val = 'n'},
		{.name = "OpenWeatherMapKey", .has_arg = required_argument, .flag = NULL, .val = 'o'},
		{.name = "CopyXUrl", .has_arg = required_argument, .flag = NULL, .val = 'u'},
		{.name = "OpenAIKey", .has_arg = required_argument, .flag = NULL, .val = 'a'}};

	altair_config->user_config.connectionType = DX_CONNECTION_TYPE_NOT_DEFINED;

	// Loop over all of the options.
	while ((option = getopt_long(argc, argv, "s:c:k:d:n:o:u:a:", cmdLineOptions, NULL)) != -1)
	{
		// Check if arguments are missing. Every option requires an argument.
		if (optarg != NULL && optarg[0] == '-')
		{
			printf("WARNING: Option %c requires an argument\n", option);
			continue;
		}
		switch (option)
		{
			case 'c':
				altair_config->user_config.connection_string = optarg;
				altair_config->user_config.connectionType    = DX_CONNECTION_TYPE_STRING;
				break;
			case 's':
				altair_config->user_config.idScope        = optarg;
				altair_config->user_config.connectionType = DX_CONNECTION_TYPE_DPS;
				break;
			case 'k':
				altair_config->user_config.device_key = optarg;
				break;
			case 'd':
				altair_config->user_config.device_id = optarg;
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
		case DX_CONNECTION_TYPE_DPS:
			if (dx_isStringNullOrEmpty(altair_config->user_config.idScope))
			{
				result = false;
			}
			break;
		case DX_CONNECTION_TYPE_STRING:
			if (dx_isStringNullOrEmpty(altair_config->user_config.connection_string))
			{
				result = false;
			}
			break;
		case DX_CONNECTION_TYPE_HOSTNAME:
			break;
	}

	if (!result)
	{
		printf("%s\n", cmdLineArgsUsageText);
	}

	return result;
}