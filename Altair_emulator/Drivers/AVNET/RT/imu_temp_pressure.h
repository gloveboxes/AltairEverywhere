#pragma once

// #include "hw/azure_sphere_learning_path.h"
#include "lsm6dso_reg.h"
#include "lps22hh_reg.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>
#include "tx_api.h"
#include "os_hal_i2c.h"

#define LSM6DSO_ADDRESS	   0x6A	  // I2C Address
static const uint8_t i2c_speed = I2C_SCL_1000kHz;

typedef struct
{
	float x;
	float y;
	float z;
} AngularRateDegreesPerSecond;

typedef struct
{
	float x;
	float y;
	float z;
} AccelerationMilligForce;

bool lp_imu_initialize(void);
void lp_imu_close(void);
float lp_get_temperature(void);
float lp_get_pressure(void);
float lp_get_temperature_lps22h(void);	// get_temperature() from lsm6dso is faster
void lp_calibrate_angular_rate(void);
AngularRateDegreesPerSecond lp_get_angular_rate(void);
AccelerationMilligForce lp_get_acceleration(void);
