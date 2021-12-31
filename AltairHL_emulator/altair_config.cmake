
# SELECT FRONT PANEL CONFIG #######################################################################################
#
set(ALTAIR_FRONT_PANEL_NONE TRUE "Altair on Azure Sphere with no panel.")
# set(ALTAIR_FRONT_PANEL_RETRO_CLICK TRUE "Avnet with the MikroE 8800 Retro Click")
# set(ALTAIR_FRONT_PANEL_KIT TRUE "Altair front panel board")
#
###################################################################################################################


# BOOT CONFIG #####################################################################################################
#
# Comment out the "add_compile_definitions(BOOT_CPM) line below to boot Altair BASIC 4.0
#
add_compile_definitions(BOOT_CPM)
#
###################################################################################################################

# WEB TERMINAL ####################################################################################################
#
add_compile_definitions(ENABLE_WEB_TERMINAL)
#
###################################################################################################################


##################################################################################################################
# Configure Azure IoT Hub conection string
add_compile_definitions(IOT_HUB_HOST_NAME="HostName=iotc-1b7e7c6e-7477-4074-901e-242361cc0b1c.azure-devices.net")
add_compile_definitions(IOT_HUB_DEVICE_ID="DeviceId=DesktopAltair")
add_compile_definitions(IOT_HUB_SHARED_ACCESS_KEY="SharedAccessKey=X0+NFivH/PQ1UZqfJciUs31UcsC682rAF4P1u3aWW4o=")
##################################################################################################################
