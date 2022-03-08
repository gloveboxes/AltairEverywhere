#include "mqtt_manager.h"

void publish_callback(void **unused, struct mqtt_response_publish *published);
void *client_refresher(void *client);

uint8_t sendbuf[2048]; /* sendbuf should be large enough to hold multiple whole mqtt messages */
uint8_t recvbuf[1024]; /* recvbuf should be large enough any whole mqtt message expected to be received */
int sockfd = -1;
const char *addr;
const char *port;
static struct mqtt_client client;
static int reconnect_count = 0;

static char output_buffer[512];
static size_t output_buffer_length = 0;

static const char *publish_topic = "altair/8800/web";

static const char *subscription_topics[] = {"altair/8800/dev", "altair/8800/dev/ctrl"};

typedef struct {
    const char *hostname;
    const char *port;
    void (*mqtt_connected_cb)(void);
} RECONNECT_STATE_T;

RECONNECT_STATE_T reconnect_state;

static int open_nb_socket(const char* addr, const char* port) {
    struct addrinfo hints = {0};

    hints.ai_family = AF_UNSPEC; /* IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Must be TCP */
    int sockfd = -1;
    int rv;
    struct addrinfo *p, *servinfo;

    /* get address information */
    rv = getaddrinfo(addr, port, &hints, &servinfo);
    if(rv != 0) {
        fprintf(stderr, "Failed to open socket (getaddrinfo): %s\n", gai_strerror(rv));
        return -1;
    }

    /* open the first possible socket */
    for(p = servinfo; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) continue;

        /* connect to server */
        rv = connect(sockfd, p->ai_addr, p->ai_addrlen);
        if(rv == -1) {
          close(sockfd);
          sockfd = -1;
          continue;
        }
        break;
    }

    /* free servinfo */
    freeaddrinfo(servinfo);

    /* make non-blocking */

    if (sockfd != -1) fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) | O_NONBLOCK);

    /* return the new socket fd */
    return sockfd;
}

TOPIC_TYPE topic_type(const void *topic_name, uint16_t topic_name_length)
{
    int number_of_topics = NELEMS(subscription_topics);

    for (int i = 0; i < number_of_topics; i++) {
        if (strncmp(subscription_topics[i], topic_name, topic_name_length) == 0) {
            return (TOPIC_TYPE)i + 1;
        }
    }
    return TOPIC_UNKNOWN;
}

void reconnect(struct mqtt_client *client, void **reconnect_state_vptr)
{
    RECONNECT_STATE_T *reconnect_state = *((RECONNECT_STATE_T **)reconnect_state_vptr);

    /* Close the clients socket if this isn't the initial reconnect call */
    if (client->error != MQTT_ERROR_INITIAL_RECONNECT) {
        close(client->socketfd);
    }

    /* Perform error handling here. */
    if (client->error != MQTT_ERROR_INITIAL_RECONNECT) {
        printf("reconnect_client: called while client was in error state \"%s\"\n", mqtt_error_str(client->error));
        reconnect_count++;
    }

    sockfd = open_nb_socket(reconnect_state->hostname, reconnect_state->port);

    mqtt_reinit(client, sockfd, sendbuf, sizeof(sendbuf), recvbuf, sizeof(recvbuf));

    const char *client_id = NULL;                       // Create an anonymous session
    uint8_t connect_flags = MQTT_CONNECT_CLEAN_SESSION; // Ensure we have a clean session
    mqtt_connect(client, client_id, NULL, NULL, 0, NULL, NULL, connect_flags, 400);

    // Subscribe to topics
    if (client->socketfd != -1) {
        for (int i = 0; i < NELEMS(subscription_topics); i++) {
            mqtt_subscribe(client, subscription_topics[i], 0);
        }

        reconnect_state->mqtt_connected_cb();
    }
}

void publish_message(const void *application_message, size_t application_message_length)
{
    int retry_count = 0;
    int queue_len = 0;

    if (client.error != MQTT_OK) {
        return;
    }
    mqtt_publish(&client, publish_topic, application_message, application_message_length, MQTT_PUBLISH_QOS_0);

    retry_count = 0;
    do {
        // https://githubhot.com/repo/LiamBindle/MQTT-C/issues/151
        mqtt_sync(&client);
        mqtt_mq_clean(&client.mq);

        if ((queue_len = mqtt_mq_length(&client.mq)) > 0) {
            usleep(1000);
            printf("MQTT-C queue length: %d\n", mqtt_mq_length(&client.mq));
        }

    } while (queue_len > 0 && retry_count++ < 50);
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

    if (output_buffer_length < sizeof(output_buffer)) {
        return;
    }

    publish_message(output_buffer, output_buffer_length);
    output_buffer_length = 0;
    dirty_buffer = false;
}

void init_mqtt(void (*publish_callback)(void **unused, struct mqtt_response_publish *published), void (*mqtt_connected_cb)(void))
{
    reconnect_state.hostname = MQTT_BROKER_URL;
    reconnect_state.port = MQTT_BROKER_PORT;
    reconnect_state.mqtt_connected_cb = mqtt_connected_cb;

    mqtt_init_reconnect(&client, reconnect, &reconnect_state, publish_callback);

    mqtt_sync(&client);

    pthread_t client_daemon;
    if (pthread_create(&client_daemon, NULL, client_refresher, &client))
    {
        fprintf(stderr, "Failed to start client daemon.\n");
    }
}

void *client_refresher(void *client)
{
    while (1)
    {
        mqtt_sync((struct mqtt_client *)client);
        usleep(100000U);
    }
    return NULL;
}