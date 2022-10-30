/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "file_io.h"
#include "dx_utilities.h"
#include <curl/curl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#ifdef AZURE_SPHERE
#include <applibs/storage.h>
#endif // AZURE_SPHERE

#define APP_SAMPLES_DIRECTORY "AppSamples"

#define GAMES_REPO              "https://raw.githubusercontent.com/AzureSphereCloudEnabledAltair8800/RetroGames/main"
#define ENDPOINT_LEN            128
#define ENDPOINT_ELEMENTS       2

static int copy_web(char *url);
static char devget_path_and_filename[50];

enum WEBGET_STATUS
{
    WEBGET_EOF,
    WEBGET_WAITING,
    WEBGET_DATA_READY,
    WEBGET_FAILED
};

typedef struct
{
    bool end_of_file;
    char *byte_stream;
    char bytes[1024];
    char personal_endpoint[ENDPOINT_LEN];
    uint8_t selected_endpoint;
    char filename[15];
    char url[150];
    enum WEBGET_STATUS status;
    int index;
    size_t byte_stream_length;
} WEBGET_T;

typedef struct
{
    int fd;
    char filename[15];
    bool file_opened;
    bool enabled;
    bool end_of_file;
    int index;
    int ch;
} DEVGET_T;

static WEBGET_T webget;
static DEVGET_T devget;

pthread_mutex_t webget_mutex = PTHREAD_MUTEX_INITIALIZER;

DX_ASYNC_HANDLER(async_copyx_request_handler, handle)
{
    copy_web(webget.url);
}
DX_ASYNC_HANDLER_END

size_t file_output(int port, uint8_t data, char *buffer, size_t buffer_length)
{
    size_t len = 0;
    switch (port)
    {
#ifdef AZURE_SPHERE
        case 68: // set devget filename
            if (devget.index == 0)
            {
                if (devget.file_opened && devget.fd != -1)
                {
                    close(devget.fd);
                }
                memset(&devget, 0x00, sizeof(DEVGET_T));
                devget.fd = -1;
            }

            if (data != 0 && devget.index < sizeof(devget.filename))
            {
                devget.filename[devget.index] = data;
                devget.index++;
            }

            if (data == 0) // NULL TERMINATION
            {
                devget.index = 0;
            }

            break;

#endif            // AZURE SPHERE
        case 110: // Set getfile custom endpoint url
            if (webget.index == 0)
            {
                memset(webget.personal_endpoint, 0x00, ENDPOINT_LEN);
            }

            if (data != 0 && webget.index < ENDPOINT_LEN)
            {
                webget.personal_endpoint[webget.index++] = data;
            }

            if (data == 0) // NULL TERMINATION
            {
                webget.personal_endpoint[webget.index] = 0x00;
                webget.index                           = 0;
            }
            break;
        case 111: // Load getfile (gf) custom endpoint url
            len = (size_t)snprintf(buffer, buffer_length, "%s", webget.personal_endpoint);
            break;
        case 112: // Select getfile (gf) endpoint to use
            if (data < ENDPOINT_ELEMENTS)
            {
                webget.selected_endpoint = data;
            }
            break;
        case 113: // Load getfile (gf) selected endpoint
            len = (size_t)snprintf(buffer, buffer_length, "%d", webget.selected_endpoint);
            break;
        case 114: // copy file from web server to mutable storage
            if (webget.index == 0)
            {
                memset(webget.filename, 0x00, sizeof(webget.filename));
            }

            if (data != 0 && webget.index < sizeof(webget.filename))
            {
                webget.filename[webget.index] = data;
                webget.index++;
            }

            if (data == 0) // NULL TERMINATION
            {
                webget.index              = 0;
                webget.status             = WEBGET_WAITING;
                webget.byte_stream_length = 0;
                webget.end_of_file        = false;
                pthread_mutex_unlock(&webget_mutex);

                memset(webget.url, 0x00, sizeof(webget.url));

                switch (webget.selected_endpoint)
                {
                    case 0:
                        snprintf(webget.url, sizeof(webget.url), "%s/%s", GAMES_REPO, webget.filename);
                        break;

                    case 1:
                        snprintf(webget.url, sizeof(webget.url), "%s/%s", webget.personal_endpoint, webget.filename);
                        break;
                    default:
                        break;
                }
                dx_asyncSend(&async_copyx_request, NULL);
            }
            break;
    }

    return len;
}

