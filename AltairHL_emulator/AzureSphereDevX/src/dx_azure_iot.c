#include "dx_azure_iot.h"

#define MAX_CONNECTION_STATUS_CALLBACKS 5

static bool SetupAzureClient(void);
static bool SetUpAzureIoTHubClientWithDaa(void);
static bool SetUpAzureIoTHubClientWithDaaDpsPnP(void);
static const char *GetMessageResultReasonString(IOTHUB_MESSAGE_RESULT reason);
static const char *GetReasonString(IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason);
static void AzureConnectionHandler(EventLoopTimer *eventLoopTimer);
static void HubConnectionStatusCallback(IOTHUB_CLIENT_CONNECTION_STATUS, IOTHUB_CLIENT_CONNECTION_STATUS_REASON, void *);
static void SendMessageCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT, void *);

static IOTHUB_DEVICE_CLIENT_LL_HANDLE iothubClientHandle = NULL;

static char *iotHubUri = NULL;
static const char *_networkInterface = NULL;
static DX_USER_CONFIG *_userConfig = NULL;
static int outstandingMessageCount = 0;

static char *_pnpModelIdJson = NULL;
static const char *_pnpModelId = NULL;
static const char *_pnpModelIdJsonTemplate = "{\"modelId\":\"%s\"}";

static IOTHUBMESSAGE_DISPOSITION_RESULT (*_messageReceivedCallback)(IOTHUB_MESSAGE_HANDLE message, void *context) = NULL;
static void (*_deviceTwinCallbackHandler)(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payload, size_t payloadSize,
                                          void *userContextCallback);

static int (*_directMethodCallbackHandler)(const char *method_name, const unsigned char *payload, size_t payloadSize,
                                           unsigned char **responsePayload, size_t *responsePayloadSize, void *userContextCallback);

static void (*_connectionStatusCallback[MAX_CONNECTION_STATUS_CALLBACKS])(bool connected);

MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_VALUE);
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(IOTHUB_CLIENT_RESULT, IOTHUB_CLIENT_RESULT_VALUE);

typedef enum {
    DEVICE_NOT_CONNECTED,
    DEVICE_PROVISIONING_ERROR,
    DEVICE_PROVISIONING,
    DEVICE_PROVISION_IOT_CLIENT,
    DEVICE_CONNECTED
} DEVICE_CONNECTION_STATE;

static DEVICE_CONNECTION_STATE deviceConnectionState = DEVICE_NOT_CONNECTED;
static PROV_DEVICE_LL_HANDLE prov_handle = NULL;

/// <summary>
/// Authentication state of the client with respect to the Azure IoT Hub.
/// </summary>
typedef enum {
    /// <summary>Client is not authenticated by the Azure IoT Hub.</summary>
    IoTHubClientAuthenticationState_NotAuthenticated = 0,
    /// <summary>Client has initiated authentication to the Azure IoT Hub.</summary>
    IoTHubClientAuthenticationState_AuthenticationInitiated = 1,
    /// <summary>Client is authenticated by the Azure IoT Hub.</summary>
    IoTHubClientAuthenticationState_Authenticated = 2,
    /// <summary>Has the device been disabled.</summary>
    IoTHubClientAuthenticationState_Device_Disbled = 3
} IoTHubClientAuthenticationState;

// Authentication state with respect to the IoT Hub.
static IoTHubClientAuthenticationState iotHubClientAuthenticationState = IoTHubClientAuthenticationState_NotAuthenticated;

#define dpsUrl "global.azure-devices-provisioning.net"

static PROV_DEVICE_RESULT dpsRegisterStatus = PROV_DEVICE_RESULT_INVALID_STATE;

static DX_TIMER_BINDING azureConnectionTimer = {.period = {0, 0}, // one-shot timer
                                                .name = "azureConnectionTimer",
                                                .handler = &AzureConnectionHandler};

void dx_azureRegisterDeviceTwinCallback(void (*deviceTwinCallbackHandler)(DEVICE_TWIN_UPDATE_STATE updateState,
                                                                          const unsigned char *payload, size_t payloadSize,
                                                                          void *userContextCallback))
{
    _deviceTwinCallbackHandler = deviceTwinCallbackHandler;
}

