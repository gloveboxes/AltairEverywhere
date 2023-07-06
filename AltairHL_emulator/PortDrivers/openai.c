/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "openai.h"
#include "dx_utilities.h"
#include "parson.h"
#include <curl/curl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#ifdef AZURE_SPHERE
#include <applibs/storage.h>
#endif // AZURE_SPHERE

static const char system_message[] = "You are an intelligent assistant";
static const char user_message[]   = "What is the meaning of life in 5 short bullet points followed by a summary";
static const int max_tokens        = 128;

// "write Microsoft BASIC-80 Rev. 5.21 syntax code with line numbers and single letter variable names. Just write code no explanation",
// "Calculate prime numbers from 0 to 100", 256);

static int stream_openai(struct curl_slist *headers, const char *postData, long timeout);
static void *openai_thread(void *arg);

static const char *openai_endpoint = "https://api.openai.com/v1/chat/completions";
static char *openai_prompt         = NULL;
static struct curl_slist *headers  = NULL;
static bool streaming              = false;

enum OPENAI_STATUS
{
    OPENAI_END_OF_STREAM,
    OPENAI_WAITING,
    OPENAI_DATA_READY,
    // OPENAI_FAILED
};

typedef struct
{
    // bool end_of_stream;
    char content[128];
    int content_length;
    int content_index;
    char last_finish_reason[10];
    char system_message[1024];
    enum OPENAI_STATUS status;
} OPENAI_T;

static OPENAI_T openai;

pthread_mutex_t openai_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_openai(const char *openai_api_key)
{
    if (!headers)
    {
        char auth[128];
        snprintf(auth, sizeof(auth), "Authorization: Bearer %s", openai_api_key);

        headers = curl_slist_append(headers, auth);
        headers = curl_slist_append(headers, "Content-Type: application/json");
    }
}

char *create_prompt(const char *system_message, const char *user_message, int max_tokens)
{
    JSON_Value *root_value = NULL;
    // create a json object of the post data using parson
    root_value               = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    json_object_set_string(root_object, "model", "gpt-3.5-turbo");
    json_object_set_number(root_object, "temperature", 0.5);
    json_object_set_number(root_object, "max_tokens", max_tokens);
    json_object_set_number(root_object, "top_p", 0.5);
    json_object_set_number(root_object, "n", 1);
    json_object_set_boolean(root_object, "stream", true);

    // create a messages array
    JSON_Value *messages_value = json_value_init_array();
    JSON_Array *messages_array = json_value_get_array(messages_value);
    json_object_set_value(root_object, "messages", messages_value);
    // create a message object
    JSON_Value *message_value   = json_value_init_object();
    JSON_Object *message_object = json_value_get_object(message_value);
    json_object_set_string(message_object, "role", "system");
    json_object_set_string(message_object, "content", system_message);
    json_array_append_value(messages_array, message_value);
    // create a message object
    message_value  = json_value_init_object();
    message_object = json_value_get_object(message_value);
    json_object_set_string(message_object, "role", "user");
    json_object_set_string(message_object, "content", user_message);
    json_array_append_value(messages_array, message_value);
    message_value = json_value_init_object();
    // message_object = json_value_get_object(message_value);
    // json_object_set_string(message_object, "role", "user");
    // json_object_set_string(message_object, "content", "then translate to tagalog");
    // json_array_append_value(messages_array, message_value);

    char *jsonString = json_serialize_to_string(root_value);
    json_value_free(root_value);

    return jsonString;
}

