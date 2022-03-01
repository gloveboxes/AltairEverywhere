/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "comms_manager_wolf.h"

#define MAX_BUFFER_SIZE 4096

static bool mqtt_connected = false;
static bool got_disconnected = true;
static const char* _networkInterface = NULL;

MqttTopic topics[3];

static char mqtt_client_id[30] = {0};
static char pub_topic_data[30] = {0};
static char sub_topic_data[30] = {0};
static char sub_topic_control[30] = {0};
static char sub_topic_paste[30] = {0};
// static char sub_topic_vdisk_data[30] = {0};
// static char pub_topic_vdisk_read[30] = {0};
// static char pub_topic_vdisk_write[30] = {0};

static void (*_publish_callback)(MqttMessage *msg);
static void (*_mqtt_connected_cb)(void);

static byte tx_buf[MAX_BUFFER_SIZE];
static byte rx_buf[MAX_BUFFER_SIZE];

static char output_buffer[MAX_BUFFER_SIZE / 4];
static size_t output_buffer_length = 0;

static wm_Sem mtLock; /* Protect "packetId" and "stop" */
static MQTTCtx gMqttCtx;
static pthread_mutex_t queue_lock;

/* locals */
static word16 mPacketIdLast;

/* argument parsing */
// static int myoptind = 0;
// static char* myoptarg = NULL;

TOPIC_TYPE topic_type(char *topic_name, size_t topic_name_length)
{
    if (strncmp(topic_name, sub_topic_data, topic_name_length) == 0) {
        return TOPIC_DATA_SUB;
    } else if (strncmp(topic_name, sub_topic_control, topic_name_length) == 0) {
        return TOPIC_CONTROL_SUB;
    } else if (strncmp(topic_name, sub_topic_paste, topic_name_length) == 0) {
        return TOPIC_PASTE_SUB;
    }
    return TOPIC_UNKNOWN;
}

static word16 mqtt_get_packetid_threadsafe(void)
{
    word16 packet_id;
    wm_SemLock(&mtLock);
    packet_id = mqtt_get_packetid();
    wm_SemUnlock(&mtLock);
    return packet_id;
}


static int check_response(MQTTCtx *mqttCtx, int rc, word32 *startSec, int packet_type)
{

#ifdef WOLFMQTT_NONBLOCK
#ifdef WOLFMQTT_TEST_CANCEL
    if (packet_type == MQTT_PACKET_TYPE_PUBLISH && rc == MQTT_CODE_CONTINUE) {
        PRINTF("Test cancel by setting early timeout");
        return MQTT_CODE_ERROR_TIMEOUT;
    } else
#else
    (void)packet_type;
#endif
        /* Track elapsed time with no activity and trigger timeout */
        rc = mqtt_check_timeout(rc, startSec, mqttCtx->cmd_timeout_ms / 1000);

    /* check return code */
    if (rc == MQTT_CODE_CONTINUE) {
#if 0
        /* optionally add delay when debugging */
        usleep(100*1000);
#endif
    }
#else
    (void)packet_type;
    (void)startSec;
    (void)mqttCtx;
#endif
    return rc;
}

bool is_mqtt_connected(void)
{
    return mqtt_connected;
}

#ifdef WOLFMQTT_DISCONNECT_CB
/* callback indicates a network error occurred */
static int mqtt_disconnect_cb(MqttClient *client, int error_code, void *ctx)
{
    if (error_code == MQTT_CODE_ERROR_NETWORK) {
        Log_Debug("Network Error Callback: %s (error %d)\n", MqttClient_ReturnCodeToString(error_code), error_code);

        mqtt_connected = false;
        got_disconnected = true;
    }

    return 0;
}
#endif

static int mqtt_message_cb(MqttClient *client, MqttMessage *msg, byte msg_new, byte msg_done)
{
    wm_SemLock(&mtLock);
    if (msg_new) {
        _publish_callback(msg);
    }
    wm_SemUnlock(&mtLock);
    return MQTT_CODE_SUCCESS;
}

