/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "comms_manager_wolf.h"

#define MAX_BUFFER_SIZE 1024

static bool mqtt_connected = false;
static bool got_disconnected = true;

static void ping_handler(EventLoopTimer* eventLoopTimer);


//#define QUEUE_BUFFER_SIZE 256 // this is small buffer to decouple sends from the MQTT-C message received callback
//uint8_t queued_buffer[QUEUE_BUFFER_SIZE];
//size_t queued_buffer_length = 0;

MqttTopic topics[4];

static char mqtt_client_id[30] = { 0 };
static char pub_topic_data[30] = { 0 };
static char sub_topic_data[30] = { 0 };
static char sub_topic_control[30] = { 0 };
static char sub_topic_paste[30] = { 0 };
static char sub_topic_vdisk_data[30] = { 0 };
static char pub_topic_vdisk_read[30] = { 0 };
static char pub_topic_vdisk_write[30] = { 0 };

static void (*_publish_callback)(MqttMessage* msg);
static void (*_mqtt_connected_cb)(void);

static byte tx_buf[MAX_BUFFER_SIZE];
static byte rx_buf[MAX_BUFFER_SIZE];

static char output_buffer[MAX_BUFFER_SIZE / 4];
static size_t output_buffer_length = 0;

static MQTTCtx gMqttCtx;

static DX_TIMER_BINDING ping_timer = { .period = {30,0}, .handler = ping_handler, .name = "ping_timer" };

/* locals */
static word16 mPacketIdLast;

/* argument parsing */
//static int myoptind = 0;
//static char* myoptarg = NULL;

static void ping_handler(EventLoopTimer* eventLoopTimer) {
	int rc;
	if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0) {
		dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
		return;
	}
	if (mqtt_connected) {
		if ((rc = MqttClient_Ping(&gMqttCtx.client)) != MQTT_CODE_SUCCESS) {
			Log_Debug("MQTT Ping Keep Alive Error: %s (%d)\n", MqttClient_ReturnCodeToString(rc), rc);
		}
	}
}

void mqtt_init_ctx(MQTTCtx* mqttCtx) {
	XMEMSET(mqttCtx, 0, sizeof(MQTTCtx));
	mqttCtx->host = ALTAIR_MQTT_HOST;
	mqttCtx->port = ALTAIR_MQTT_SECURE_PORT_WOLF;
	mqttCtx->qos = DEFAULT_MQTT_QOS;
	mqttCtx->use_tls = 1;
	mqttCtx->clean_session = 1;
	mqttCtx->keep_alive_sec = DEFAULT_KEEP_ALIVE_SEC;
	//mqttCtx->client_id = kDefClientId;
	//mqttCtx->topic_name = kDefTopicName;
	mqttCtx->cmd_timeout_ms = DEFAULT_CMD_TIMEOUT_MS;
#ifdef WOLFMQTT_V5
	mqttCtx->max_packet_size = DEFAULT_MAX_PKT_SZ;
	mqttCtx->topic_alias = 1;
	mqttCtx->topic_alias_max = 1;
#endif
}


void mqtt_free_ctx(MQTTCtx* mqttCtx) {
	if (mqttCtx == NULL) {
		return;
	}

	if (mqttCtx->dynamicTopic && mqttCtx->topic_name) {
		WOLFMQTT_FREE((char*)mqttCtx->topic_name);
		mqttCtx->topic_name = NULL;
	}
	if (mqttCtx->dynamicClientId && mqttCtx->client_id) {
		WOLFMQTT_FREE((char*)mqttCtx->client_id);
		mqttCtx->client_id = NULL;
	}
}

#if defined(__GNUC__) && !defined(NO_EXIT)
__attribute__((noreturn))
#endif
int err_sys(const char* msg) {
	if (msg) {
		PRINTF("wolfMQTT error: %s", msg);
	}
	exit(EXIT_FAILURE);
}


word16 mqtt_get_packetid(void) {
	/* Check rollover */
	if (mPacketIdLast >= MAX_PACKET_ID) {
		mPacketIdLast = 0;
	}

	return ++mPacketIdLast;
}

#ifdef WOLFMQTT_NONBLOCK

static word32 mqtt_get_timer_seconds(void) {
	word32 timer_sec = 0;
	timer_sec = (word32)time(0);
	return timer_sec;
}

