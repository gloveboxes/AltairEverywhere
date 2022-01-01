#pragma once

#include "../applibs_versions.h"
#include <applibs/gpio.h>

void* GroveAD7992_Open(int i2cFd);
float GroveAD7992_Read(void* inst, int channel);
float GroveAD7992_ConvertToMillisVolt(float value);
