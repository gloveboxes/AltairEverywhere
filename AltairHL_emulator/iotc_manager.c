/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "iotc_manager.h"

/// <summary>
/// Device Twin Handler to set the desired temperature value
/// </summary>
void device_twin_set_cpu_state_handler(DX_DEVICE_TWIN_BINDING* deviceTwinBinding) {
#ifdef ENABLE_WEB_TERMINAL
	if (is_mqtt_connected()) {
		cpu_operating_mode = *(bool*)deviceTwinBinding->propertyValue ? CPU_RUNNING : CPU_STOPPED;
	}
#else
	cpu_operating_mode = *(bool*)deviceTwinBinding->propertyValue ? CPU_RUNNING : CPU_STOPPED;
#endif // ENABLE_WEB_TERMINAL
	dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue, DX_DEVICE_TWIN_RESPONSE_COMPLETED);
}

void device_twin_set_local_serial_handler(DX_DEVICE_TWIN_BINDING* deviceTwinBinding) {
	local_serial = *(bool*)deviceTwinBinding->propertyValue;
	dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue, DX_DEVICE_TWIN_RESPONSE_COMPLETED);
}

/// <summary>
/// Device Twin Handler to set the brightness of MAX7219 8x8 LED panel8x8
/// </summary>
void device_twin_set_led_brightness_handler(DX_DEVICE_TWIN_BINDING* deviceTwinBinding) {
    if (!IN_RANGE(*(int *)deviceTwinBinding->propertyValue, 0, 15)) {
		dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue, DX_DEVICE_TWIN_RESPONSE_ERROR);
	} else {
#ifdef ALTAIR_FRONT_PANEL_CLICK
		max7219_set_brightness(&panel8x8, (unsigned char)*(int*)deviceTwinBinding->propertyValue);
#elif ALTAIR_FRONT_PANEL_RETRO_CLICK
		as1115_set_brightness(&retro_click, (unsigned char)*(int *)deviceTwinBinding->propertyValue);
#endif // ALTAIR_FRONT_PANEL_CLICK
		dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue, DX_DEVICE_TWIN_RESPONSE_COMPLETED);
	}
}

/// <summary>
/// Device Twin Handler to set the mqtt channel id
/// </summary>
void device_twin_set_channel_id_handler(DX_DEVICE_TWIN_BINDING* deviceTwinBinding) {
	write_channel_id_to_storage(*(int*)deviceTwinBinding->propertyValue);
	dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue, DX_DEVICE_TWIN_RESPONSE_COMPLETED);
}

/// <summary>
/// Report back to IoT Central device memory usage
/// </summary>
/// <param name="eventLoopTimer"></param>
void memory_diagnostics_handler(EventLoopTimer* eventLoopTimer) {
	if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0) {
		dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
		return;
	}

	if (dx_jsonSerialize(msgBuffer, sizeof(msgBuffer), 2,
		DX_JSON_INT, "TotalMemoryUsage", Applications_GetTotalMemoryUsageInKB(),
		DX_JSON_INT, "PeakUserModeMemoryUsage", Applications_GetPeakUserModeMemoryUsageInKB()))
	{
		dx_azurePublish(msgBuffer, strlen(msgBuffer), NULL, 0, NULL);
	}
}

void publish_telemetry(int temperature, int pressure) {
	if (dx_jsonSerialize(msgBuffer, sizeof(msgBuffer), 2,
		DX_JSON_INT, "Temperature", temperature, 
		DX_JSON_INT, "Pressure", pressure))
	{
		dx_azurePublish(msgBuffer, strlen(msgBuffer), NULL, 0, NULL);
	}
}

/// <summary>
/// Restart the Device
/// </summary>
void delay_restart_device_handler(EventLoopTimer* eventLoopTimer) {
	if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0) {
		dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
		return;
	}

	PowerManagement_ForceSystemReboot();
}

/// <summary>
/// Start Device Power Restart Direct Method
/// </summary>
DX_DIRECT_METHOD_RESPONSE_CODE RestartDeviceHandler(JSON_Value* json, DX_DIRECT_METHOD_BINDING* directMethodBinding, char** responseMsg) {
	*responseMsg = NULL;
	dx_timerOneShotSet(&restartDeviceOneShotTimer, &(struct timespec){2, 0});  // restart the device in 2 seconds
	return DX_METHOD_SUCCEEDED;
}