int mqtt_check_timeout(int rc, word32* start_sec, word32 timeout_sec) {
	word32 elapsed_sec;

	/* if start seconds not set or is not continue */
	if (*start_sec == 0 || rc != MQTT_CODE_CONTINUE) {
		*start_sec = mqtt_get_timer_seconds();
		return rc;
	}

	elapsed_sec = mqtt_get_timer_seconds();
	if (*start_sec < elapsed_sec) {
		elapsed_sec -= *start_sec;
		if (elapsed_sec >= timeout_sec) {
			*start_sec = mqtt_get_timer_seconds();
			PRINTF("Timeout timer %d seconds", timeout_sec);
			return MQTT_CODE_ERROR_TIMEOUT;
		}
	}

	return rc;
}
#endif /* WOLFMQTT_NONBLOCK */


int mqtt_tls_cb(MqttClient* client) {
	int ret;
	char* abs_path = NULL;
	int rc = WOLFSSL_FAILURE;

	client->tls.ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());
	if (client->tls.ctx == NULL) {
		return WOLFSSL_FAILURE;
	}

	/* Load root CA certificates full path */
	abs_path = Storage_GetAbsolutePathInImagePackage(MQTT_CA_CERTIFICATE);
	if (abs_path) {
		rc = wolfSSL_CTX_load_verify_locations(client->tls.ctx, abs_path, NULL);
		if (rc != WOLFSSL_SUCCESS) {
			free(abs_path);
			abs_path = NULL;

			//Log_Debug("Error loading CA %s: %d", mTlsCaFile, rc);
			return rc;
		}
		free(abs_path);
		abs_path = NULL;
	}

	abs_path = Storage_GetAbsolutePathInImagePackage(MQTT_CLIENT_PRIVATE_KEY);
	if (abs_path == NULL) {
		Log_Debug("ERROR: the private key path could not be resolved\n");
		return WOLFSSL_FAILURE;
	}

	ret = wolfSSL_CTX_use_PrivateKey_file(client->tls.ctx, abs_path, WOLFSSL_FILETYPE_PEM);
	if (ret != WOLFSSL_SUCCESS) {
		Log_Debug("ERROR: failed to private key certificate\n");
		free(abs_path);
		abs_path = NULL;
		return WOLFSSL_FAILURE;
	}

	free(abs_path);
	abs_path = NULL;


	abs_path = Storage_GetAbsolutePathInImagePackage(MQTT_CLIENT_CERTIFICATE);
	if (abs_path == NULL) {
		Log_Debug("ERROR: the client certificate path could not be resolved\n");
		return WOLFSSL_FAILURE;
	}

	ret = wolfSSL_CTX_use_certificate_file(client->tls.ctx, abs_path, WOLFSSL_FILETYPE_PEM);
	if (ret != WOLFSSL_SUCCESS) {
		Log_Debug("ERROR: failed to client certificate\n");
		free(abs_path);
		abs_path = NULL;
		return WOLFSSL_FAILURE;
	}

	free(abs_path);
	abs_path = NULL;

	return WOLFSSL_SUCCESS;
}

TOPIC_TYPE topic_type(char* topic_name, size_t topic_name_length) {
	if (strncmp(topic_name, sub_topic_data, topic_name_length) == 0) {
		return TOPIC_DATA_SUB;
	} else if (strncmp(topic_name, sub_topic_control, topic_name_length) == 0) {
		return TOPIC_CONTROL_SUB;
	} else if (strncmp(topic_name, sub_topic_paste, topic_name_length) == 0) {
		return TOPIC_PASTE_SUB;
	} else if (strncmp(topic_name, sub_topic_vdisk_data, topic_name_length) == 0) {
		return TOPIC_VDISK_SUB;
	}
	return TOPIC_UNKNOWN;
}

bool is_mqtt_connected(void) {
	return mqtt_connected;
}

#ifdef WOLFMQTT_DISCONNECT_CB
/* callback indicates a network error occurred */
static int mqtt_disconnect_cb(MqttClient* client, int error_code, void* ctx) {
	Log_Debug("Network Error Callback: %s (error %d)\n",
		MqttClient_ReturnCodeToString(error_code), error_code);

	if (error_code == MQTT_CODE_ERROR_NETWORK) {
		mqtt_connected = false;
		got_disconnected = true;
	}
	return 0;
}
#endif

static int mqtt_message_cb(MqttClient* client, MqttMessage* msg, byte msg_new, byte msg_done) {
	if (msg_new) {
		_publish_callback(msg);
	}
	return MQTT_CODE_SUCCESS;
}

static void client_cleanup(MQTTCtx* mqttCtx) {
	/* Cleanup network */
	MqttClientNet_DeInit(&mqttCtx->net);

	MqttClient_DeInit(&mqttCtx->client);
}

