#pragma once

#include "dx_terminate.h"
#include "dx_timer.h"
#include "dx_utilities.h"
#include <applibs/log.h>
#include <applibs/sysevent.h>
#include <errno.h>
#include <stdbool.h>


/// <summary>
/// register callbacks for deferred updates
/// </summary>
/// <param name="deferredUpdateCalculateCallback"></param>
/// <param name="deferredUpdateNotificationCallback"></param>
void dx_deferredUpdateRegistration(uint32_t (*deferredUpdateCalculateCallback)(uint32_t max_deferral_time_in_minutes,
                                                                               SysEvent_UpdateType type, SysEvent_Status status,
                                                                               const char *typeDescription, const char *statusDescription),
                                   void (*deferredUpdateNotificationCallback)(uint32_t max_deferral_time_in_minutes,
                                                                              SysEvent_UpdateType type, SysEvent_Status status,
                                                                              const char *typeDescription, const char *statusDescription));