void dx_azureRegisterDirectMethodCallback(int (*directMethodCallbackHandler)(const char *method_name, const unsigned char *payload,
                                                                             size_t payloadSize, unsigned char **responsePayload,
                                                                             size_t *responsePayloadSize, void *userContextCallback))
{
    _directMethodCallbackHandler = directMethodCallbackHandler;
}

void dx_azureRegisterMessageReceivedNotification(IOTHUBMESSAGE_DISPOSITION_RESULT (*messageReceivedCallback)(IOTHUB_MESSAGE_HANDLE message,
                                                                                                             void *context))
{
    _messageReceivedCallback = messageReceivedCallback;
}

bool dx_azureRegisterConnectionChangedNotification(void (*connectionStatusCallback)(bool connected))
{
    bool result = false;
    for (size_t i = 0; i < MAX_CONNECTION_STATUS_CALLBACKS; i++) {
        if (_connectionStatusCallback[i] == NULL) {
            _connectionStatusCallback[i] = connectionStatusCallback;
            result = true;
            break;
        }
    }
    return result;
}

void dx_azureUnregisterConnectionChangedNotification(void (*connectionStatusCallback)(bool connected))
{
    for (size_t i = 0; i < MAX_CONNECTION_STATUS_CALLBACKS; i++) {
        if (_connectionStatusCallback[i] == connectionStatusCallback) {
            _connectionStatusCallback[i] = NULL;
        }
    }
}

static void dx_azureToDeviceStart(void)
{
    if (azureConnectionTimer.eventLoopTimer == NULL) {
        dx_timerStart(&azureConnectionTimer);
        dx_timerOneShotSet(&azureConnectionTimer, &(struct timespec){1, 0});
    }
}

void dx_azureToDeviceStop(void)
{
    if (azureConnectionTimer.eventLoopTimer != NULL) {
        dx_timerStop(&azureConnectionTimer);
    }
}

bool createPnpModelIdJson(void)
{
    if (!dx_isStringNullOrEmpty(_pnpModelId)) {
        // allow for JSON format "{\"modelId\":\"%s\"}", 14 char, plus terminiating NULL
        size_t modelIdLen = 15;

        // add length of the PnP Module to the model id length
        modelIdLen += strlen(_pnpModelId);

        // Better safe than sorry
        if (_pnpModelIdJson != NULL) {
            free(_pnpModelIdJson);
            _pnpModelIdJson = NULL;
        }

        _pnpModelIdJson = (char *)malloc(modelIdLen);

        if (_pnpModelIdJson == NULL) {
            Log_Debug("ERROR: PnP Model ID malloc failed.\n");
            dx_terminate(DX_ExitCode_PnPModelJsonMemoryAllocationFailed);
            return false;
        }

        memset(_pnpModelIdJson, 0x00, modelIdLen);

        int len = snprintf(_pnpModelIdJson, modelIdLen, _pnpModelIdJsonTemplate, _pnpModelId);
        if (len < 0 || len >= modelIdLen) {
            Log_Debug("ERROR: Cannot write Model ID to buffer.\n");
            dx_terminate(DX_ExitCode_PnPModelJsonFailed);
            return false;
        }
    }
    return true;
}

void dx_azureConnect(DX_USER_CONFIG *userConfig, const char *networkInterface, const char *plugAndPlayModelId)
{
    if (userConfig->connectionType == DX_CONNECTION_TYPE_NOT_DEFINED) {
        Log_Debug("ERROR: Connection type not defined\n");
        dx_terminate(DX_ExitCode_Validate_Connection_Type_Not_Defined);
        return;
    }

    _userConfig = userConfig;
    _networkInterface = networkInterface;
    _pnpModelId = plugAndPlayModelId;

    if (_userConfig->connectionType == DX_CONNECTION_TYPE_DPS) {
        if (!createPnpModelIdJson()) {
            return;
        }
    }

    dx_azureToDeviceStart();
}

/// <summary>
/// If network connection has changed then call all network status changed registered callbacks
/// </summary>
/// <param name="connection_state"></param>
static void ProcessConnectionStatusCallbacks(bool connection_state) {
    static bool previous_connection_state = false;

    if (connection_state != previous_connection_state) {
        previous_connection_state = connection_state;

        for (size_t i = 0; i < MAX_CONNECTION_STATUS_CALLBACKS; i++) {
            if (_connectionStatusCallback[i] != NULL) {
                _connectionStatusCallback[i](connection_state);
            }
        }
    }
}

