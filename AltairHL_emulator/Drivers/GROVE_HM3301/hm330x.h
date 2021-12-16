/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#ifndef HM330X_H
#define HM330X_H

// https://github.com/Seeed-Studio/Seeed_PM2_5_sensor_HM3301
// https://media.digikey.com/pdf/Data%20Sheets/Seeed%20Technology/Grove_Laser_PM2.5_Sensor_HM3301_Web.pdf
// https://www.seeedstudio.com/Grove-Laser-PM2-5-Sensor-HM3301.html

// Typical applications:
// Air purifier / air conditioner 
// Air quality testing equipment 
// Industrial PM value analysis 
// Dust and smoke detection and analysis 
// Real-time PM2.5, PM10, TSP detector 
// Multichannel particle counter 
// Environmental testing equipment

#include <applibs/i2c.h>
#include <applibs/log.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#define HM33X_I2C_ADDRESS 0x40
#define SELECT_COMM_CMD 0X88

typedef struct
{
    uint16_t standard_particulate_matter_1_0;
    uint16_t standard_particulate_matter_2_5;
    uint16_t standard_particulate_matter_3_0;
    uint16_t atmospheric_environment_1_0;
    uint16_t atmospheric_environment_2_5;
    uint16_t atmospheric_environment_3_0;
} PARTICULATE_MATTER_DATA;

typedef struct {
    int fd;
    bool initialized;
    bool valid_data;
    PARTICULATE_MATTER_DATA data;
} hm330x_t;

bool hm330x_init(int i2c_fd, hm330x_t* sensor);
bool hm330x_read(hm330x_t* sensor);

#endif /* HM330X_H */