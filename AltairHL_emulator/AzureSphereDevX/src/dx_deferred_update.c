#include "dx_deferred_update.h"

static EventRegistration *updateEventReg = NULL;
static void UpdateCallback(SysEvent_Events event, SysEvent_Status status, const SysEvent_Info *info, void *context);

uint32_t (*_deferred_update_calculate_callback)(uint32_t max_deferral_time_in_minutes, SysEvent_UpdateType type, SysEvent_Status status,
                                                const char *typeDescription, const char *statusDescription) = NULL;
void (*_deferred_update_notification_callback)(uint32_t max_deferral_time_in_minutes, SysEvent_UpdateType type, SysEvent_Status status,
                                               const char *typeDescription, const char *statusDescription) = NULL;

/// <summary>
/// Register for update events
/// </summary>
/// <param name="deferredUpdateCalculateCallback"></param>
/// <param name="deferredUpdateNotificationCallback"></param>
void dx_deferredUpdateRegistration(uint32_t (*deferredUpdateCalculateCallback)(uint32_t max_deferral_time_in_minutes,
                                                                               SysEvent_UpdateType type, SysEvent_Status status,
                                                                               const char *typeDescription, const char *statusDescription),
                                   void (*deferredUpdateNotificationCallback)(uint32_t max_deferral_time_in_minutes,
                                                                              SysEvent_UpdateType type, SysEvent_Status status,
                                                                              const char *typeDescription, const char *statusDescription))
{
    _deferred_update_calculate_callback = deferredUpdateCalculateCallback;
    _deferred_update_notification_callback = deferredUpdateNotificationCallback;

    updateEventReg =
        SysEvent_RegisterForEventNotifications(dx_timerGetEventLoop(), SysEvent_Events_UpdateReadyForInstall, UpdateCallback, NULL);
    if (updateEventReg == NULL) {
        dx_terminate(DX_ExitCode_SetUpSysEvent_RegisterEvent);
    }
}

/// <summary>
///     Convert the supplied system event status to a human-readable string.
/// </summary>
/// <param name="status">The status.</param>
/// <returns>String representation of the supplied status.</param>
static const char *EventStatusToString(SysEvent_Status status)
{
    switch (status) {
    case SysEvent_Status_Invalid:
        return "Invalid";
    case SysEvent_Status_Pending:
        return "Pending";
    case SysEvent_Status_Final:
        return "Final";
    case SysEvent_Status_Deferred:
        return "Deferred";
    case SysEvent_Status_Complete:
        return "Completed";
    default:
        return "Unknown";
    }
}

/// <summary>
///     Convert the supplied update type to a human-readable string.
/// </summary>
/// <param name="updateType">The update type.</param>
/// <returns>String representation of the supplied update type.</param>
static const char *UpdateTypeToString(SysEvent_UpdateType updateType)
{
    switch (updateType) {
    case SysEvent_UpdateType_Invalid:
        return "Invalid";
    case SysEvent_UpdateType_App:
        return "Application";
    case SysEvent_UpdateType_System:
        return "System";
    default:
        return "Unknown";
    }
}

/// <summary>
///     This function matches the SysEvent_EventsCallback signature, and is invoked
///     from the event loop when the system wants to perform an application or system update.
///     See <see cref="SysEvent_EventsCallback" /> for information about arguments.
/// </summary>
static void UpdateCallback(SysEvent_Events event, SysEvent_Status status, const SysEvent_Info *info, void *context)
{
    SysEvent_Info_UpdateData data;
    int result = SysEvent_Info_GetUpdateData(info, &data);
    uint32_t requested_minutes = 0;


    if (event != SysEvent_Events_UpdateReadyForInstall) {
        dx_terminate(DX_ExitCode_UpdateCallback_UnexpectedEvent);
        return;
    }

    if (_deferred_update_notification_callback != NULL) {

        _deferred_update_notification_callback(data.max_deferral_time_in_minutes, data.update_type, status,
                                               UpdateTypeToString(data.update_type), EventStatusToString(status));
    }

    if (result == -1) {
        dx_terminate(DX_ExitCode_UpdateCallback_GetUpdateEvent);
        return;
    }

    switch (status) {
    case SysEvent_Status_Pending:
        // If pending update the calculate if update should be deferred
        if (_deferred_update_calculate_callback != NULL) {

            requested_minutes = _deferred_update_calculate_callback(data.max_deferral_time_in_minutes, data.update_type, status,
                                                                    UpdateTypeToString(data.update_type), EventStatusToString(status));
        }

        if (requested_minutes > 0) {
            // defer for requested_minutes
            if (SysEvent_DeferEvent(SysEvent_Events_UpdateReadyForInstall, requested_minutes) == -1) {
                dx_terminate(DX_ExitCode_UpdateCallback_DeferEvent);
            }
        }

        break;

    case SysEvent_Status_Final:
        // Terminate app before it is forcibly shut down and replaced.
        // The application may be restarted before the update is applied.
        dx_terminate(DX_ExitCode_UpdateCallback_FinalUpdate);
        break;

    case SysEvent_Status_Deferred:
        // Log_Debug("INFO: Update deferred.\n");
        // Update has been deferred
        break;

    case SysEvent_Status_Complete:
        break;

    default:
        dx_terminate(DX_ExitCode_UpdateCallback_UnexpectedStatus);
        break;
    }
}