static void client_cleanup(MQTTCtx *mqttCtx)
{
    /* Cleanup network */
    MqttClientNet_DeInit(&mqttCtx->net);
    MqttClient_DeInit(&mqttCtx->client);
}

static void client_disconnect(MQTTCtx *mqttCtx)
{
    int rc;
    mqtt_connected = false;

    do {
        /* Disconnect */
        rc = MqttClient_Disconnect_ex(&mqttCtx->client, &mqttCtx->disconnect);
    } while (rc == MQTT_CODE_CONTINUE);

    Log_Debug("MQTT Disconnect: %s (%d)\n", MqttClient_ReturnCodeToString(rc), rc);

    rc = MqttClient_NetDisconnect(&mqttCtx->client);

    Log_Debug("MQTT Socket Disconnect: %s (%d)", MqttClient_ReturnCodeToString(rc), rc);

    client_cleanup(mqttCtx);
}

static int init_mqtt_connection(MQTTCtx *mqttCtx)
{
    int rc = MQTT_CODE_SUCCESS;
    word32 startSec;

    Log_Debug("MQTT Client: QoS %d, Use TLS %d\n", mqttCtx->qos, mqttCtx->use_tls);

    /* Create a demo mutex for making packet id values */
    rc = wm_SemInit(&mtLock);
    if (rc != 0) {
        return 0;
    }

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
    rc = MqttClient_Init(&mqttCtx->client, &mqttCtx->net, mqtt_message_cb, mqttCtx->tx_buf, MAX_BUFFER_SIZE, mqttCtx->rx_buf,
                         MAX_BUFFER_SIZE, (int)mqttCtx->cmd_timeout_ms);

    Log_Debug("MQTT Init: %s (%d)\n", MqttClient_ReturnCodeToString(rc), rc);

    if (rc != MQTT_CODE_SUCCESS) {
        return rc;
    }

    #ifdef WOLFMQTT_NONBLOCK
    mqttCtx->useNonBlockMode = 1;
    #endif


    /* The client.ctx will be stored in the cert callback ctx during
       MqttSocket_Connect for use by mqtt_tls_verify_cb */
    mqttCtx->client.ctx = mqttCtx;

#ifdef WOLFMQTT_DISCONNECT_CB
    /* setup disconnect callback */
    rc = MqttClient_SetDisconnectCallback(&mqttCtx->client, mqtt_disconnect_cb, NULL);
    if (rc != MQTT_CODE_SUCCESS) {
        return rc;
    }
#endif

    /* Connect to broker */
    startSec = 0;
    do {
        rc = MqttClient_NetConnect(&mqttCtx->client, mqttCtx->host, mqttCtx->port, DEFAULT_CON_TIMEOUT_MS, mqttCtx->use_tls, mqtt_tls_cb);
        rc = check_response(mqttCtx, rc, &startSec, MQTT_PACKET_TYPE_CONNECT);
    } while (rc == MQTT_CODE_CONTINUE);

    /* Connect to broker */
    // rc = MqttClient_NetConnect(&mqttCtx->client, mqttCtx->host, mqttCtx->port, DEFAULT_CON_TIMEOUT_MS, mqttCtx->use_tls, mqtt_tls_cb);

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
    // mqttCtx->connect.username = mqttCtx->username;
    // mqttCtx->connect.password = mqttCtx->password;

    /* Send Connect and wait for Connect Ack */

    startSec = 0;
    do {
        rc = MqttClient_Connect(&mqttCtx->client, &mqttCtx->connect);
        rc = check_response(mqttCtx, rc, &startSec, MQTT_PACKET_TYPE_CONNECT);
    } while (rc == MQTT_CODE_CONTINUE);

    // do {
    //     rc = MqttClient_Connect(&mqttCtx->client, &mqttCtx->connect);
    // } while (rc == MQTT_CODE_CONTINUE || rc == MQTT_CODE_STDIN_WAKE);

    Log_Debug("MQTT Connect: Proto (%s), %s (%d)\n", MqttClient_GetProtocolVersionString(&mqttCtx->client),
              MqttClient_ReturnCodeToString(rc), rc);

    if (rc != MQTT_CODE_SUCCESS) {
        client_disconnect(mqttCtx);
    }

    return rc;
}