size_t openai_output(int port, uint8_t data, char *buffer, size_t buffer_length)
{
    size_t len = 0;
    switch (port)
    {

        case 120: // Set system message
            // if (openai.index == 0)
            // {
            //     memset(openai.system_message, 0x00, ENDPOINT_LEN);
            // }

            // if (data != 0 && openai.index < ENDPOINT_LEN)
            // {
            //     openai.system_message[openai.index++] = data;
            // }

            // if (data == 0) // NULL TERMINATION
            // {
            //     openai.system_message[openai.index] = 0x00;
            //     openai.index                        = 0;
            // }
            break;
        case 121: // Set user Message

            if (openai_prompt == NULL)
            {
                openai_prompt = create_prompt(system_message, user_message, max_tokens);
            }
            break;

        case 122: // Set assistant message

            break;
        case 123: // Clear all messages

            break;
        case 124: // Load OpenAI stream
            if (headers)
            {
                openai.status = OPENAI_WAITING;
                dx_startThreadDetached(openai_thread, NULL, "OpenAI Thread");
            }

            break;

        case 125: // Cancel ChatGPT stream

            break;
    }

    return len;
}

uint8_t openai_input(uint8_t port)
{
    uint8_t retVal = 0;

    switch (port)
    {
        case 120: // get streaming status
            retVal = openai.status;
            break;
        case 121: // get message
            if (openai.content_length > 0)
            {
                retVal = openai.content[openai.content_index++];
                openai.content_length--;

                if (openai.content_length == 0)
                {
                    openai.status = OPENAI_WAITING;
                    pthread_mutex_unlock(&openai_mutex);
                }
            }
            else
            {
                retVal = 0x00;
                pthread_mutex_unlock(&openai_mutex);
            }
            break;
    }

    return retVal;
}

#ifdef __APPLE__

/*
 * A pthread_mutex_timedlock() impl for OSX/macOS, which lacks the real thing.
 * NOTE: Unlike the real McCoy, won't return EOWNERDEAD, EDEADLK, or EOWNERDEAD
 */
static int pthread_mutex_timedlock(pthread_mutex_t *mutex, const struct timespec *abs_timeout)
{
    int rv;
    struct timespec remaining, slept, ts;
    remaining = *abs_timeout;

    long nsecs = remaining.tv_sec * 1000 * ONE_MS;
    nsecs += remaining.tv_nsec;
    printf("\n");

    while ((rv = pthread_mutex_trylock(mutex)) == EBUSY)
    {
        printf(".");
        if (nsecs > 10 * ONE_MS)
        {
            nanosleep(&(struct timespec){0, 10 * ONE_MS}, NULL);
            nsecs -= 10 * ONE_MS;
        }
        else
        {
            nanosleep(&(struct timespec){0, nsecs}, NULL);
            nsecs = 0;
        }

        if (nsecs <= 0)
        {
            return ETIMEDOUT;
        }
    }

    return rv;
}

#endif // __APPLE__

