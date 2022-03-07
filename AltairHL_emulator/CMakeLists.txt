#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required (VERSION 3.10)
project (Altair_emulator C)

# SELECT FRONT PANEL CONFIG #######################################################################################
#
# set(ALTAIR_FRONT_PI_SENSE_HAT TRUE "Enable Raspberry Pi Sense HAT")
#
###################################################################################################################

# BOOT CONFIG #####################################################################################################
#
# Comment out the "add_compile_definitions(BOOT_CPM) line below to boot Altair BASIC 4.0
#
add_compile_definitions(BOOT_CPM)
#
###################################################################################################################

# wolfMQTT Settings ###############################################################################################
#
# add_compile_definitions(WOLFMQTT_MULTITHREAD)
# add_compile_definitions(WOLFMQTT_DISCONNECT_CB)
# add_compile_definitions(WOLFMQTT_NO_STDIN_CAP)
# add_compile_definitions(WOLFSSL_USER_SETTINGS)
# # add_compile_definitions(WOLFMQTT_V5)
# add_compile_definitions(NO_DH)
# add_compile_definitions(WOLFMQTT_USER_SETTINGS)

###################################################################################################################

add_compile_definitions(PRINTF=Log_Debug)

set(DISABLE_IO_MOCKING ON CACHE  BOOL "DISABLE IO mocking for AzureSphereRemoteX.Client" FORCE )

# ENABLE_EDGE_DEVX_REMOTEX is enabled as Log_Debug found in appslib is required.
set(ENABLE_EDGE_DEVX_REMOTEX ON CACHE  BOOL "DISABLE EdgeDevX RemoteX peripheral support" FORCE )

if(DISABLE_IO_MOCKING)
    add_compile_definitions(DISABLE_IO_MOCKING=TRUE)
endif(DISABLE_IO_MOCKING)

if (ALTAIR_FRONT_PI_SENSE_HAT)

    message(STATUS, "ALTAIR PROJECT CONFIGURATION SELECTED: RASPBERRY PI SENSE HAT FRONT PANEL")

    add_compile_definitions(ALTAIR_FRONT_PANEL_PI_SENSE)

    set(Oem
        "FrontPanels/front_panel_pi_sense_hat.c"
        "Drivers/pi_sense_hat/led_panel.c"
    )
    source_group("Oem" FILES ${Oem})

else()

    message(STATUS, "ALTAIR PROJECT CONFIGURATION SELECTED: NO FRONT PANEL")

    set(Oem
    "FrontPanels/front_panel_none.c"
    )
    source_group("Oem" FILES ${Oem})

endif(ALTAIR_FRONT_PI_SENSE_HAT)


set(Source
    "Altair8800/88dcdd.c"
    "Altair8800/intel8080.c"
    "Altair8800/memory.c"
    "altair_config.c"
    "io_ports.c"
    "FrontPanels/front_panel_virtual.c"
    "iotc_manager.c"
    "mqtt_manager.c"
    "main.c"
    "environment.c"
    # "env_air_visual.c"
    "env_open_weather_map.c"
    "location_from_ip.c"
    "storage.c"
    "utils.c"  
)
source_group("Source" FILES ${Source})

# set(wolfMQTT
#     "wolfMQTT/src/mqtt_client.c"
#     "wolfMQTT/src/mqtt_packet.c"
#     "wolfMQTT/src/mqtt_socket.c"
#     "wolfMQTT/examples/mqttnet.c"
#     "wolfMQTT/examples/mqttexample.c"
# )
# source_group("wolfMQTT" FILES ${wolfMQTT})

set(MQTT-C
    MQTT-C/src/mqtt_pal.c
    MQTT-C/src/mqtt.c
)
source_group("MQTT-C" FILES ${MQTT-C})


set(ALL_FILES
    ${Source}
    ${Oem}
    ${MQTT-C}
)

# # MQTT-C library
# add_library(mqttc STATIC
#     MQTT-C/src/mqtt_pal.c
#     MQTT-C/src/mqtt.c
# )
# target_include_directories(mqttc PUBLIC MQTT-C/include)
# target_link_libraries(mqttc PUBLIC 
#     $<$<C_COMPILER_ID:MSVC>:ws2_32>
# )

# Create executable
add_executable(${PROJECT_NAME} ${ALL_FILES})

################################################################################
# Following lines required for EdgeDevX
add_subdirectory("EdgeDevX" edge_devx)
target_link_libraries(${PROJECT_NAME} "edge_devx")
################################################################################



# target_compile_definitions(${PROJECT_NAME} PUBLIC AZURE_IOT_HUB_CONFIGURED)
target_link_libraries(${PROJECT_NAME} applibs pthread c edge_devx curl)

target_include_directories(${PROJECT_NAME} PUBLIC /usr/local/include)
target_link_options(${PROJECT_NAME} PUBLIC "-L/usr/local/lib")


target_include_directories(${PROJECT_NAME} PUBLIC AzureSphereDevX/include )
# target_include_directories(${PROJECT_NAME} PUBLIC ../IntercoreContract)
# target_include_directories(${PROJECT_NAME} PUBLIC wolfMQTT wolfMQTT/wolfmqtt ./ )
target_include_directories(${PROJECT_NAME} PUBLIC MQTT-C/include)
target_include_directories(${PROJECT_NAME} PUBLIC Drivers/pi_sense_hat)
include_directories(${CMAKE_SOURCE_DIR} FrontPanels)
include_directories(${CMAKE_SOURCE_DIR} Altair8800)


target_compile_options(${PROJECT_NAME} PRIVATE -Wno-unknown-pragmas)

set_source_files_properties(Altair8800/intel8080.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(Altair8800/intel8080.h PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(Altair8800/memory.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
# set_source_files_properties(comms_manager_wolf.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(FrontPanels/front_panel_virtual.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
# set_source_files_properties(wolfMQTT/examples/mqttnet.c PROPERTIES COMPILE_FLAGS -Wno-sign-conversion)
# set_source_files_properties(wolfMQTT/src/mqtt_client.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
# set_source_files_properties(wolfMQTT/src/mqtt_client.c PROPERTIES COMPILE_FLAGS -Wno-sign-conversion)
# set_source_files_properties(wolfMQTT/src/mqtt_packet.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
# set_source_files_properties(wolfMQTT/src/mqtt_packet.c PROPERTIES COMPILE_FLAGS -Wno-sign-conversion)
# set_source_files_properties(wolfMQTT/src/mqtt_socket.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
# set_source_files_properties(wolfMQTT/src/mqtt_socket.c PROPERTIES COMPILE_FLAGS -Wno-sign-conversion)
# set_source_files_properties(wolfMQTT/src/mqttnet.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
# set_source_files_properties(wolfMQTT/src/mqttnet.c PROPERTIES COMPILE_FLAGS -Wno-sign-conversion)
# set_source_files_properties(wolfMQTT/wolfmqtt/mqtt_client.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
# set_source_files_properties(wolfMQTT/wolfmqtt/mqtt_packet.h PROPERTIES COMPILE_FLAGS -Wno-conversion)


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)