/* this task subscribes to topic */
static void subscribe_task(MQTTCtx *mqttCtx)
{
    int rc = MQTT_CODE_SUCCESS;
    word32 startSec = 0;

    XMEMSET(&mqttCtx->subscribe, 0, sizeof(MqttSubscribe));

    topics[0].topic_filter = sub_topic_data;
    topics[0].qos = mqttCtx->qos;

    topics[1].topic_filter = sub_topic_control;
    topics[1].qos = mqttCtx->qos;

    topics[2].topic_filter = sub_topic_paste;
    topics[2].qos = mqttCtx->qos;

#ifdef WOLFMQTT_V5
    if (mqttCtx->subId_not_avail != 1) {

        for (int i = 0; i < sizeof(topics) / sizeof(MqttTopic); i++) {
            /* Subscription Identifier */
            MqttProp *prop;
            topics[i].sub_id = i + 1; /* Sub ID starts at 1 */
            prop = MqttClient_PropsAdd(&mqttCtx->subscribe.props);
            prop->type = MQTT_PROP_SUBSCRIPTION_ID;
            prop->data_int = topics[i].sub_id;
        }
    }
#endif

    dx_Log_Debug("Subscribing\n");

    /* Subscribe Topic */
    mqttCtx->subscribe.packet_id = mqtt_get_packetid_threadsafe();
    mqttCtx->subscribe.topic_count = sizeof(topics) / sizeof(MqttTopic);
    mqttCtx->subscribe.topics = topics;

    do {
        rc = MqttClient_Subscribe(&mqttCtx->client, &mqttCtx->subscribe);
        rc = check_response(mqttCtx, rc, &startSec, MQTT_PACKET_TYPE_SUBSCRIBE);
    } while (rc == MQTT_CODE_CONTINUE);

    dx_Log_Debug("Subscribe return code: %d\n", rc);

    mqtt_connected = rc == MQTT_CODE_SUCCESS;

#ifdef WOLFMQTT_V5
    if (mqttCtx->subscribe.props != NULL) {
        MqttClient_PropsFree(mqttCtx->subscribe.props);
    }
#endif
}

static bool send_ping_new(void)
{
    int rc;
    MqttPing ping;
    word32 startSec;

    wm_SemLock(&mtLock);

    startSec = 0;
    XMEMSET(&ping, 0, sizeof(ping));
    do {
        rc = MqttClient_Ping_ex(&gMqttCtx.client, &ping);
        rc = check_response(&gMqttCtx, rc, &startSec, MQTT_PACKET_TYPE_PING_REQ);
    } while (rc == MQTT_CODE_CONTINUE);
    if (rc != MQTT_CODE_SUCCESS) {
        MqttClient_CancelMessage(&gMqttCtx.client, (MqttObject *)&ping);
    }
    wm_SemUnlock(&mtLock);

    if (rc != MQTT_CODE_SUCCESS) {
        PRINTF("MQTT Ping Keep Alive Error: %s (%d)", MqttClient_ReturnCodeToString(rc), rc);
    }

    return rc == MQTT_CODE_SUCCESS;
}

