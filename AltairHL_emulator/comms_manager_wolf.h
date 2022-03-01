/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "dx_device_twins.h"
#include "dx_timer.h"
#include "storage.h"
#include <applibs/log.h>
#include <applibs/storage.h>
#include <stdbool.h>
#include <time.h>

#include "wolfmqtt/mqtt_client.h"
#include "examples/mqttnet.h"

#ifndef ALTAIR_MQTT_BROKER
#define ALTAIR_MQTT_BROKER "localhost"
#endif

#ifndef ALTAIR_MQTT_BROKER_PORT
#define ALTAIR_MQTT_BROKER_PORT 1883
#endif

#ifndef ALTAIR_MQTT_IDENTITY
#define ALTAIR_MQTT_IDENTITY "altair"
#endif


#ifdef ENABLE_MQTT_TLS
static const char* mTlsCaFile;
static const char* mTlsCertFile;
static const char* mTlsKeyFile;
#ifdef HAVE_SNI
static int useSNI = 0;
static const char* mTlsSniHostName;
#endif
#endif

#define MQTT_CLIENT_ID		"altair%d"
#define PUB_TOPIC_DATA		"altair/%d/web"
#define SUB_TOPIC_DATA		"altair/%d/dev"
#define SUB_TOPIC_CONTROL	"altair/%d/dev/ctrl"
#define SUB_TOPIC_PASTE		"altair/%d/dev/paste"

typedef enum {
	TOPIC_UNKNOWN,
	TOPIC_DATA_SUB,
	TOPIC_PASTE_SUB,
	TOPIC_CONTROL_SUB
} TOPIC_TYPE;


typedef struct  __attribute__((packed)) {
	uint32_t offset;
	uint8_t data[137];
} vdisk_mqtt_write_sector_t;

extern volatile bool dirty_buffer;

TOPIC_TYPE topic_type(char* topic_name, size_t topic_name_length);
int init_mqtt(int argc, char *argv[], void (*publish_callback)(MqttMessage *msg), void (*mqtt_connected_cb)(void), const char* networkInterface);
void publish_message(const char* message, size_t message_length);
void publish_character(char character);
void send_partial_message(void);
