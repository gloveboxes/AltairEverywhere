//GROVE_NAME        "Grove - Light Sensor v1.2"
//SKU               101020132
//WIKI_URL          http://wiki.seeedstudio.com/Grove-Light_Sensor/
#pragma once

void* GroveLightSensor_Init(int i2cFd, int analog_pin);
float GroveLightSensor_Read(void* inst);
