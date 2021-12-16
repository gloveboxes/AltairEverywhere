/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "dx_terminate.h"

static volatile sig_atomic_t terminationRequired = false;
static volatile sig_atomic_t _exitCode = 0;

void dx_registerTerminationHandler(void)
{
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = dx_terminationHandler;
    sigaction(SIGTERM, &action, NULL);
}

void dx_terminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    terminationRequired = true;
    _exitCode = DX_ExitCode_TermHandler_SigTerm;
}

void dx_terminate(int exitCode)
{
    _exitCode = exitCode;
    terminationRequired = true;
}

bool dx_isTerminationRequired(void)
{
    return terminationRequired;
}

int dx_getTerminationExitCode(void)
{
    return _exitCode;
}

void dx_eventLoopRun(void)
{
    EventLoop *el = dx_timerGetEventLoop();
    // run the main event loop
    while (!terminationRequired) {
        int result = EventLoop_Run(el, -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == -1 && errno != EINTR) {
            dx_terminate(DX_ExitCode_Main_EventLoopFail);
        }
    }
}