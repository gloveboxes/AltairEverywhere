#pragma once

/*
MIT License
Copyright (c) Avnet Corporation. All rights reserved.
Author: Brian Willess

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <applibs/log.h>

#include "applibs_versions.h"
#include "iothub_device_client_ll.h"
#include "parson.h"

#include "dx_exit_codes.h"
#include "dx_azure_iot.h"
#include "dx_timer.h"
#include "dx_json_serializer.h" // for DX_JSON_TYPE enum
#include "dx_utilities.h"
#include "dx_azure_iot.h"
#include "dx_config.h"

#define DX_AVNET_IOT_CONNECT_GUID_LEN 36
#define DX_AVNET_IOT_CONNECT_SID_LEN 64
#define DX_AVNET_IOT_CONNECT_METADATA 256
#define DX_AVNET_IOT_CONNECT_JSON_BUFFER_SIZE 512

// The gateway field length is long to acomidate long ids from child devices
#define DX_AVNET_IOT_CONNECT_GW_FIELD_LEN 128+64

typedef enum
{
	AVT_ERROR_CODE_OK = 0,
	AVT_ERROR_CODE_DEV_NOT_REGISTERED = 1,
    AVT_ERROR_CODE_DEV_AUTO_REGISTERED = 2,
    AVT_ERROR_CODE_DEV_NOT_FOUND = 3,
    AVT_ERROR_CODE_DEV_INACTIVE = 4,
    AVT_ERROR_CODE_OBJ_MOVED = 5,
    AVT_ERROR_CODE_CPID_NOT_FOUND = 6,
    AVT_ERROR_CODE_COMPANY_NOT_FOUND = 7
} AVT_IOTC_ERROR_CODES;

typedef enum
{
	AVT_RESPONSE_CODE_OK_CHILD_ADDED = 0,
	AVT_RESPONSE_CODE_MISSING_CHILD_TAG = 1,
    AVT_RESPONSE_CODE_MISSING_CHILD_UID = 2,
    AVT_RESPONSE_CODE_MISSING_CHILD_DISPLAY_NAME = 3,
    AVT_RESPONSE_CODE_GW_DEVICE_NOT_FOUND= 4,
    AVT_RESPONSE_CODE_UNKNOWN_ERROR = 5,
    AVT_RESPONSE_CODE_INVALID_CHILD_TAG = 6,
    AVT_RESPONSE_CODE_TAG_NAME_CAN_NOT_BE_THE_SAME_AS_GATEWAY_DEVICE = 7,
    AVT_RESPONSE_CODE_UID_ALREADY_EXISTS = 8
} AVT_IOTC_221_RESPONSE_CODES;

// Linked list node definition
typedef struct node {
	struct node* next;
	struct node* prev;
	char tg[DX_AVNET_IOT_CONNECT_GW_FIELD_LEN];
	char id[DX_AVNET_IOT_CONNECT_GW_FIELD_LEN];
} gw_child_list_node_t;

/// <summary>
/// Takes properly formatted JSON telemetry data and wraps it with ToTConnect
/// metaData.  Returns false if the application has not received the IoTConnect hello
/// response, if the passed in buffer is too small for the modified JSON document, or
/// if the passed in JSON is malformed.  If childDevice is passed in is non-NULL, the payload
/// will be formatted for a gateway child device using the id and tag values set in the
/// gw_child_list_note_t structure.
/// </summary>
/// <param name="originalJsonMessage"></param>
/// <param name="modifiedJsonMessage"></param>
/// <param name="modifiedBufferSize"></param>
/// <param name="childDevice"></param>
/// <returns></returns>
bool dx_avnetJsonSerializePayload(const char *originalJsonMessage, char *modifiedJsonMessage, size_t modifiedBufferSize, gw_child_list_node_t* childDevice);

/// <summary>
/// Returns a JSON document containing passed in <type, key, value> triples.  The JSON document
/// will have the IoTConnect metadata header prepended to the telemetry data.   If childDevice is 
/// passed in is non-NULL, the payload will be formatted for a gateway child device using the id 
/// and tag values set in the gw_child_list_note_t structure.
/// </summary>
/// <param name="jsonMessageBuffer"></param>
/// <param name="bufferSize"></param>
/// <parm name="childDevice"></param>
/// <param name="key_value_pair_count"></param>
/// <param name="(type, key, value) triples"></parm>
/// <returns></returns>
bool dx_avnetJsonSerialize(char * jsonMessageBuffer, size_t bufferSize, gw_child_list_node_t* childDevice, int key_value_pair_count, ...);

/// <summary>
/// Initializes the IoTConnect timer.  This routine should be called on application init
/// </summary>
/// <returns></returns>
void dx_avnetConnect(DX_USER_CONFIG *userConfig, const char *networkInterface);

/// <summary>
/// This routine returns the status of the IoTConnect connection.  
/// </summary>
/// <returns></returns>
bool dx_isAvnetConnected(void);

/// <summary>
/// Finds the child node in the linked list by id and returns
/// a pointer to the child node.
/// </summary>
/// <parm name="id"></param>
/// <returns></returns>
gw_child_list_node_t *dx_avnetFindChild(const char* id);

/// <summary>
/// Sends a message to IoTConnect to create a child device associated with 
/// the device that the application is running on.  IoTConnect will send
/// a 221 response message when the child is added and the library will
/// add a new child device to the child list.
/// </summary>
/// <param name="id"></param>
/// <param name="tg"></param>
/// <parm name="dn"></param>
/// <returns></returns>
void dx_avnetCreateChildOnIoTConnect(const char* id, const char* tg, const char* dn);

/// <summary>
/// Sends a message to IoTConnect to delete a child device associated with 
/// the device that the application is running on.  IoTConnect will send
/// a 222 response message when the child is deleted and the library will
/// device the child from the child list.
/// </summary>
/// <param name="id"></param>
/// <returns></returns>
void dx_avnetDeleteChildOnIoTConnect(const char* id);

/// <summary>
/// Returns a pointer to the head of the children list which
/// can be used to traverse the children list, see dx_avnetGetNextChild()
/// </summary>
/// <returns></returns>
gw_child_list_node_t* dx_avnetGetFirstChild(void);

/// <summary>
/// Returns a pointer to child node after the node passed in to the function.
/// If there is not a next node, the routine returns NULL.
/// </summary>
/// <returns></returns>
gw_child_list_node_t* dx_avnetGetNextChild(gw_child_list_node_t* currentChild);

/// <summary>
/// Outputs all child devices in the children list to debug
/// </summary>
/// <returns></returns>
void dx_avnetPrintGwChildrenList(void);