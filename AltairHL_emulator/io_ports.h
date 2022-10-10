/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

// #include "dx_timer.h"
// #include "dx_utilities.h"
// #include "environment_types.h"
// #include "iotc_manager.h"
// #include <fcntl.h>
// #include <pthread.h>
// #include <stdbool.h>
// #include <stdint.h>
// #include <stdio.h>
// #include <time.h>
// #include <unistd.h>

#include "PortDrivers/file_io.h"
#include "PortDrivers/led_matrix_io.h"

uint8_t io_port_in(uint8_t port);
void io_port_out(uint8_t port, uint8_t data);
