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
//#include "examples/mqttexample.h"
#include "examples/mqttnet.h"

#define MQTT_CLIENT_ID		"altair%d"
#define PUB_TOPIC_DATA		"altair/%d/web"
#define SUB_TOPIC_DATA		"altair/%d/dev"
#define SUB_TOPIC_CONTROL	"altair/%d/dev/ctrl"
#define SUB_TOPIC_PASTE		"altair/%d/dev/paste"
#define PUB_TOPIC_VDISK_READ		"altair/%d/vdisk/read"
#define PUB_TOPIC_VDISK_WRITE		"altair/%d/vdisk/write"
#define SUB_TOPIC_VDISK_RESPONSE	"altair/%d/vdisk/data"

typedef enum {
	TOPIC_UNKNOWN,
	TOPIC_DATA_SUB,
	TOPIC_PASTE_SUB,
	TOPIC_CONTROL_SUB,
	TOPIC_VDISK_SUB
} TOPIC_TYPE;


typedef struct  __attribute__((packed)) {
	uint32_t offset;
	uint8_t data[137];
} vdisk_mqtt_write_sector_t;

extern DX_TIMER_BINDING mqttConnectTimer;
extern int console_fd;
extern bool local_serial;
extern DX_DEVICE_TWIN_BINDING dt_channelId;
//extern bool dirty_queue;
extern bool dirty_buffer;

bool is_mqtt_connected(void);
TOPIC_TYPE topic_type(char* topic_name, size_t topic_name_length);
void init_mqtt(void (*publish_callback)(MqttMessage* msg), void (*mqtt_connected_cb)(void));
void publish_message(const char* message, size_t message_length);
void publish_character(char character);
void vdisk_mqtt_read_sector(uint32_t offset);
void vdisk_mqtt_write_sector(vdisk_mqtt_write_sector_t* write_sector);
void queue_mqtt_message(const uint8_t* data, size_t data_length);
void send_partial_message(void);
//void invoke_mqtt_ping(void);
void send_queued_messages(void);
