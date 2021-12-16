/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "dx_config.h"

extern volatile sig_atomic_t terminationRequired;

// Usage text for command line arguments in application manifest.
static const char *cmdLineArgsUsageText =
    "DPS connection type: \" CmdArgs \": [ \"--ScopeID\", \"<scope_id>\"]\n"
    "Hostname connection type: \" CmdArgs \": [\"--Hostname\", \"<azureiothub_hostname>\"]\n";

/// <summary>
///     Parse the command line arguments given in the application manifest.
/// </summary>
bool dx_configParseCmdLineArguments(int argc, char *argv[], DX_USER_CONFIG *userConfig)
{
    bool result = true;
    int option = 0;
    static const struct option cmdLineOptions[] = {
        {.name = "ScopeID", .has_arg = required_argument, .flag = NULL, .val = 's'},
        {.name = "Hostname", .has_arg = required_argument, .flag = NULL, .val = 'h'}};

    userConfig->connectionType = DX_CONNECTION_TYPE_NOT_DEFINED;

    // Loop over all of the options.
    while ((option = getopt_long(argc, argv, "s:h:", cmdLineOptions, NULL)) != -1) {
        // Check if arguments are missing. Every option requires an argument.
        if (optarg != NULL && optarg[0] == '-') {
            Log_Debug("WARNING: Option %c requires an argument\n", option);
            continue;
        }
        switch (option) {
        case 's':
            Log_Debug("ScopeId: %s\n", optarg);
            userConfig->idScope = optarg;
            userConfig->connectionType = DX_CONNECTION_TYPE_DPS;
            break;
        case 'h':
            Log_Debug("Hostname String: %s\n", optarg);
            userConfig->hostname = optarg;
            userConfig->connectionType = DX_CONNECTION_TYPE_HOSTNAME;
            break;
        default:
            // Unknown options are ignored.
            break;
        }
    }

    switch (userConfig->connectionType) {
    case DX_CONNECTION_TYPE_NOT_DEFINED:
        dx_terminate(DX_ExitCode_Validate_Connection_Type_Not_Defined);
        result = false;
        break;
    case DX_CONNECTION_TYPE_DPS:
        if (dx_isStringNullOrEmpty(userConfig->idScope)) {
            dx_terminate(DX_ExitCode_Validate_ScopeId_Not_Defined);
            result = false;
        }
        break;
    case DX_CONNECTION_TYPE_HOSTNAME:
        if (dx_isStringNullOrEmpty(userConfig->hostname)) {
            dx_terminate(DX_ExitCode_Validate_Hostname_Not_Defined);
            result = false;
        }
        break;
    }

    if (!result) {
        Log_Debug(cmdLineArgsUsageText);
    }

    return result;
}