static void client_disconnect(MQTTCtx* mqttCtx) {
	int rc;
	mqtt_connected = false;

	do {
		/* Disconnect */
		rc = MqttClient_Disconnect_ex(&mqttCtx->client,
			&mqttCtx->disconnect);
	} while (rc == MQTT_CODE_CONTINUE);

	Log_Debug("MQTT Disconnect: %s (%d)\n",
		MqttClient_ReturnCodeToString(rc), rc);

	rc = MqttClient_NetDisconnect(&mqttCtx->client);

	Log_Debug("MQTT Socket Disconnect: %s (%d)",
		MqttClient_ReturnCodeToString(rc), rc);

	client_cleanup(mqttCtx);
}

static int init_mqtt_connection(MQTTCtx* mqttCtx) {
	int rc = MQTT_CODE_SUCCESS;

	Log_Debug("MQTT Client: QoS %d, Use TLS %d\n", mqttCtx->qos,
		mqttCtx->use_tls);

	/* Initialize Network */
	rc = MqttClientNet_Init(&mqttCtx->net, mqttCtx);

	Log_Debug("MQTT Net Init: %s (%d)\n", MqttClient_ReturnCodeToString(rc), rc);
	if (rc != MQTT_CODE_SUCCESS) {
		return rc;
	}

	/* setup tx/rx buffers */
	mqttCtx->tx_buf = tx_buf;
	mqttCtx->rx_buf = rx_buf;

	/* Initialize MqttClient structure */
	rc = MqttClient_Init(&mqttCtx->client, &mqttCtx->net,
		mqtt_message_cb,
		mqttCtx->tx_buf, MAX_BUFFER_SIZE,
		mqttCtx->rx_buf, MAX_BUFFER_SIZE,
		(int)mqttCtx->cmd_timeout_ms);

	Log_Debug("MQTT Init: %s (%d)\n", MqttClient_ReturnCodeToString(rc), rc);

	if (rc != MQTT_CODE_SUCCESS) {
		return rc;
	}
	/* The client.ctx will be stored in the cert callback ctx during
	   MqttSocket_Connect for use by mqtt_tls_verify_cb */
	mqttCtx->client.ctx = mqttCtx;

#ifdef WOLFMQTT_DISCONNECT_CB
	/* setup disconnect callback */
	rc = MqttClient_SetDisconnectCallback(&mqttCtx->client,
		mqtt_disconnect_cb, NULL);
	if (rc != MQTT_CODE_SUCCESS) {
		return rc;
	}
#endif

	/* Connect to broker */
	rc = MqttClient_NetConnect(&mqttCtx->client, mqttCtx->host, mqttCtx->port,
		DEFAULT_CON_TIMEOUT_MS, mqttCtx->use_tls, mqtt_tls_cb);

	Log_Debug("MQTT Socket Connect: %s (%d)\n", MqttClient_ReturnCodeToString(rc), rc);

	if (rc != MQTT_CODE_SUCCESS) {
		return rc;
	}

	/* Build connect packet */
	XMEMSET(&mqttCtx->connect, 0, sizeof(MqttConnect));
	mqttCtx->connect.keep_alive_sec = mqttCtx->keep_alive_sec;
	mqttCtx->connect.clean_session = mqttCtx->clean_session;
	mqttCtx->connect.client_id = mqttCtx->client_id;

	/* Optional authentication */
	mqttCtx->connect.username = mqttCtx->username;
	mqttCtx->connect.password = mqttCtx->password;

	/* Send Connect and wait for Connect Ack */
	do {
		rc = MqttClient_Connect(&mqttCtx->client, &mqttCtx->connect);
	} while (rc == MQTT_CODE_CONTINUE || rc == MQTT_CODE_STDIN_WAKE);

	Log_Debug("MQTT Connect: Proto (%s), %s (%d)\n",
		MqttClient_GetProtocolVersionString(&mqttCtx->client),
		MqttClient_ReturnCodeToString(rc), rc);

	if (rc != MQTT_CODE_SUCCESS) {
		client_disconnect(mqttCtx);
	}

	return rc;
}

