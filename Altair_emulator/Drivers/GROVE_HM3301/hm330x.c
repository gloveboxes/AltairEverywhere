/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "hm330x.h"

static uint8_t input_buffer[29];

const char *str[] = {
    "sensor num: ",
    "PM1.0 concentration(CF=1,Standard particulate matter,unit:ug/m3): ",
    "PM2.5 concentration(CF=1,Standard particulate matter,unit:ug/m3): ",
    "PM10 concentration(CF=1,Standard particulate matter,unit:ug/m3): ",
    "PM1.0 concentration(Atmospheric environment,unit:ug/m3): ",
    "PM2.5 concentration(Atmospheric environment,unit:ug/m3): ",
    "PM10 concentration(Atmospheric environment,unit:ug/m3): ",
};

uint16_t get_value(uint8_t *data, int index)
{
    return (uint16_t)((uint16_t)data[index * 2] << 8 | data[index * 2 + 1]);
}

/*parse buf with 29 uint8_t-data*/
void parse_result(hm330x_t *sensor, uint8_t *data)
{
    sensor->data.standard_particulate_matter_1_0 = get_value(data, 2);
    sensor->data.standard_particulate_matter_2_5 = get_value(data, 3);
    sensor->data.standard_particulate_matter_3_0 = get_value(data, 4);
    sensor->data.atmospheric_environment_1_0 = get_value(data, 5);
    sensor->data.atmospheric_environment_2_5 = get_value(data, 6);
    sensor->data.atmospheric_environment_3_0 = get_value(data, 7);
}

static bool parse_result_value(hm330x_t *sensor, uint8_t *data)
{
    if (NULL == data)
    {
        return false;
    }

    uint8_t sum = 0;
    for (int i = 0; i < 28; i++)
    {
        sum = (uint8_t)(sum + data[i]);
    }
    if (sum != data[28])
    {
        // Log_Debug("Particulate matter sensor: Invalid checkSum returned\n");
        return false;
    }

    parse_result(sensor, input_buffer);

    return true;
}

bool hm330x_read(hm330x_t *sensor)
{
    if (!sensor->initialized)
    {
        return false;
    }

    memset(input_buffer, 0x00, sizeof(input_buffer));

    if (I2CMaster_Read(sensor->fd, HM33X_I2C_ADDRESS, input_buffer, sizeof(input_buffer)) != sizeof(input_buffer))
    {
        // Log_Debug("HM33X sensor read failed\n");
        return false;
    }

    return sensor->valid_data = parse_result_value(sensor, input_buffer);
}

bool hm330x_init(int i2c_fd, hm330x_t *sensor)
{
    if (sensor->initialized)
    {
        return true;
    }

    memset(sensor, 0x00, sizeof(hm330x_t));

    if (i2c_fd == -1)
    {
        Log_Debug("ERROR: I2CMaster_Open: errno=%d (%s)\n", errno, strerror(errno));
        return false;
    }

    sensor->fd = i2c_fd;
    sensor->initialized = true;

    // start the sensor
    uint8_t cmd = SELECT_COMM_CMD;

    if (I2CMaster_Write(i2c_fd, HM33X_I2C_ADDRESS, &cmd, 1) != 1)
    {
        Log_Debug("HM330X cmd failed.\n");
        return false;
    }

    // Note, on power up the first read checksum is can be invalid, so retry
    int retry = 0;
    while (!hm330x_read(sensor) && retry++ < 4)
    {
        nanosleep(&(struct timespec){0, 500 * 1000000}, NULL);
    }

    return true;
}