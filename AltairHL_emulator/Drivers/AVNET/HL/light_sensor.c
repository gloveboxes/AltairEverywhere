#include "light_sensor.h"

static int adcControllerFd = -1;
// The size of a sample in bits
static int sampleBitCount = -1;
// The maximum voltage
static float sampleMaxVoltage = 2.5f;


int avnet_get_light_level(void) {
    uint32_t value;
    int result = ADC_Poll(adcControllerFd, ADC_CHANNEL, &value);
    if (result == -1) {
        Log_Debug("ADC_Poll failed with error: %s (%d)\n", strerror(errno), errno);
        return -1.0;
    }

    float voltage = ((float)value * sampleMaxVoltage) / (float)((1 << sampleBitCount) - 1);
    //Log_Debug("The out sample value is %.3f V\n", voltage);
    int percentage = (int)(voltage / sampleMaxVoltage * 100);
    //Log_Debug("Light as a percentage value is %d\n", percentage);
    return percentage;
}


bool avnet_open_adc(ADC_ControllerId adc_id) {
    adcControllerFd = ADC_Open(adc_id);
    if (adcControllerFd < 0) {
        Log_Debug("ADC_Open failed with error: %s (%d)\n", strerror(errno), errno);
        return false;
    }

    sampleBitCount = ADC_GetSampleBitCount(adcControllerFd, ADC_CHANNEL);
    if (sampleBitCount == -1) {
        Log_Debug("ADC_GetSampleBitCount failed with error : %s (%d)\n", strerror(errno), errno);
        return false;
    }
    if (sampleBitCount == 0) {
        Log_Debug("ADC_GetSampleBitCount returned sample size of 0 bits.\n");
        return false;
    }

    int result = ADC_SetReferenceVoltage(adcControllerFd, ADC_CHANNEL,
        sampleMaxVoltage);
    if (result < 0) {
        Log_Debug("ADC_SetReferenceVoltage failed with error : %s (%d)\n", strerror(errno), errno);
        return false;
    }

    return true;
}