/* this task subscribes to topic */
static void subscribe_task(MQTTCtx* mqttCtx) {
	int rc = MQTT_CODE_SUCCESS;

	XMEMSET(&mqttCtx->subscribe, 0, sizeof(MqttSubscribe));

	topics[0].topic_filter = sub_topic_data;
	topics[0].qos = mqttCtx->qos;

	topics[1].topic_filter = sub_topic_control;
	topics[1].qos = mqttCtx->qos;

	topics[2].topic_filter = sub_topic_paste;
	topics[2].qos = mqttCtx->qos;

	topics[3].topic_filter = sub_topic_vdisk_data;
	topics[3].qos = mqttCtx->qos;

#ifdef WOLFMQTT_V5
	if (mqttCtx->subId_not_avail != 1) {

		for (int i = 0; i < sizeof(topics) / sizeof(MqttTopic); i++) {
			/* Subscription Identifier */
			MqttProp* prop;
			topics[i].sub_id = i + 1; /* Sub ID starts at 1 */
			prop = MqttClient_PropsAdd(&mqttCtx->subscribe.props);
			prop->type = MQTT_PROP_SUBSCRIPTION_ID;
			prop->data_int = topics[i].sub_id;
		}
	}
#endif

	/* Subscribe Topic */
	mqttCtx->subscribe.packet_id = mqtt_get_packetid();
	mqttCtx->subscribe.topic_count = sizeof(topics) / sizeof(MqttTopic);
	mqttCtx->subscribe.topics = topics;

	rc = MqttClient_Subscribe(&mqttCtx->client, &mqttCtx->subscribe);

	mqtt_connected = rc == MQTT_CODE_SUCCESS;

#ifdef WOLFMQTT_V5
	if (mqttCtx->subscribe.props != NULL) {
		MqttClient_PropsFree(mqttCtx->subscribe.props);
	}
#endif
}

/* This task waits for messages */
static void* waitMessage_task(void* args) {
	int rc;
	MQTTCtx* mqttCtx = &gMqttCtx;
	int channel_id = -1;

	do {
		if (mqtt_connected) {
			rc = MqttClient_WaitMessage(&mqttCtx->client, (int)mqttCtx->cmd_timeout_ms);

			if (got_disconnected) {
				client_disconnect(&gMqttCtx);
			}
		} else {
			if (got_disconnected && ((channel_id = read_channel_id_from_storage()) != -1 || dt_channelId.propertyUpdated) && dx_isNetworkReady()) {
				// if channel id is -1 then channel id was not in storage
				if (channel_id == -1) {
					channel_id = *(int*)(dt_channelId.propertyValue);
				}

				memset(mqtt_client_id, 0, sizeof(mqtt_client_id));
				memset(pub_topic_data, 0, sizeof(pub_topic_data));
				memset(sub_topic_data, 0, sizeof(sub_topic_data));
				memset(sub_topic_control, 0, sizeof(sub_topic_control));
				memset(sub_topic_paste, 0, sizeof(sub_topic_paste));
				memset(sub_topic_vdisk_data, 0, sizeof(sub_topic_paste));
				memset(pub_topic_vdisk_read, 0, sizeof(pub_topic_vdisk_read));
				memset(pub_topic_vdisk_write, 0, sizeof(pub_topic_vdisk_write));

				snprintf(mqtt_client_id, sizeof(mqtt_client_id), MQTT_CLIENT_ID, channel_id);
				snprintf(pub_topic_data, sizeof(pub_topic_data), PUB_TOPIC_DATA, channel_id);
				snprintf(sub_topic_data, sizeof(sub_topic_data), SUB_TOPIC_DATA, channel_id);
				snprintf(sub_topic_control, sizeof(sub_topic_control), SUB_TOPIC_CONTROL, channel_id);
				snprintf(sub_topic_paste, sizeof(sub_topic_paste), SUB_TOPIC_PASTE, channel_id);
				snprintf(sub_topic_vdisk_data, sizeof(sub_topic_vdisk_data), SUB_TOPIC_VDISK_RESPONSE, channel_id);
				snprintf(pub_topic_vdisk_read, sizeof(pub_topic_vdisk_read), PUB_TOPIC_VDISK_READ, channel_id);
				snprintf(pub_topic_vdisk_write, sizeof(pub_topic_vdisk_write), PUB_TOPIC_VDISK_WRITE, channel_id);

				mqttCtx->topic_name = pub_topic_data;
				mqttCtx->client_id = mqtt_client_id;

				rc = init_mqtt_connection(&gMqttCtx);

				if (rc == MQTT_CODE_SUCCESS) {
					got_disconnected = false;
					subscribe_task(&gMqttCtx);
					_mqtt_connected_cb();
				} else {
					Log_Debug("Retry network connect error : %s (error %d)\n",
						MqttClient_ReturnCodeToString(rc), rc);
				}

				if (got_disconnected) {
					client_disconnect(&gMqttCtx);
				}
			}
			if (!mqtt_connected) {
				nanosleep(&(struct timespec) { 2, 0 }, NULL);
			}
		}
	} while (true);

	return NULL;
}

