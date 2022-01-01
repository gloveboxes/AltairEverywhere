//GROVE_NAME        "Grove - Temperature&Humidity Sensor (SHT31)"
//SKU               101020212
//WIKI_URL          http://wiki.seeedstudio.com/Grove-TempAndHumi_Sensor-SHT31/

#pragma once
void* GroveTempHumiSHT31_Open(int i2cFd);
void GroveTempHumiSHT31_Read(void* inst);
void GroveTempHumiSHT31_EnableHeater(void* inst);
void GroveTempHumiSHT31_DisableHeater(void* inst);
float GroveTempHumiSHT31_GetTemperature(void* inst);
float GroveTempHumiSHT31_GetHumidity(void* inst);