bool dx_isAzureConnected(void)
{
    if (!dx_isNetworkConnected(_networkInterface) && iotHubClientAuthenticationState == IoTHubClientAuthenticationState_Authenticated) {
        iotHubClientAuthenticationState = IoTHubClientAuthenticationState_NotAuthenticated;
        deviceConnectionState = DEVICE_NOT_CONNECTED;
        ProcessConnectionStatusCallbacks(false);
        return false;
    } else if (iothubClientHandle != NULL && iotHubClientAuthenticationState == IoTHubClientAuthenticationState_Authenticated &&
               dx_isNetworkConnected(_networkInterface)) {
        ProcessConnectionStatusCallbacks(true);
        return true;
    }
    ProcessConnectionStatusCallbacks(false);
    return false;
}

/// <summary>
///     Callback confirming message delivered to IoT Hub.
/// </summary>
/// <param name="result">Message delivery status</param>
/// <param name="context">User specified context</param>
static void SendMessageCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *context)
{
    outstandingMessageCount--;
#if DX_LOGGING_ENABLED
    Log_Debug("INFO: Message received by IoT Hub. Result is: %d\n", result);
#endif
}

/// <summary>
///     Azure IoT Hub DoWork Handler with back off up to 5 seconds for network disconnect
/// </summary>
static void AzureConnectionHandler(EventLoopTimer *eventLoopTimer)
{
    struct timespec nextEventPeriod = {0, 0};

    if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0) {
        dx_terminate(DX_ExitCode_AzureCloudToDeviceHandler);
        return;
    }

    // network disconnected but was previously authenticated
    if (!dx_isNetworkConnected(_networkInterface) && iotHubClientAuthenticationState == IoTHubClientAuthenticationState_Authenticated) {
        iotHubClientAuthenticationState = IoTHubClientAuthenticationState_NotAuthenticated;
        deviceConnectionState = DEVICE_NOT_CONNECTED;
    }

    switch (iotHubClientAuthenticationState) {
    case IoTHubClientAuthenticationState_NotAuthenticated:
        SetupAzureClient();
        nextEventPeriod = (struct timespec){1, 0};
        break;
    case IoTHubClientAuthenticationState_AuthenticationInitiated:
        IoTHubDeviceClient_LL_DoWork(iothubClientHandle);
        nextEventPeriod = (struct timespec){1, 0};
        break;
    case IoTHubClientAuthenticationState_Authenticated:
        IoTHubDeviceClient_LL_DoWork(iothubClientHandle);
        nextEventPeriod = (struct timespec){IOT_HUB_POLL_TIME_SECONDS, IOT_HUB_POLL_TIME_NANOSECONDS};
        break;
    case IoTHubClientAuthenticationState_Device_Disbled:
        iotHubClientAuthenticationState = IoTHubClientAuthenticationState_NotAuthenticated;
        deviceConnectionState = DEVICE_NOT_CONNECTED;
        nextEventPeriod = (struct timespec){1, 0};
        break;
    }

    dx_timerOneShotSet(&azureConnectionTimer, &nextEventPeriod);
}

