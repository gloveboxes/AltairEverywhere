/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "dx_direct_methods.h"

static int DirectMethodCallbackHandler(const char *method_name, const unsigned char *payload, size_t payloadSize,
                                       unsigned char **responsePayload, size_t *responsePayloadSize, void *userContextCallback);

static DX_DIRECT_METHOD_BINDING **_directMethods;
static size_t _directMethodCount;

void dx_directMethodSubscribe(DX_DIRECT_METHOD_BINDING *directMethods[], size_t directMethodCount)
{
    dx_azureRegisterDirectMethodCallback(DirectMethodCallbackHandler);

    _directMethods = directMethods;
    _directMethodCount = directMethodCount;
}

void dx_directMethodUnsubscribe(void)
{
    dx_azureRegisterDirectMethodCallback(NULL);

    _directMethods = NULL;
    _directMethodCount = 0;
}

/*
This implementation of Direct Methods expects a JSON Payload Object
*/
static int DirectMethodCallbackHandler(const char *method_name, const unsigned char *payload,
                                          size_t payloadSize, unsigned char **responsePayload,
                                          size_t *responsePayloadSize, void *userContextCallback)
{
    const char *methodSucceededMsg = "Method Succeeded";
    const char *methodNotFoundMsg = "Method not found";
    const char *methodErrorMsg = "Method Error";
    const char *mallocFailedMsg = "Memory Allocation failed";
    const char *invalidJsonMsg = "Invalid JSON";

    DX_DIRECT_METHOD_RESPONSE_CODE responseCode = DX_METHOD_NOT_FOUND;
    char *responseMsg = NULL;

    DX_DIRECT_METHOD_BINDING *directMethodBinding = NULL;

    const char *responseMessage = methodNotFoundMsg;
    int result = DX_METHOD_NOT_FOUND;
    size_t responseMessageLength;

    JSON_Value *root_value = NULL;

    // Prepare the payload for the response. This is a heap allocated null terminated string.
    // The Azure IoT Hub SDK is responsible of freeing it.
    *responsePayload = NULL;  // Response payload content.
    *responsePayloadSize = 0; // Response payload content size.

    char *payLoadString = (char *)malloc(payloadSize + 1);
    if (payLoadString == NULL) {
        responseMessage = mallocFailedMsg;
        result = DX_METHOD_FAILED;
        goto cleanup;
    }

    memset(payLoadString, 0x00, payloadSize + 1);

    memcpy(payLoadString, payload, payloadSize);
    payLoadString[payloadSize] = 0; // null terminate string

    root_value = json_parse_string(payLoadString);
    if (root_value == NULL) {
        responseMessage = invalidJsonMsg;
        result = DX_METHOD_FAILED;
        goto cleanup;
    }

    // loop through array of DirectMethodBindings looking for a matching method name
    for (int i = 0; i < _directMethodCount; i++) {
        if (strcmp(method_name, _directMethods[i]->methodName) == 0) {
            directMethodBinding = _directMethods[i];
            break;
        }
    }

    if (directMethodBinding != NULL &&
        directMethodBinding->handler != NULL) { // was a DX_DIRECT_METHOD_BINDING found

        responseCode = directMethodBinding->handler(root_value, directMethodBinding, &responseMsg);

        result = (int)responseCode;

        switch (responseCode) {
        case DX_METHOD_SUCCEEDED: // 200
            responseMessage =
                responseMsg == NULL || strlen(responseMsg) == 0 ? methodSucceededMsg : responseMsg;
            break;
        case DX_METHOD_FAILED: // 500
            responseMessage =
                responseMsg == NULL || strlen(responseMsg) == 0 ? methodErrorMsg : responseMsg;
            break;
        case DX_METHOD_NOT_FOUND:
            break;
        }
    }

cleanup:

    // Prepare the payload for the response.
    // The Azure IoT Hub SDK is responsible of freeing it.
    responseMessageLength = strlen(responseMessage);
    *responsePayloadSize =
        responseMessageLength + 2; // add two as going to wrap the message with quotes for JSON

    *responsePayload = (unsigned char *)malloc(*responsePayloadSize);
    if (*responsePayload != NULL) {
        memset(*responsePayload, 0x00, *responsePayloadSize);

        // response message needs to be wrapped in quotes as it is part of the JSON payload object
        memcpy(*responsePayload, "\"", 1);
        memcpy(*responsePayload + 1, responseMessage, responseMessageLength);
        memcpy(*responsePayload + responseMessageLength + 1, "\"", 1);
    } else {
        *responsePayloadSize = 0;
    }

    if (root_value != NULL) {
        json_value_free(root_value);
    }

    if (payLoadString != NULL) {
        free(payLoadString);
        payLoadString = NULL;
    }

    if (responseMsg != NULL) { // there was memory allocated for a response message so free it now
        free(responseMsg);
        responseMsg = NULL;
    }

    return result;
}