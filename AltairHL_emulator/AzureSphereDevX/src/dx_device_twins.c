/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "dx_device_twins.h"

static bool deviceTwinReportState(DX_DEVICE_TWIN_BINDING *deviceTwinBinding, void *state,
                                  bool deviceTwinPnPAcknowledgment,
                                  DX_DEVICE_TWIN_RESPONSE_CODE statusCode);
static bool deviceTwinUpdateReportedState(char *reportedPropertiesString);
static void deviceTwinClose(DX_DEVICE_TWIN_BINDING *deviceTwinBinding);
static void deviceTwinOpen(DX_DEVICE_TWIN_BINDING *deviceTwinBinding);
static void deviceTwinsReportStatusCallback(int result, void *context);
static void SetDesiredState(JSON_Object *desiredProperties,
                            DX_DEVICE_TWIN_BINDING *deviceTwinBinding);
static void DeviceTwinCallbackHandler(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payload, size_t payloadSize,
                                      void *userContextCallback);

static DX_DEVICE_TWIN_BINDING **_deviceTwins = NULL;
static size_t _deviceTwinCount = 0;

void dx_deviceTwinSubscribe(DX_DEVICE_TWIN_BINDING *deviceTwins[], size_t deviceTwinCount)
{
    dx_azureRegisterDeviceTwinCallback(DeviceTwinCallbackHandler);

    _deviceTwins = deviceTwins;
    _deviceTwinCount = deviceTwinCount;

    for (int i = 0; i < _deviceTwinCount; i++) {
        deviceTwinOpen(_deviceTwins[i]);
    }
}

void dx_deviceTwinUnsubscribe(void)
{
    dx_azureRegisterDeviceTwinCallback(NULL);

    for (int i = 0; i < _deviceTwinCount; i++) {
        deviceTwinClose(_deviceTwins[i]);
    }
}

static void deviceTwinOpen(DX_DEVICE_TWIN_BINDING *deviceTwinBinding)
{
    if (deviceTwinBinding->twinType == DX_TYPE_UNKNOWN) {
        Log_Debug(
            "\n\nDevice Twin '%s' missing type information.\nInclude .twinType option in "
            "DX_DEVICE_TWIN_BINDING definition.\nExample .twinType=DX_DEVICE_TWIN_BOOL. Valid types "
            "include DX_DEVICE_TWIN_BOOL, DX_DEVICE_TWIN_INT, DX_DEVICE_TWIN_FLOAT, DX_DEVICE_TWIN_STRING.\n\n",
            deviceTwinBinding->propertyName);
        dx_terminate(DX_ExitCode_OpenDeviceTwin);
    }

    // types JSON and String allocated dynamically when called in azure_iot.c
    switch (deviceTwinBinding->twinType) {
    case DX_DEVICE_TWIN_INT:
        deviceTwinBinding->propertyValue = malloc(sizeof(int));
        memset(deviceTwinBinding->propertyValue, 0x00, sizeof(int));
        *(int *)deviceTwinBinding->propertyValue = 0;
        break;
    case DX_DEVICE_TWIN_FLOAT:
        deviceTwinBinding->propertyValue = malloc(sizeof(float));
        memset(deviceTwinBinding->propertyValue, 0x00, sizeof(float));
        *(float *)deviceTwinBinding->propertyValue = 0.0f;
        break;
    case DX_DEVICE_TWIN_DOUBLE:
        deviceTwinBinding->propertyValue = malloc(sizeof(double));
        memset(deviceTwinBinding->propertyValue, 0x00, sizeof(double));
        *(double *)deviceTwinBinding->propertyValue = 0.0;
        break;
    case DX_DEVICE_TWIN_BOOL:
        deviceTwinBinding->propertyValue = malloc(sizeof(bool));
        memset(deviceTwinBinding->propertyValue, 0x00, sizeof(bool));
        *(bool *)deviceTwinBinding->propertyValue = false;
        break;
    case DX_DEVICE_TWIN_STRING:
        // Note no memory is allocated for string twin type as size is unknown
        break;
    default:
        break;
    }
}