/// @brief callback for OpenAI Streaming API
/// @param contents
/// @param size
/// @param nmemb
/// @param userp
/// @return
static size_t StreamOpenAICallback(void *contents, size_t size, size_t nmemb, void *openai)
{
    // Packet shape for OpenAI streaming API
    // {
    //     "id": "cmpl-7Y2XIEtgA7MkTlLhKD3kZ7LSHA6ms",
    //     "object": "text_completion",
    //     "created": 1688347256,
    //     "choices": [
    //         {
    //             "text": "Building",
    //             "index": 0,
    //             "logprobs": null,
    //             "finish_reason": null
    //         }
    //     ],
    //     "model": "text-davinci-003"
    // }

    JSON_Array *choices    = NULL;
    JSON_Object *choice    = NULL;
    JSON_Object *messages  = NULL;
    JSON_Value *root_value = NULL;
    const char *content    = NULL;

    size_t realsize = size * nmemb;
    OPENAI_T *chat  = (OPENAI_T *)openai;

    struct timespec timeoutTime;
    memset(&timeoutTime, 0, sizeof(struct timespec));

#ifndef __APPLE__

    clock_gettime(CLOCK_REALTIME, &timeoutTime);
    timeoutTime.tv_sec += 3;
    timeoutTime.tv_nsec = 0;

#else // __APPLE__

    // Max wait is 500ms
    timeoutTime.tv_sec  = 0;
    timeoutTime.tv_nsec = 500 * ONE_MS;

#endif // __APPLE__

        if (pthread_mutex_timedlock(&openai_mutex, &timeoutTime) != 0)
    {
        chat->status = OPENAI_END_OF_STREAM;
        // setting this to -1 will cause the curl request to fail and end
        realsize = -1;
        goto cleanup;
    }

    chat->content_index  = 0;
    chat->content_length = 0;
    char *ptr            = (char *)contents;

    while (ptr != NULL)
    {
        ptr = strstr(ptr, "data: ");
        if (ptr != NULL)
        {
            ptr += 6;

            root_value = json_parse_string(ptr);
            if (root_value == NULL)
            {
                // printf("StreamOpenAICallback: json_parse_string failed\n");
                goto cleanup;
            }

            JSON_Object *root_object = json_value_get_object(root_value);
            if (root_object == NULL)
            {
                // printf("StreamOpenAICallback: json_parse_string failed\n");
                goto cleanup;
            }

            choices = json_object_get_array(root_object, "choices");
            if (choices == NULL)
            {
                // printf("StreamOpenAICallback: json_object_get_array failed\n");
                goto cleanup;
            }

            JSON_Object *choice = json_array_get_object(choices, 0);
            if (choice == NULL)
            {
                goto cleanup;
            }

            const char *finish_reason = json_object_get_string(choice, "finish_reason");

            if (!finish_reason)
            {
                JSON_Object *choice = json_array_get_object(choices, 0);

                if ((messages = json_object_get_object(choice, "delta")) != NULL)
                {
                    content = json_object_get_string(messages, "content");
                }
            }
            else
            {
                content = NULL;
                strcpy(chat->last_finish_reason, finish_reason);
            }

            if (content != NULL)
            {
                int content_length = strlen(content);
                if (content_length > 0 && chat->content_length + content_length < sizeof(chat->content))
                {
                    memcpy(chat->content + chat->content_length, content, content_length);
                    chat->content_length += content_length;
                }
            }

        cleanup:

            if (root_value != NULL)
            {
                json_value_free(root_value);
                root_value = NULL;
            }
        }
    }

    chat->status = OPENAI_DATA_READY;

    return realsize;
}

static int stream_openai(struct curl_slist *headers, const char *postData, long timeout)
{
    CURL *curl_handle;

    strcpy(openai.last_finish_reason, "failed");

    /* init the curl session */
    curl_handle = curl_easy_init();
    if (curl_handle)
    {
        curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);

        /* set URL to get here */
        curl_easy_setopt(curl_handle, CURLOPT_URL, openai_endpoint);

        /* use a CURLOPT_POSTFIELDS to fetch data */
        curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, postData);

        // https://curl.se/libcurl/c/CURLOPT_TIMEOUT.html
        curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 40L);

        /* send all data to this function  */
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, StreamOpenAICallback);

        /* we pass our 'chunk' struct to the callback function */
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&openai);

        /* some servers do not like requests that are made without a user-agent
           field, so we provide one */
        curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

        // based on the libcurl sample - https://curl.se/libcurl/c/https.html
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);

        // https://curl.se/libcurl/c/CURLOPT_NOSIGNAL.html
        curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1L);

        /* Switch on full protocol/debug output while testing */
        // curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);

        /* disable progress meter, set to 0L to enable it */
        curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);

        // A long parameter set to 1 tells the library to fail the request if the HTTP code returned is equal to
        // or larger than 400
        curl_easy_setopt(curl_handle, CURLOPT_FAILONERROR, true);

        openai.status = OPENAI_WAITING;
        // openai.end_of_stream = false;

        /* write the page body to this file handle */
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &openai);

        /* get it! */
        CURLcode res = curl_easy_perform(curl_handle);

        openai.status = OPENAI_END_OF_STREAM;

        /* cleanup curl stuff */
        curl_easy_cleanup(curl_handle);
        curl_global_cleanup();
    }

    return 0;
}

static void *openai_thread(void *arg)
{
    if (!streaming)
    {
        streaming = true;
        pthread_mutex_unlock(&openai_mutex);
        stream_openai(headers, openai_prompt, 5);
        printf("openai_thread: stream_openai returned\n");
        streaming = false;
    }
    return NULL;
}