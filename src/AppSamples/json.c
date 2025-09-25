/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

/* C application to demonstrate use of Intel 8080 IO Ports */

/*
w = weather
l = location
p = pollution
*/

int w_key;
int w_value;
int w_items;
int l_key;
int l_value;
int l_items;
int p_key;
int p_value;
int p_items;

#define DELAY       10
#define JSON_LENGTH 256

char json[JSON_LENGTH];

char pressure[20];
char humidity[20];
char temperature[20];
char latitude[20];
char longitude[20];
char aqi[20];

main()
{
    unsigned c, l;
    char buffer[50];
    int i;
    char *data;

    w_key   = 34;
    w_value = 35;
    w_items = 6;
    l_key   = 36;
    l_value = 37;
    l_items = 4;
    p_key   = 38;
    p_value = 39;
    p_items = 9;

    printf("\nHello from the Altair 8800 emulator\n\n");

    for (c = 0; c < 65535; c++)
    {
        printf("%c[91;22;24m---------------------------------------------------------------", 27);
        printf("----------------------------------------------------------------------%c[0m\n", 27);

        printf("Reading: %u\t%s\n", c, get_port_data(43, 0, buffer, 50));

        print_k_v(l_key, l_value, l_items);
        print_k_v(w_key, w_value, w_items);
        print_k_v(p_key, p_value, p_items);

        get_port_data(w_value, 0, temperature, 20);
        get_port_data(w_value, 1, pressure, 20);
        get_port_data(w_value, 2, humidity, 20);

        get_port_data(l_value, 0, latitude, 20);
        get_port_data(l_value, 1, longitude, 20);
        get_port_data(p_value, 0, aqi, 20);

        publish_json();

        printf("Sleeping for %d seconds\n", DELAY);
        for (i = 0; i < DELAY; i++)
        {
            sleep(1);
            printf(".");
        }
        printf("\n\n");
    }
}

void print_k_v(key, value, items) int key;
int value;
int items;
{
    int i;
    char buffer[50];
    char *data;

    printf("%c[94;22;24m\n", 27);

    for (i = 0; i < items; i++)
    {
        data = get_port_data(key, i, buffer, 50);
        printf("%s", data);
        if (i < items - 1)
        {
            printf("\t");
            if (strlen(data) < 8)
            {
                printf("\t");
            }
        }
    }

    printf("%c[0m\n", 27);

    for (i = 0; i < items; i++)
    {
        data = get_port_data(value, i, buffer, 50);
        printf("%s\t", data);
        if (strlen(data) < 8)
        {
            printf("\t");
        }
    }
    printf("\n");
}

void publish_json()
{
    int json_length;
    int i;

    printf("\nPublishing JSON to Azure IoT Hub/Central\n");

    sprintf(json, "{\"temperature\":%s,\"pressure\":%s,\"humidity\":%s,\"latitude\":%s,\"longitude\":%s,\"aqi\":%s}", temperature, pressure,
        humidity, latitude, longitude, aqi);

    printf("\n%s\n\n", json);

    /* Wait on Port 31 to go false to signify no pending Azure IoT Publish */
    while (inp(31))
        ;

    /* Write the JSON to Port 31 */
    json_length = strlen(json);
    if (json_length > 0)
    {
        for (i = 0; i < json_length; i++)
        {
            outp(31, json[i]);
        }
        outp(31, 0);
    }
}

/* Sleep for n seconds */
void sleep(seconds) char seconds;
{
    outp(30, seconds); /* Enable sleep for N seconds */
    while (inp(30))
        ; /* Wait for sleep to expire */
}

/* Get data from Intel 8080 IO port */
char *get_port_data(port_num, port_data, buffer, buffer_len)
int port_num;
int port_data;
char *buffer;
int buffer_len;
{
    char ch;
    int index;

    index = 0;

    /* Select data to be read */
    outp(port_num, port_data);

    while ((ch = inp(200)) && index < buffer_len)
    {
        buffer[index++] = ch;
    }
    buffer[index] = 0x00;

    return buffer;
}