/* This task waits for messages */
static void *waitMessage_task(void *args)
{
    int rc;
    MQTTCtx *mqttCtx = &gMqttCtx;
    int channel_id = -1;
    word32 cmd_timeout_ms = mqttCtx->cmd_timeout_ms;

    do {
        if (mqtt_connected) {
            rc = MqttClient_WaitMessage(&mqttCtx->client, cmd_timeout_ms);

            if (got_disconnected) {
                client_disconnect(&gMqttCtx);
            }

            if (rc == MQTT_CODE_ERROR_TIMEOUT) {
                if (!send_ping_new()){
                    dx_Log_Debug("Ping failed\n");
                    // break;
                }              
            }

        } else {
            if (got_disconnected && ((channel_id = read_channel_id_from_storage()) != -1)) {

                memset(mqtt_client_id, 0, sizeof(mqtt_client_id));
                memset(pub_topic_data, 0, sizeof(pub_topic_data));
                memset(sub_topic_data, 0, sizeof(sub_topic_data));
                memset(sub_topic_control, 0, sizeof(sub_topic_control));
                memset(sub_topic_paste, 0, sizeof(sub_topic_paste));

                snprintf(mqtt_client_id, sizeof(mqtt_client_id), MQTT_CLIENT_ID, channel_id);
                snprintf(pub_topic_data, sizeof(pub_topic_data), PUB_TOPIC_DATA, channel_id);
                snprintf(sub_topic_data, sizeof(sub_topic_data), SUB_TOPIC_DATA, channel_id);
                snprintf(sub_topic_control, sizeof(sub_topic_control), SUB_TOPIC_CONTROL, channel_id);
                snprintf(sub_topic_paste, sizeof(sub_topic_paste), SUB_TOPIC_PASTE, channel_id);

                mqttCtx->topic_name = pub_topic_data;
                mqttCtx->client_id = mqtt_client_id;

                dx_Log_Debug("Connecting to MQTT broker\n");
                rc = init_mqtt_connection(&gMqttCtx);

                if (rc == MQTT_CODE_SUCCESS) {
                    got_disconnected = false;
                    subscribe_task(&gMqttCtx);
                    _mqtt_connected_cb();
                } else {
                    Log_Debug("Retry network connect error : %s (error %d)\n", MqttClient_ReturnCodeToString(rc), rc);
                }

                if (got_disconnected && rc != MQTT_CODE_ERROR_NETWORK) {
                    client_disconnect(&gMqttCtx);
                }
            }
            if (!mqtt_connected) {
                dx_Log_Debug("Broker not connected\n");
                nanosleep(&(struct timespec){2, 0}, NULL);
            }
        }
    } while (true);

    return NULL;
}

void publish_message(const char *message, size_t message_length)
{
    if (!mqtt_connected) {
        return;
    }

    MQTTCtx *mqttCtx = &gMqttCtx;
    MqttPublish publish;

    /* Publish Topic */
    XMEMSET(&publish, 0, sizeof(MqttPublish));
    publish.retain = 0;
    publish.qos = 0;
    publish.duplicate = 0;
    publish.topic_name = mqttCtx->topic_name;
    publish.packet_id = mqtt_get_packetid_threadsafe();

    publish.buffer = (byte *)message;
    publish.total_len = message_length;

    MqttClient_Publish(&mqttCtx->client, &publish);
}

/**
 * init MQTT connection and subscribe desired topics
 */
int init_mqtt(int argc, char *argv[], void (*publish_callback)(MqttMessage *msg), void (*mqtt_connected_cb)(void), const char* networkInterface)
{
    int rc;

    _publish_callback = publish_callback;
    _mqtt_connected_cb = mqtt_connected_cb;
    _networkInterface = networkInterface;

    mqtt_init_ctx(&gMqttCtx);

    gMqttCtx.app_name = "Altair emulator";

    gMqttCtx.host = ALTAIR_MQTT_BROKER;
    gMqttCtx.port = ALTAIR_MQTT_BROKER_PORT;
    gMqttCtx.client_id = ALTAIR_MQTT_IDENTITY;

    // Mutex object to guard output_queue object
    if (pthread_mutex_init(&queue_lock, NULL) != 0) {
        Log_Debug("pthread_mutex_init() error");
    }

    dx_startThreadDetached(waitMessage_task, NULL, "wait for message");

    return 0;
}

void send_partial_message(void)
{
    if (output_buffer_length > 0) {
        publish_message(output_buffer, output_buffer_length);
        output_buffer_length = 0;
    }
    dirty_buffer = false;
}

void publish_character(char character)
{
    dirty_buffer = true;

    memcpy(output_buffer + output_buffer_length, &character, 1);
    output_buffer_length++;

    if (output_buffer_length >= sizeof(output_buffer)) {
        publish_message(output_buffer, output_buffer_length);
        output_buffer_length = 0;
        dirty_buffer = false;
    }
}