bool dx_azurePublish(const void *message, size_t messageLength, DX_MESSAGE_PROPERTY **messageProperties, size_t messagePropertyCount,
                     DX_MESSAGE_CONTENT_PROPERTIES *messageContentProperties)
{
    IOTHUB_CLIENT_RESULT result;
    IOTHUB_MESSAGE_RESULT messageResult;
    IOTHUB_MESSAGE_HANDLE messageHandle;

    if (messageLength == 0) {
        return true;
    }

    if (!dx_isAzureConnected()) {
        // Log_Debug("FAILED: Not connected to Azure IoT\n");
        return false;
    }

    messageHandle = IoTHubMessage_CreateFromByteArray(message, messageLength);

    if (messageHandle == NULL) {
        Log_Debug("ERROR: unable to create a new IoTHubMessage\n");
        return false;
    }

    // add system content properties
    if (messageContentProperties != NULL) {
        if (!dx_isStringNullOrEmpty(messageContentProperties->contentEncoding)) {
            if ((messageResult = IoTHubMessage_SetContentEncodingSystemProperty(
                     messageHandle, messageContentProperties->contentEncoding)) != IOTHUB_MESSAGE_OK) {
                Log_Debug("ERROR: ContentEncodingSystemProperty: %s\n", GetMessageResultReasonString(messageResult));
                return false;
            }
        }

        if (!dx_isStringNullOrEmpty(messageContentProperties->contentType)) {
            if ((messageResult = IoTHubMessage_SetContentTypeSystemProperty(messageHandle, messageContentProperties->contentType)) !=
                IOTHUB_MESSAGE_OK) {
                Log_Debug("ERROR: ContentTypeSystemProperty: %s\n", GetMessageResultReasonString(messageResult));
                return false;
            }
        }
    }

    // add application properties
    if (messageProperties != NULL && messagePropertyCount > 0) {
        for (size_t i = 0; i < messagePropertyCount; i++) {
            if (!dx_isStringNullOrEmpty(messageProperties[i]->key) && !dx_isStringNullOrEmpty(messageProperties[i]->value)) {
                if ((messageResult = IoTHubMessage_SetProperty(messageHandle, messageProperties[i]->key, messageProperties[i]->value)) !=
                    IOTHUB_MESSAGE_OK) {
                    Log_Debug("ERROR: Setting key/value properties: %s, %s, %s\n", messageProperties[i]->key, messageProperties[i]->value,
                              GetMessageResultReasonString(messageResult));
                    return false;
                }
            }
        }
    }

    if ((result = IoTHubDeviceClient_LL_SendEventAsync(iothubClientHandle, messageHandle, SendMessageCallback,
                                                       /*&callback_param*/ 0)) != IOTHUB_CLIENT_OK) {
        Log_Debug("ERROR: failed to hand over the message to IoTHubClient\n");
    } else {
        outstandingMessageCount++;
    }

    IoTHubMessage_Destroy(messageHandle);

    // IoTHubDeviceClient_LL_DoWork(iothubClientHandle);

    return result == IOTHUB_CLIENT_OK;
}

IOTHUB_DEVICE_CLIENT_LL_HANDLE dx_azureClientHandleGet(void)
{
    return iothubClientHandle;
}

static IOTHUBMESSAGE_DISPOSITION_RESULT HubMessageReceivedCallback(IOTHUB_MESSAGE_HANDLE message, void *context)
{
    if (_messageReceivedCallback != NULL) {
        return _messageReceivedCallback(message, context);
    }

    // if no active callbacks then just return message accepted
    return IOTHUBMESSAGE_ACCEPTED;
}

static void HubDeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payload, size_t payloadSize,
                                  void *userContextCallback)
{
    if (_deviceTwinCallbackHandler != NULL) {
        _deviceTwinCallbackHandler(updateState, payload, payloadSize, userContextCallback);
    }
}

static int HubDirectMethodCallback(const char *method_name, const unsigned char *payload, size_t payloadSize,
                                   unsigned char **responsePayload, size_t *responsePayloadSize, void *userContextCallback)
{
    if (_directMethodCallbackHandler != NULL) {
        return _directMethodCallbackHandler(method_name, payload, payloadSize, responsePayload, responsePayloadSize, userContextCallback);
    } else {
        return -1;
    }
}

/// <summary>
///     Sets up the Azure IoT Hub connection (creates the iothubClientHandle)
///     When the SAS Token for a device expires the connection needs to be recreated
///     which is why this is not simply a one time call.
/// </summary>
static bool SetupAzureClient()
{
    if (iothubClientHandle != NULL) {
        IoTHubDeviceClient_LL_Destroy(iothubClientHandle);
        iothubClientHandle = NULL;
    }

    switch (_userConfig->connectionType) {
    case DX_CONNECTION_TYPE_HOSTNAME:
        if (!SetUpAzureIoTHubClientWithDaa()) {
            return false;
        }
        break;
    case DX_CONNECTION_TYPE_DPS:
        if (!SetUpAzureIoTHubClientWithDaaDpsPnP()) {
            return false;
        }
        break;
    default:
        return false;
        break;
    }

    if (deviceConnectionState != DEVICE_CONNECTED) {
        return false;
    }

    iotHubClientAuthenticationState = IoTHubClientAuthenticationState_AuthenticationInitiated;

    IoTHubDeviceClient_LL_SetDeviceTwinCallback(iothubClientHandle, HubDeviceTwinCallback, NULL);
    IoTHubDeviceClient_LL_SetDeviceMethodCallback(iothubClientHandle, HubDirectMethodCallback, NULL);
    IoTHubDeviceClient_LL_SetConnectionStatusCallback(iothubClientHandle, HubConnectionStatusCallback, NULL);
    IoTHubDeviceClient_LL_SetMessageCallback(iothubClientHandle, HubMessageReceivedCallback, NULL);

    IoTHubDeviceClient_LL_DoWork(iothubClientHandle);

    return true;
}

