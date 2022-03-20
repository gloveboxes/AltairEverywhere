/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

/* C application to demonstarte use of Intel 8080 IO Ports */

main()
{
    unsigned c, l;
    char buffer[50];

    printf("\nHello from the Altair 8800 emulator\n\n");

    for (c = 0; c < 65535; c++) 
    {
        printf("Count:%u\n", c);
        printf("System tick count: %s\n", get_port_data(41, buffer, 50));
        printf("UTC date and time: %s\n", get_port_data(42, buffer, 50));
        printf("Local date and time: %s\n\n", get_port_data(43, buffer, 50));

        sleep(1); /* Sleep for 1 second */
    }
}

/* Sleep for n seconds */
sleep(seconds)
char seconds;
{
    outp(30, seconds); /* Enable sleep for N seconds */
    while (inp(30)); /* Wait for sleep to expire */
}

/* Get data from Intel 8080 IO port */
char *get_port_data(port_num, buffer, buffer_len)
int port_num;
char *buffer;
int buffer_len;
{
    char ch;
    int index;

    index = 0;

    /* Select data to be read */
    outp(port_num, 0);

    while ((ch = inp(200)) && index < buffer_len) {
        buffer[index++] = ch;
    }
    buffer[index] = 0x00;

    return buffer;
}