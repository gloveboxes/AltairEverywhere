The BD Software C compiler is included on drive A: or drive B: if you have the CP/M filesystem loaded on the Micro SD card. The following text is an exert from the [BDS C Wikipedia](https://en.wikipedia.org/wiki/BDS_C){:target=_blank} article.

*BDS C (or the BD Software C Compiler) is a compiler for a sizeable subset of the C programming language, that ran on and generated code for the Intel 8080 and Zilog Z80 processors. It was the first C compiler for CP/M. It was written by Leor Zolman and first released in 1979 when he was 20 years old. "BDS" stands for "Brain Damage Software."*

## BDS C User's Guide

Refer to the [BDS C User's Guide](https://github.com/AzureSphereCloudEnabledAltair8800/Altair8800.manuals/blob/master/BDS_C_Compiler.pdf){:target=_blank} for more about the language and its implementation.

## Compile C applications

The CP/M disk image includes a simple *HW.C* (Hello, world) application. BDS C language has support for Intel 8080 CPU input and output port instructions. The *HW.C* application displays the system tick count, UTC, and local date and time, and then sleeps for 1 second. For more information about Intel 8080 IO port mappings, refer to [Intel 8080 input and output ports](https://github.com/gloveboxes/Altair8800.Emulator.UN-X/wiki#intel-8080-input-and-output-ports){:target=_blank}.

Follow these steps to list, compile, link, and run the *HW.C* file:

1. List the *hw.c* file

    ```cpm
    type hw.c
    ```

    ```c
    /* Copyright (c) Microsoft Corporation. All rights reserved.
       Licensed under the MIT License. */

    /* C application to demonstrate use of Intel 8080 IO Ports */

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

        while ((ch = inp(port_num)) && index < buffer_len) {
            buffer[index++] = ch;
        }
        buffer[index] = 0x00;

        return buffer;
    }
    ```

1. Compile the *hw.c* file:

    ```cpm
    cc hw
    ```

1. Link the *hw* application:

    ```cpm
    clink hw
    ```

1. Run the *hw* application:

    ```cpm
    hw
    ```

1. Stop the *hw* application by selecting **Ctrl+C**.

## Editing files

See [Editing Files](01-Editing-files.md){:target=_blank} for details about transferring, editing, compiling, and running C source files.