/// <summary>
/// Called by both DPS and Direct X509 functions to set up the connection.
/// </summary>
/// <param name="hostname"></param>
/// <returns></returns>
static bool ConnectToIotHub(const char *hostname)
{
    bool urlAutoEncodeDecode = true;
    const int deviceIdForDaaCertUsage = 1;
    IOTHUB_CLIENT_RESULT iothubResult;

    if (dx_isStringNullOrEmpty(hostname)) {
        return false;
    }

    if ((iothubClientHandle = IoTHubDeviceClient_LL_CreateWithAzureSphereFromDeviceAuth(hostname, &MQTT_Protocol)) == NULL) {
        Log_Debug("ERROR: Failed to create client IoT Hub Client Handle\n");
        return false;
    }

    // IOTHUB_CLIENT_RESULT iothub_result
    if ((iothubResult = IoTHubDeviceClient_LL_SetOption(iothubClientHandle, "SetDeviceId", &deviceIdForDaaCertUsage)) != IOTHUB_CLIENT_OK) {
        Log_Debug("ERROR: Failed to set Device ID on IoT Hub Client: %s\n", IOTHUB_CLIENT_RESULTStrings(iothubResult));
        return false;
    }

    // Sets auto URL encoding on IoT Hub Client
    if ((iothubResult = IoTHubDeviceClient_LL_SetOption(iothubClientHandle, OPTION_AUTO_URL_ENCODE_DECODE, &urlAutoEncodeDecode)) !=
        IOTHUB_CLIENT_OK) {
        Log_Debug("ERROR: Failed to set auto Url encode option on IoT Hub Client: %s\n", IOTHUB_CLIENT_RESULTStrings(iothubResult));
        return false;
    }

    if (_pnpModelIdJson != NULL) {
        if ((iothubResult = IoTHubDeviceClient_LL_SetOption(iothubClientHandle, OPTION_MODEL_ID, _pnpModelId)) != IOTHUB_CLIENT_OK) {
            Log_Debug("ERROR: Failed to set PnP Model ID %s, for Model ID: %s\n", IOTHUB_CLIENT_RESULTStrings(iothubResult),
                      OPTION_MODEL_ID);
            return false;
        }
    }

    return true;
}

/// <summary>
///     Sets up the Azure IoT Hub connection (creates the iothubClientHandle)
///     with DAA
/// </summary>
static bool SetUpAzureIoTHubClientWithDaa(void)
{
    int retError = 0;

    deviceConnectionState = DEVICE_NOT_CONNECTED;

    // If network/DAA are not ready, fail out (which will trigger a retry)
    if (!dx_isDeviceAuthReady() || !dx_isNetworkConnected(_networkInterface)) {
        return false;
    }

    // Set up auth type
    if ((retError = iothub_security_init(IOTHUB_SECURITY_TYPE_X509)) != 0) {
        Log_Debug("ERROR: iothub_security_init failed with error %d.\n", retError);
        return false;
    }

    if (!ConnectToIotHub(_userConfig->hostname)) {

        if (iothubClientHandle != NULL) {
            IoTHubDeviceClient_LL_Destroy(iothubClientHandle);
            iothubClientHandle = NULL;
        }

        goto cleanup;
    }

    deviceConnectionState = DEVICE_CONNECTED;

cleanup:
    iothub_security_deinit();

    return deviceConnectionState == DEVICE_CONNECTED;
}

