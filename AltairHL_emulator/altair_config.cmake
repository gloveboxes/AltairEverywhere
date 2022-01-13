
# SELECT FRONT PANEL CONFIG #######################################################################################
#
# set(ALTAIR_FRONT_PANEL_NONE TRUE "Altair on Azure Sphere with no panel.")
set(ALTAIR_FRONT_PI_SENSE_HAT TRUE "Avnet with the MikroE 8800 Retro Click")
#
###################################################################################################################


##################################################################################################################
# Configure Azure IoT Hub conection string
add_compile_definitions(IOT_HUB_HOST_NAME="HostName=iotc-1b7e7c6e-7477-4074-901e-242361cc0b1c.azure-devices.net")
add_compile_definitions(IOT_HUB_DEVICE_ID="DeviceId=RaspberryAltair")
add_compile_definitions(IOT_HUB_SHARED_ACCESS_KEY="SharedAccessKey=NGp9Mzi9N+respyd4/VU9PCimA3dq/yKvdvTKA1qGao=")
##################################################################################################################
