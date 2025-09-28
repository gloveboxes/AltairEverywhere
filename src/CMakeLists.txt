#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required (VERSION 3.10)
project (Altair_emulator C)

# SELECT FRONT PANEL CONFIG #######################################################################################
#
# set(ALTAIR_FRONT_PI_SENSE_HAT TRUE "Enable Raspberry Pi Sense HAT")
# set(ALTAIR_FRONT_PANEL_KIT TRUE "Front_Panel_Kit")
#
###################################################################################################################

###################################################################################################################
#
# set(ALTAIR_CLOUD TRUE "Enable Altair as a service real-time in-memory differencing disk")
###################################################################################################################

if (ALTAIR_CLOUD)
    add_compile_definitions(ALTAIR_CLOUD)
endif(ALTAIR_CLOUD)

# overide wsServer MAX_CLIENTS
add_compile_definitions(MAX_CLIENTS=1)

# set(DISABLE_IO_MOCKING ON CACHE  BOOL "DISABLE IO mocking for AzureSphereRemoteX.Client" FORCE )

# ENABLE_EDGE_DEVX_REMOTEX is enabled as Log_Debug found in appslib is required.
# set(ENABLE_EDGE_DEVX_REMOTEX ON CACHE  BOOL "DISABLE EdgeDevX RemoteX peripheral support" FORCE )

#if(DISABLE_IO_MOCKING)
#    add_compile_definitions(DISABLE_IO_MOCKING=TRUE)
#endif(DISABLE_IO_MOCKING)

if (ALTAIR_FRONT_PI_SENSE_HAT)

    message(STATUS, "ALTAIR PROJECT CONFIGURATION SELECTED: RASPBERRY PI SENSE HAT FRONT PANEL")

    add_compile_definitions(ALTAIR_FRONT_PANEL_PI_SENSE)

    set(Oem
        "FrontPanels/front_panel_pi_sense_hat.c"
        "Drivers/pi_sense_hat/led_panel.c"
        "Drivers/pi_sense_hat/graphics.c"
        "Drivers/sensehat-driver/src/sense_hat.c"
    )
    source_group("Oem" FILES ${Oem})

else()

    message(STATUS, "ALTAIR PROJECT CONFIGURATION SELECTED: NO FRONT PANEL")

    set(Oem
    "FrontPanels/front_panel_none.c"
    )
    source_group("Oem" FILES ${Oem})

endif(ALTAIR_FRONT_PI_SENSE_HAT)

if (ALTAIR_FRONT_PANEL_KIT)

    add_compile_definitions(ALTAIR_FRONT_PANEL_KIT)
    add_compile_definitions(DEVICE_PANEL="Front Panel Kit")

    set(Oem
        "FrontPanels/front_panel_kit.c"
        "Drivers/front_panel/graphics.c"
        "Drivers/spidev-lib/src/spidev_lib.c"
    )
    source_group("Oem" FILES ${Oem})
endif(ALTAIR_FRONT_PANEL_KIT)


set(Source
    "Altair8800/88dcdd.c"
    "Altair8800/intel8080.c"
    "Altair8800/memory.c"
    "altair_config.c"
    "io_ports.c"
    "cpu_monitor.c"
    "difference_disk.c"
    "iotc_manager.c"
    "web_console.c"
    "main.c"
    "environment.c"
    # "env_air_visual.c"
    "env_open_weather_map.c"
    "location_from_ip.c"
    "utils.c"  
    "PortDrivers/azure_io.c"
    "PortDrivers/led_matrix_io.c"
    "PortDrivers/weather_io.c"
    "PortDrivers/time_io.c"
    "PortDrivers/file_io.c"
    "PortDrivers/utility_io.c"
    "PortDrivers/onboard_io.c"
    "PortDrivers/openai.c"
    "PortDrivers/power_io.c"
    )
source_group("Source" FILES ${Source})

set(wsServer
    wsServer/src/ws.c
    wsServer/src/base64.c
    wsServer/src/sha1.c
    wsServer/src/handshake.c
    wsServer/src/utf8.c
)
source_group("wsServer" FILES ${wsServer})

set(ALL_FILES
    ${Source}
    ${Oem}
    ${wsServer}
)

# Create executable
add_executable(${PROJECT_NAME} ${ALL_FILES})

# Configure MQTT if hostname is specified
if (ALTAIR_MQTT_BROKER_HOSTNAME)
    target_compile_options(${PROJECT_NAME} PRIVATE "-DMQTT_BROKER_HOSTNAME=\"${ALTAIR_MQTT_BROKER_HOSTNAME}\"")
endif(ALTAIR_MQTT_BROKER_HOSTNAME)

################################################################################
# Following lines required for EdgeMqttDevX
add_subdirectory("EdgeMqttDevX" edge_mqtt_devx)
target_link_libraries(${PROJECT_NAME} "edge_mqtt_devx")
################################################################################

# target_compile_definitions(${PROJECT_NAME} PUBLIC AZURE_IOT_HUB_CONFIGURED)
target_link_libraries(${PROJECT_NAME} c curl)

target_include_directories(${PROJECT_NAME} PUBLIC /usr/local/include)
target_link_options(${PROJECT_NAME} PUBLIC "-L/usr/local/lib")
target_include_directories(${PROJECT_NAME} PUBLIC AzureSphereDevX/include )
target_include_directories(${PROJECT_NAME} PRIVATE wsServer/include)
target_include_directories(${PROJECT_NAME} PUBLIC Drivers/pi_sense_hat)
target_include_directories(${PROJECT_NAME} PUBLIC Drivers/front_panel)
target_include_directories(${PROJECT_NAME} PUBLIC Drivers/spidev-lib/src)
target_include_directories(${PROJECT_NAME} PUBLIC Drivers/sensehat-driver/src)
target_include_directories(${PROJECT_NAME} PUBLIC uthash/src)


include_directories(${CMAKE_SOURCE_DIR} FrontPanels)
include_directories(${CMAKE_SOURCE_DIR} Altair8800)


target_compile_options(${PROJECT_NAME} PRIVATE -Wno-unknown-pragmas)

set_source_files_properties(Altair8800/intel8080.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(Altair8800/intel8080.h PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(Altair8800/memory.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(FrontPanels/front_panel_virtual.c PROPERTIES COMPILE_FLAGS -Wno-conversion)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