void publish_message(const char* message, size_t message_length) {
	if (console_fd != -1 && local_serial) {
		write(console_fd, message, message_length);
	}

#ifdef ENABLE_WEB_TERMINAL
	if (!mqtt_connected) { return; }

	MQTTCtx* mqttCtx = &gMqttCtx;
	MqttPublish publish;

	/* Publish Topic */
	XMEMSET(&publish, 0, sizeof(MqttPublish));
	publish.retain = 0;
	publish.qos = 0;
	publish.duplicate = 0;
	publish.topic_name = mqttCtx->topic_name;
	publish.packet_id = mqtt_get_packetid();

	publish.buffer = (byte*)message;
	publish.total_len = message_length;

	MqttClient_Publish(&mqttCtx->client, &publish);

#endif // ENABLE_WEB_TERMINAL
}



void vdisk_mqtt_read_sector(uint32_t offset) {
	if (!mqtt_connected) { return; }

	MQTTCtx* mqttCtx = &gMqttCtx;
	MqttPublish publish;

	/* Publish Topic */
	XMEMSET(&publish, 0, sizeof(MqttPublish));
	publish.retain = 0;
	publish.qos = 0;
	publish.duplicate = 0;
	publish.topic_name = pub_topic_vdisk_read;
	publish.packet_id = mqtt_get_packetid();

	publish.buffer = (byte*)&offset;
	publish.total_len = sizeof(offset);

	MqttClient_Publish(&mqttCtx->client, &publish);
}

void vdisk_mqtt_write_sector(vdisk_mqtt_write_sector_t* write_sector) {
	if (!mqtt_connected) { return; }

	MQTTCtx* mqttCtx = &gMqttCtx;
	MqttPublish publish;

	/* Publish Topic */
	XMEMSET(&publish, 0, sizeof(MqttPublish));
	publish.retain = 0;
	publish.qos = mqttCtx->qos;
	publish.duplicate = 0;
	publish.topic_name = pub_topic_vdisk_write;
	publish.packet_id = mqtt_get_packetid();

	publish.buffer = (byte*)write_sector;
	publish.total_len = sizeof(vdisk_mqtt_write_sector_t);

	MqttClient_Publish(&mqttCtx->client, &publish);
}

//static int unsubscribe_do(MQTTCtx* mqttCtx) {
//	int rc;
//
//	/* Unsubscribe Topics */
//	XMEMSET(&mqttCtx->unsubscribe, 0, sizeof(MqttUnsubscribe));
//	mqttCtx->unsubscribe.packet_id = mqtt_get_packetid();
//
//	mqttCtx->unsubscribe.topic_count = sizeof(mqttCtx->topics) / sizeof(MqttTopic);
//	mqttCtx->unsubscribe.topics = mqttCtx->topics;
//
//	/* Unsubscribe Topics */
//	rc = MqttClient_Unsubscribe(&mqttCtx->client, &mqttCtx->unsubscribe);
//
//	Log_Debug("MQTT Unsubscribe: %s (%d)\n", MqttClient_ReturnCodeToString(rc), rc);
//
//	return rc;
//}

/**
 * init MQTT connection and subscribe desired topics
 */
void init_mqtt(void (*publish_callback)(MqttMessage* msg), void (*mqtt_connected_cb)(void)) {
	_publish_callback = publish_callback;
	_mqtt_connected_cb = mqtt_connected_cb;

	mqtt_init_ctx(&gMqttCtx);
	gMqttCtx.app_name = "Altair on Azure Sphere";

#ifdef ENABLE_WEB_TERMINAL
	dx_startThreadDetached(waitMessage_task, NULL, "wait for message");
	dx_timerStart(&ping_timer);
#else
	mqtt_connected_cb();
#endif // ENABLE_WEB_TERMINAL
}

void queue_mqtt_message(const uint8_t* data, size_t data_length) {
	publish_message(data, data_length);
}

void send_partial_message(void) {
	if (output_buffer_length > 0) {
		publish_message(output_buffer, output_buffer_length);
		output_buffer_length = 0;
	}
	dirty_buffer = false;
}

void publish_character(char character) {
	dirty_buffer = true;

	memcpy(output_buffer + output_buffer_length, &character, 1);
	output_buffer_length++;

	if (output_buffer_length >= sizeof(output_buffer)) {
		publish_message(output_buffer, output_buffer_length);
		output_buffer_length = 0;
		dirty_buffer = false;
	}
}