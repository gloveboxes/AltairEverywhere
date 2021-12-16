/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "dx_exit_codes.h"
#include "dx_timer.h"
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

// volatile sig_atomic_t terminationRequired = false;

bool dx_isTerminationRequired(void);
int dx_getTerminationExitCode(void);
void dx_eventLoopRun(void);
void dx_registerTerminationHandler(void);
void dx_terminate(int exitCode);
void dx_terminationHandler(int signalNumber);