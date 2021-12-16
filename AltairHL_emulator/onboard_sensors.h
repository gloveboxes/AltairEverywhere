#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "hw/azure_sphere_learning_path.h"

#ifdef OEM_AVNET
#include "Drivers/AVNET/HL/imu_temp_pressure.h"
#endif

typedef struct {
    int temperature;
    int humidity;
    int pressure;
} ONBOARD_TELEMETRY;

bool onboard_sensors_read(ONBOARD_TELEMETRY *telemetry);
#ifdef OEM_AVNET
bool onboard_sensors_init(int i2c_fd);
#else
bool onboard_sensors_init(void);
#endif
bool onboard_sensors_close(void);