/// <summary>
///     DPS provisioning callback with status
/// </summary>
static void RegisterProvisioningDeviceCallback(PROV_DEVICE_RESULT registerResult, const char *callbackHubUri, const char *deviceId,
                                               void *userContext)
{
    dpsRegisterStatus = registerResult;

    if (registerResult == PROV_DEVICE_RESULT_OK && callbackHubUri != NULL) {

        size_t uriSize = strlen(callbackHubUri) + 1; // +1 for NULL string termination

        if (iotHubUri != NULL) {
            free(iotHubUri);
            iotHubUri = NULL;
        }

        iotHubUri = (char *)malloc(uriSize);
        if (iotHubUri == NULL) {
            Log_Debug("ERROR: IoT Hub URI malloc failed.\n");
        } else {
            memset(iotHubUri, 0, uriSize);
            strncpy(iotHubUri, callbackHubUri, uriSize);
        }
    }
}

/// <summary>
///     Provision with DPS and assign IoT Plug and Play Model ID
/// </summary>
static bool SetUpAzureIoTHubClientWithDaaDpsPnP(void)
{
    const int deviceIdForDaaCertUsage = 1; // Use DAA cert in provisioning flow - requires the SetDeviceId option to be set on the
                                           // provisioning client.
    PROV_DEVICE_RESULT prov_result;
    static bool security_init_called = false;
    static int provisionCompletedMaxRetry = 0;

    if (!dx_isDeviceAuthReady() || !dx_isNetworkConnected(_networkInterface)) {
        return false;
    }

    switch (deviceConnectionState) {
    case DEVICE_NOT_CONNECTED:
    case DEVICE_PROVISIONING_ERROR:

        dpsRegisterStatus = PROV_DEVICE_RESULT_INVALID_STATE;
        provisionCompletedMaxRetry = 0;

        // Initiate security with X509 Certificate
        if (prov_dev_security_init(SECURE_DEVICE_TYPE_X509) != 0) {
            Log_Debug("ERROR: Failed to initiate X509 Certificate security\n");
            deviceConnectionState = DEVICE_PROVISIONING_ERROR;
            goto cleanup;
        }

        security_init_called = true;

        // Create Provisioning Client for communication with DPS using MQTT protocol
        if ((prov_handle = Prov_Device_LL_Create(dpsUrl, _userConfig->idScope, Prov_Device_MQTT_Protocol)) == NULL) {
            Log_Debug("ERROR: Failed to create Provisioning Client\n");
            deviceConnectionState = DEVICE_PROVISIONING_ERROR;
            goto cleanup;
        }

        // Sets Device ID on Provisioning Client
        if ((prov_result = Prov_Device_LL_SetOption(prov_handle, "SetDeviceId", &deviceIdForDaaCertUsage)) != PROV_DEVICE_RESULT_OK) {
            Log_Debug("ERROR: Failed to set Device ID in Provisioning Client, error=%d\n", prov_result);
            deviceConnectionState = DEVICE_PROVISIONING_ERROR;
            goto cleanup;
        }

        // Sets Model ID provisioning data
        if (_pnpModelIdJson != NULL) {
            if ((prov_result = Prov_Device_LL_Set_Provisioning_Payload(prov_handle, _pnpModelIdJson)) != PROV_DEVICE_RESULT_OK) {
                Log_Debug("Error: Failed to set Model ID in Provisioning Client, error=%d\n", prov_result);
                deviceConnectionState = DEVICE_PROVISIONING_ERROR;
                goto cleanup;
            }
        }

        // Sets the callback function for device registration
        if ((prov_result = Prov_Device_LL_Register_Device(prov_handle, RegisterProvisioningDeviceCallback, NULL, NULL, NULL)) !=
            PROV_DEVICE_RESULT_OK) {
            Log_Debug("ERROR: Failed to set callback function for device registration, error=%d\n", prov_result);
            deviceConnectionState = DEVICE_PROVISIONING_ERROR;
            goto cleanup;
        }

        deviceConnectionState = DEVICE_PROVISIONING;

        break;

    case DEVICE_PROVISIONING:

        Prov_Device_LL_DoWork(prov_handle);
        if (dpsRegisterStatus == PROV_DEVICE_RESULT_OK) {
            deviceConnectionState = DEVICE_PROVISION_IOT_CLIENT;
        }

        // Retry cadence is once a second, wait max 60 seconds for call to
        // RegisterProvisioningDeviceCallback() to complete else restart provisioning process
        if (provisionCompletedMaxRetry++ > 60) {
            deviceConnectionState = DEVICE_PROVISIONING_ERROR;
            Log_Debug("ERROR: Failed to register device with provisioning service: %s\n", PROV_DEVICE_RESULTStrings(dpsRegisterStatus));
        }

        break;

    case DEVICE_PROVISION_IOT_CLIENT:

        if (!ConnectToIotHub(iotHubUri)) {

            if (iothubClientHandle != NULL) {
                IoTHubDeviceClient_LL_Destroy(iothubClientHandle);
                iothubClientHandle = NULL;
            }

            deviceConnectionState = DEVICE_PROVISIONING_ERROR;
            goto cleanup;
        }

        deviceConnectionState = DEVICE_CONNECTED;

        break;

    default:
        break;
    }

cleanup:
    if (deviceConnectionState == DEVICE_CONNECTED || deviceConnectionState == DEVICE_PROVISIONING_ERROR) {

        if (prov_handle != NULL) {
            Prov_Device_LL_Destroy(prov_handle);
            prov_handle = NULL;
        }

        if (security_init_called) {
            prov_dev_security_deinit();
            security_init_called = false;
        }
    }

    return deviceConnectionState == DEVICE_CONNECTED;
}

