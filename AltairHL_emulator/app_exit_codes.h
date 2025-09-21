/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

/// <summary>
/// Exit codes for this application. These are used for the
/// application exit code.  They must all be between zero and 255,
/// where zero is reserved for successful termination.
/// </summary>
typedef enum
{
	APP_EXIT_SEEED_SPI_OPEN     = 100,
	APP_EXIT_SEEED_SPI_TRANSFER = 101,
	APP_EXIT_PANEL8X8_SPI_OPEN  = 102,
	APP_EXIT_KEY4X4_SPI_OPEN    = 103,
	APP_EXIT_RETRO_CLICK_OPEN   = 104,
	APP_EXIT_DISK_A_INIT_FAILED = 105,
	APP_EXIT_DISK_B_INIT_FAILED = 106,
	APP_EXIT_DISK_C_INIT_FAILED = 107,
	APP_EXIT_DISK_D_INIT_FAILED = 108,
	APP_EXIT_ROM_LOAD_FAILED    = 109,
	APP_EXIT_INVALID_CONFIG     = 110
} APP_EXIT_CODE;