static void deviceTwinClose(DX_DEVICE_TWIN_BINDING *deviceTwinBinding)
{
    if (deviceTwinBinding->propertyValue != NULL) {
        free(deviceTwinBinding->propertyValue);
        deviceTwinBinding->propertyValue = NULL;
    }
}

/// <summary>
///     Callback invoked when a Device Twin update is received from IoT Hub.
/// </summary>
/// <param name="payload">contains the Device Twin JSON document (desired and reported)</param>
/// <param name="payloadSize">size of the Device Twin JSON document</param>
static void DeviceTwinCallbackHandler(DEVICE_TWIN_UPDATE_STATE updateState,
                                   const unsigned char *payload, size_t payloadSize,
                                   void *userContextCallback)
{
    JSON_Value *root_value = NULL;
    JSON_Object *root_object = NULL;

    char *payLoadString = (char *)malloc(payloadSize + 1);
    if (payLoadString == NULL) {
        goto cleanup;
    }

    memset(payLoadString, 0x00, payloadSize + 1);

    memcpy(payLoadString, payload, payloadSize);
    payLoadString[payloadSize] = 0; // null terminate string

    root_value = json_parse_string(payLoadString);
    if (root_value == NULL) {
        goto cleanup;
    }

    root_object = json_value_get_object(root_value);
    if (root_object == NULL) {
        goto cleanup;
    }

    JSON_Object *desiredProperties = json_object_dotget_object(root_object, "desired");
    if (desiredProperties == NULL) {
        desiredProperties = root_object;
    }

    for (int i = 0; i < _deviceTwinCount; i++) {
        JSON_Value *jsonValue =
            json_object_get_value(desiredProperties, _deviceTwins[i]->propertyName);
        if (jsonValue != NULL) {
            SetDesiredState(desiredProperties, _deviceTwins[i]);
        }
    }

cleanup:
    // Release the allocated memory.
    if (root_value != NULL) {
        json_value_free(root_value);
    }

    if (payLoadString != NULL) {
        free(payLoadString);
        payLoadString = NULL;
    }
}

/// <summary>
///     Checks to see if the device twin propertyName(name) is found in the json object. If yes,
///     then act upon the request
/// </summary>
static void SetDesiredState(JSON_Object *jsonObject, DX_DEVICE_TWIN_BINDING *deviceTwinBinding)
{
    if (json_object_has_value_of_type(jsonObject, "$version", JSONNumber)) {
        deviceTwinBinding->propertyVersion = (int)json_object_get_number(jsonObject, "$version");
    }

    switch (deviceTwinBinding->twinType) {
    case DX_DEVICE_TWIN_INT:
        if (json_object_has_value_of_type(jsonObject, deviceTwinBinding->propertyName,
                                          JSONNumber)) {
            *(int *)deviceTwinBinding->propertyValue =
                (int)json_object_get_number(jsonObject, deviceTwinBinding->propertyName);

            deviceTwinBinding->propertyUpdated = true;

            if (deviceTwinBinding->handler != NULL) {
                deviceTwinBinding->handler(deviceTwinBinding);
            }
        }
        break;
    case DX_DEVICE_TWIN_FLOAT:
        if (json_object_has_value_of_type(jsonObject, deviceTwinBinding->propertyName,
                                          JSONNumber)) {
            *(float *)deviceTwinBinding->propertyValue =
                (float)json_object_get_number(jsonObject, deviceTwinBinding->propertyName);

            deviceTwinBinding->propertyUpdated = true;

            if (deviceTwinBinding->handler != NULL) {
                deviceTwinBinding->handler(deviceTwinBinding);
            }
        }
        break;
    case DX_DEVICE_TWIN_DOUBLE:
        if (json_object_has_value_of_type(jsonObject, deviceTwinBinding->propertyName,
                                          JSONNumber)) {
            *(double *)deviceTwinBinding->propertyValue =
                (double)json_object_get_number(jsonObject, deviceTwinBinding->propertyName);

            deviceTwinBinding->propertyUpdated = true;

            if (deviceTwinBinding->handler != NULL) {
                deviceTwinBinding->handler(deviceTwinBinding);
            }
        }
        break;
    case DX_DEVICE_TWIN_BOOL:
        if (json_object_has_value_of_type(jsonObject, deviceTwinBinding->propertyName,
                                          JSONBoolean)) {
            *(bool *)deviceTwinBinding->propertyValue =
                (bool)json_object_get_boolean(jsonObject, deviceTwinBinding->propertyName);

            deviceTwinBinding->propertyUpdated = true;

            if (deviceTwinBinding->handler != NULL) {
                deviceTwinBinding->handler(deviceTwinBinding);
            }
        }
        break;
    case DX_DEVICE_TWIN_STRING:
        if (json_object_has_value_of_type(jsonObject, deviceTwinBinding->propertyName,
                                          JSONString)) {
            deviceTwinBinding->propertyValue =
                (char *)json_object_get_string(jsonObject, deviceTwinBinding->propertyName);

            if (deviceTwinBinding->handler != NULL) {
                deviceTwinBinding->handler(deviceTwinBinding);
            }
            deviceTwinBinding->propertyValue = NULL;
        }
        break;
    default:
        break;
    }
}