/// <summary>
///     Sets the IoT Hub authentication state for the app
///     The SAS Token expires which will set the authentication state
/// </summary>
static void HubConnectionStatusCallback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason,
                                        void *userContextCallback)
{
    switch (result) {
    case IOTHUB_CLIENT_CONNECTION_AUTHENTICATED:
        Log_Debug("Result: IOTHUB_CLIENT_CONNECTION_AUTHENTICATED\n");
        break;
    case IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED:
        Log_Debug("Result: IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED\n");
        break;
    }

    Log_Debug("IoT Hub Connection Status reason: %s\n", GetReasonString(reason));

    if (result != IOTHUB_CLIENT_CONNECTION_AUTHENTICATED) {
        if (reason == IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED || reason == IOTHUB_CLIENT_CONNECTION_NO_NETWORK) {

            iotHubClientAuthenticationState = IoTHubClientAuthenticationState_Device_Disbled;
            Log_Debug("Hub status callback: IoTHubClientAuthenticationState_Device_Disbled\n");

        } else {
            iotHubClientAuthenticationState = IoTHubClientAuthenticationState_NotAuthenticated;
            Log_Debug("Hub status callback: IoTHubClientAuthenticationState_NotAuthenticated\n");
        }

        deviceConnectionState = DEVICE_NOT_CONNECTED;

    } else {
        iotHubClientAuthenticationState = IoTHubClientAuthenticationState_Authenticated;
    }

    dx_isAzureConnected();
}

static const char *GetMessageResultReasonString(IOTHUB_MESSAGE_RESULT reason)
{
    static char *reasonString = "unknown reason";
    switch (reason) {
    case IOTHUB_MESSAGE_OK:
        reasonString = "IOTHUB_MESSAGE_OK";
        break;
    case IOTHUB_MESSAGE_INVALID_ARG:
        reasonString = "IOTHUB_MESSAGE_INVALID_ARG";
        break;
    case IOTHUB_MESSAGE_INVALID_TYPE:
        reasonString = "IOTHUB_MESSAGE_INVALID_TYPE";
        break;
    case IOTHUB_MESSAGE_ERROR:
        reasonString = "IOTHUB_MESSAGE_ERROR";
        break;
    default:
        break;
    }
    return reasonString;
}

/// <summary>
///     Converts the IoT Hub connection status reason to a string.
/// </summary>
static const char *GetReasonString(IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason)
{
    static char *reasonString = "unknown reason";
    switch (reason) {
    case IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN:
        reasonString = "IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN";
        break;
    case IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED:
        reasonString = "IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED";
        break;
    case IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL:
        reasonString = "IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL";
        break;
    case IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED:
        reasonString = "IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED";
        break;
    case IOTHUB_CLIENT_CONNECTION_NO_NETWORK:
        reasonString = "IOTHUB_CLIENT_CONNECTION_NO_NETWORK";
        break;
    case IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR:
        reasonString = "IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR";
        break;
    case IOTHUB_CLIENT_CONNECTION_OK:
        reasonString = "IOTHUB_CLIENT_CONNECTION_OK";
        break;
    case IOTHUB_CLIENT_CONNECTION_NO_PING_RESPONSE:
        reasonString = "IOTHUB_CLIENT_CONNECTION_NO_PING_RESPONSE";
        break;
    }
    return reasonString;
}
