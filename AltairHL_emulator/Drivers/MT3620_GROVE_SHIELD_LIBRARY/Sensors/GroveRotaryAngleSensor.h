//GROVE_NAME        "Grove - Rotary Angle Sensor"
//SKU               101020132
//WIKI_URL          http://wiki.seeedstudio.com/Grove-Rotary_Angle_Sensor/
#pragma once

void* GroveRotaryAngleSensor_Init(int i2cFd, int analog_pin);
float GroveRotaryAngleSensor_Read(void* inst);