/// <summary>
///     Sends device twin desire state IoT Plug and Play acknowledgement
/// </summary>
bool dx_deviceTwinAckDesiredValue(DX_DEVICE_TWIN_BINDING *deviceTwinBinding, void *state,
                                  DX_DEVICE_TWIN_RESPONSE_CODE statusCode)
{
    return deviceTwinReportState(deviceTwinBinding, state, true, statusCode);
}

/// <summary>
///     device twin report state to Azure IoT Hub
/// </summary>
bool dx_deviceTwinReportValue(DX_DEVICE_TWIN_BINDING *deviceTwinBinding, void *state)
{
    return deviceTwinReportState(deviceTwinBinding, state, false, DX_DEVICE_TWIN_RESPONSE_COMPLETED);
}

/// <summary>
///   Supports device twin report state and device twin ack desired state request
/// </summary>
static bool deviceTwinReportState(DX_DEVICE_TWIN_BINDING *deviceTwinBinding, void *state,
                                  bool deviceTwinPnPAcknowledgment,
                                  DX_DEVICE_TWIN_RESPONSE_CODE statusCode)
{
    int len = 0;
    size_t reportLen = 10; // initialize to 10 chars to allow for JSON and NULL termination. This is
                           // generous by a couple of bytes
    bool result = false;

    if (deviceTwinBinding == NULL) {
        return false;
    }

    if (!dx_isAzureConnected()) {
        return false;
    }

    reportLen +=
        strlen(deviceTwinBinding->propertyName); // allow for twin property name in JSON response

    if (deviceTwinBinding->twinType == DX_DEVICE_TWIN_STRING) {
        reportLen += strlen((char *)state);
    } else {
        reportLen += 40; // allow 40 chars for Int, float, double, and boolean serialization
    }

    // to allow for device twin acknowledgement data
    if (deviceTwinPnPAcknowledgment) {
        reportLen += 40;
    }

    char *reportedPropertiesString = (char *)malloc(reportLen);
    if (reportedPropertiesString == NULL) {
        return false;
    }

    memset(reportedPropertiesString, 0, reportLen);

    switch (deviceTwinBinding->twinType) {
    case DX_DEVICE_TWIN_INT:
        *(int *)deviceTwinBinding->propertyValue = *(int *)state;

        if (deviceTwinPnPAcknowledgment) {
            len = snprintf(reportedPropertiesString, reportLen,
                           "{\"%s\":{\"value\":%d, \"ac\":%d, \"av\":%d}}",
                           deviceTwinBinding->propertyName, (*(int *)deviceTwinBinding->propertyValue),
                           (int)statusCode, deviceTwinBinding->propertyVersion);
        } else {
            len = snprintf(reportedPropertiesString, reportLen, "{\"%s\":%d}",
                           deviceTwinBinding->propertyName, (*(int *)deviceTwinBinding->propertyValue));
        }
        break;
    case DX_DEVICE_TWIN_FLOAT:
        *(float *)deviceTwinBinding->propertyValue = *(float *)state;

        if (deviceTwinPnPAcknowledgment) {
            len =
                snprintf(reportedPropertiesString, reportLen,
                         "{\"%s\":{\"value\":%f, \"ac\":%d, \"av\":%d}}",
                         deviceTwinBinding->propertyName, (*(float *)deviceTwinBinding->propertyValue),
                         (int)statusCode, deviceTwinBinding->propertyVersion);
        } else {
            len =
                snprintf(reportedPropertiesString, reportLen, "{\"%s\":%f}",
                         deviceTwinBinding->propertyName, (*(float *)deviceTwinBinding->propertyValue));
        }
        break;
    case DX_DEVICE_TWIN_DOUBLE:
        *(double *)deviceTwinBinding->propertyValue = *(double *)state;

        if (deviceTwinPnPAcknowledgment) {
            len =
                snprintf(reportedPropertiesString, reportLen,
                         "{\"%s\":{\"value\":%lf, \"ac\":%d, \"av\":%d}}",
                         deviceTwinBinding->propertyName, (*(double *)deviceTwinBinding->propertyValue),
                         (int)statusCode, deviceTwinBinding->propertyVersion);
        } else {
            len = snprintf(reportedPropertiesString, reportLen, "{\"%s\":%lf}",
                           deviceTwinBinding->propertyName,
                           (*(double *)deviceTwinBinding->propertyValue));
        }
        break;
    case DX_DEVICE_TWIN_BOOL:
        *(bool *)deviceTwinBinding->propertyValue = *(bool *)state;

        if (deviceTwinPnPAcknowledgment) {
            len = snprintf(reportedPropertiesString, reportLen,
                           "{\"%s\":{\"value\":%s, \"ac\":%d, \"av\":%d}}",
                           deviceTwinBinding->propertyName,
                           (*(bool *)deviceTwinBinding->propertyValue ? "true" : "false"),
                           (int)statusCode, deviceTwinBinding->propertyVersion);
        } else {
            len = snprintf(reportedPropertiesString, reportLen, "{\"%s\":%s}",
                           deviceTwinBinding->propertyName,
                           (*(bool *)deviceTwinBinding->propertyValue ? "true" : "false"));
        }
        break;
    case DX_DEVICE_TWIN_STRING:
        deviceTwinBinding->propertyValue = NULL;

        if (deviceTwinPnPAcknowledgment) {
            len = snprintf(reportedPropertiesString, reportLen,
                           "{\"%s\":{\"value\":\"%s\", \"ac\":%d, \"av\":%d}}",
                           deviceTwinBinding->propertyName, (char *)state, (int)statusCode,
                           deviceTwinBinding->propertyVersion);
        } else {
            len = snprintf(reportedPropertiesString, reportLen, "{\"%s\":\"%s\"}",
                           deviceTwinBinding->propertyName, (char *)state);
        }

        break;
    case DX_TYPE_UNKNOWN:
        Log_Debug("Device Twin Type Unknown");
        break;
    default:
        break;
    }

    if (len > 0) {
        result = deviceTwinUpdateReportedState(reportedPropertiesString);
    }

    if (reportedPropertiesString != NULL) {
        free(reportedPropertiesString);
        reportedPropertiesString = NULL;
    }

    return result;
}

static bool deviceTwinUpdateReportedState(char *reportedPropertiesString)
{
    if (IoTHubDeviceClient_LL_SendReportedState(
            dx_azureClientHandleGet(), (unsigned char *)reportedPropertiesString,
            strlen(reportedPropertiesString), deviceTwinsReportStatusCallback,
            0) != IOTHUB_CLIENT_OK) {
#if DX_LOGGING_ENABLED
        Log_Debug("ERROR: failed to set reported state for '%s'.\n", reportedPropertiesString);
#endif

        return false;
    } else {
#if DX_LOGGING_ENABLED
        Log_Debug("INFO: Reported state propertyUpdated '%s'.\n", reportedPropertiesString);
#endif

        return true;
    }

    // IoTHubDeviceClient_LL_DoWork(dx_azureClientHandleGet());
}

/// <summary>
///     Callback invoked when the Device Twin reported properties are accepted by IoT Hub.
/// </summary>
void deviceTwinsReportStatusCallback(int result, void *context)
{
#if DX_LOGGING_ENABLED
    Log_Debug("INFO: Device Twin reported properties update result: HTTP status code %d\n", result);
#endif
}