uint8_t file_input(uint8_t port)
{
    uint8_t retVal = 0;

    switch (port)
    {
        case 33: // has copyx file need copied and loaded
            retVal = webget.status;
            break;
        case 68: // has devget eof
            retVal = devget.end_of_file;
            break;
        case 201: // Read file from http(s) web server
            if (webget.byte_stream_length > 0)
            {
                retVal = webget.byte_stream[0];
                webget.byte_stream++;
                webget.byte_stream_length--;

                if (webget.byte_stream_length == 0)
                {
                    webget.status = webget.end_of_file ? WEBGET_EOF : WEBGET_WAITING;
                    pthread_mutex_unlock(&webget_mutex);
                }
            }
            else
            {
                retVal = 0x00;
            }
            break;
#ifdef AZURE_SPHERE

        case 202: // READ DEVGET file from immutable storage
            if (devget.end_of_file)
            {
                retVal = 0x00;
            }
            else
            {
                if (!devget.file_opened)
                {
                    /* open the file */
                    snprintf(devget_path_and_filename, sizeof(devget_path_and_filename), "%s/%s", APP_SAMPLES_DIRECTORY, devget.filename);

                    if ((devget.fd = Storage_OpenFileInImagePackage(devget_path_and_filename)) != -1)
                    {
                        devget.file_opened = true;
                    }
                    else
                    {
                        devget.end_of_file = true;
                    }
                }

                if (devget.file_opened)
                {
                    if (read(devget.fd, &retVal, 1) == 0)
                    {
                        close(devget.fd);

                        devget.file_opened = false;
                        devget.end_of_file = true;
                        devget.fd          = -1;
                        retVal             = 0x00;
                    }
                }
                else
                {
                    retVal = 0x00;
                }
            }
            break;

#endif // AZURE SPHERE
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
    while ((rv = pthread_mutex_trylock(mutex)) == EBUSY)
    {
        ts.tv_sec  = 0;
        ts.tv_nsec = (remaining.tv_sec > 0 ? 10000000 : (remaining.tv_nsec < 10000000 ? remaining.tv_nsec : 10000000));

        nanosleep(&ts, &slept);

        ts.tv_nsec -= slept.tv_nsec;
        if (ts.tv_nsec <= remaining.tv_nsec)
        {
            remaining.tv_nsec -= ts.tv_nsec;
        }
        else
        {
            remaining.tv_sec--;
            remaining.tv_nsec = (1000000 - (ts.tv_nsec - remaining.tv_nsec));
        }

        if (remaining.tv_sec < 0 || (!remaining.tv_sec && remaining.tv_nsec <= 0))
        {
            return ETIMEDOUT;
        }
    }

    return rv;
}

#endif // __APPLE__

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *webget)
{
    size_t realsize = size * nmemb;
    WEBGET_T *wg    = (WEBGET_T *)webget;

    struct timespec timeoutTime;
    memset(&timeoutTime, 0, sizeof(struct timespec));

#ifndef __APPLE__
    clock_gettime(CLOCK_REALTIME, &timeoutTime);
#endif // __APPLE__

    timeoutTime.tv_sec += 20;

    if (pthread_mutex_timedlock(&webget_mutex, &timeoutTime) != 0)
    {
        wg->status = WEBGET_FAILED;
    }
    else
    {
        memcpy(wg->bytes, ptr, realsize);

        wg->byte_stream        = wg->bytes;
        wg->byte_stream_length = realsize;
        wg->status             = WEBGET_DATA_READY;
    }

    return realsize;
}

static int copy_web(char *url)
{
    CURL *curl_handle;

    /* init the curl session */
    curl_handle = curl_easy_init();

    /* set URL to get here */
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);

    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);

    // https://curl.se/libcurl/c/CURLOPT_NOSIGNAL.html
    curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1L);

    // https://curl.se/libcurl/c/CURLOPT_TIMEOUT.html
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 12L);

    /* Switch on full protocol/debug output while testing */
    // curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);

    /* disable progress meter, set to 0L to enable it */
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);

    // set returned data chuck to max 128 bytes
    curl_easy_setopt(curl_handle, CURLOPT_BUFFERSIZE, 1024L);

    /* send all data to this function  */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);

    // A long parameter set to 1 tells the library to fail the request if the HTTP code returned is equal to
    // or larger than 400
    curl_easy_setopt(curl_handle, CURLOPT_FAILONERROR, true);

    webget.status      = WEBGET_WAITING;
    webget.end_of_file = false;

    /* write the page body to this file handle */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &webget);

    /* get it! */
    CURLcode res = curl_easy_perform(curl_handle);

    if (res != CURLE_OK)
    {
        webget.status = WEBGET_FAILED;
    }

    webget.end_of_file = true;

    /* cleanup curl stuff */
    curl_easy_cleanup(curl_handle);

    curl_global_cleanup();

    return 0;
}
