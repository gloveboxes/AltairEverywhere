#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required (VERSION 3.10)
project (Altair_emulator C)

include(cmake/altair_config.cmake)


# MQTT BROKER CONFIG ##############################################################################################
#
add_compile_definitions(ALTAIR_MQTT_SECURE_PORT_WOLF=1883)
add_compile_definitions(MQTT_CA_CERTIFICATE="certs/ca.crt")
add_compile_definitions(MQTT_CLIENT_PRIVATE_KEY="certs/client.key")
add_compile_definitions(MQTT_CLIENT_CERTIFICATE="certs/client.crt")

add_compile_definitions(MQTT_WOLF_DRIVER)
add_compile_definitions(PRINTF=Log_Debug)
#
###################################################################################################################

# wolfMQTT Settings ###############################################################################################
#
add_compile_definitions(WOLFMQTT_MULTITHREAD)
add_compile_definitions(WOLFMQTT_DISCONNECT_CB)
add_compile_definitions(WOLFMQTT_NONBLOCK)
# add_compile_definitions(WOLFMQTT_NO_STDIO)
add_compile_definitions(WOLFSSL_USER_SETTINGS)
# add_compile_definitions(WOLFMQTT_V5)
# add_compile_definitions(__linux__)
# add_compile_definitions(MQTT_USE_WOLFSSL)
# add_compile_definitions(ENABLE_MQTT_TLS)
add_compile_definitions(NO_DH)
# add_compile_definitions(AZURE_SPHERE)
add_compile_definitions(WOLFMQTT_USER_SETTINGS)


###################################################################################################################
# IoT Hub Polling time overide
add_compile_definitions(IOT_HUB_POLL_TIME_SECONDS=1)
add_compile_definitions(IOT_HUB_POLL_TIME_NANOSECONDS=500000000)
###################################################################################################################

if(AVNET OR AVNET_REV_2)
    add_subdirectory("Drivers/AVNET/HL" drivers)
endif(AVNET OR AVNET_REV_2)

if (MICRO_SD_CLICK)
    add_compile_definitions(SD_CARD_ENABLED)
endif (MICRO_SD_CLICK)

if (ALTAIR_FRONT_PANEL_KIT)

    add_compile_definitions(ALTAIR_FRONT_PANEL_KIT)

    set(Oem
        "FrontPanels/front_panel_kit.c"
    )
    source_group("Oem" FILES ${Oem})
endif(ALTAIR_FRONT_PANEL_KIT)

if (ALTAIR_FRONT_PANEL_CLICK)

    add_compile_definitions(ALTAIR_FRONT_PANEL_CLICK)

    set(Oem
        "FrontPanels/front_panel_click.c"
    )
    source_group("Oem" FILES ${Oem})
endif(ALTAIR_FRONT_PANEL_CLICK)

if (ALTAIR_FRONT_PANEL_RETRO_CLICK)

    add_compile_definitions(ALTAIR_FRONT_PANEL_RETRO_CLICK)

    set(Oem
        "FrontPanels/front_panel_retro_click.c"
    )
    source_group("Oem" FILES ${Oem})
endif(ALTAIR_FRONT_PANEL_RETRO_CLICK)

if (ALTAIR_FRONT_PANEL_NONE)

    add_compile_definitions(ALTAIR_FRONT_PANEL_NONE)

    set(Oem
        "FrontPanels/front_panel_none.c"
    )
    source_group("Oem" FILES ${Oem})
endif(ALTAIR_FRONT_PANEL_NONE)


set(Source
    "Altair8800/88dcdd.c"
    "Altair8800/intel8080.c"
    "Altair8800/memory.c"
    "comms_manager_wolf.c"
    "FrontPanels/front_panel_virtual.c"
    "iotc_manager.c"
    "location_from_ip.c"
    "main.c"
    "onboard_sensors.c"
    "storage.c"
    "utils.c"
    # "weather.c"
)
source_group("Source" FILES ${Source})

set(wolfMQTT
    "wolfMQTT/src/mqtt_client.c"
    "wolfMQTT/src/mqtt_packet.c"
    "wolfMQTT/src/mqtt_socket.c"
    "wolfMQTT/examples/mqttnet.c"
    "wolfMQTT/examples/mqttexample.c"
)
source_group("wolfMQTT" FILES ${wolfMQTT})


set(ALL_FILES
    ${Source}
    ${Oem}
    ${wolfMQTT}
)

# Create executable
add_executable(${PROJECT_NAME} ${ALL_FILES})

################################################################################
# Following lines required for DesktopDevX
add_subdirectory("DesktopDevX" desktop_devx)
target_link_libraries(${PROJECT_NAME} "desktop_devx")
################################################################################

# ################################################################################
# # The following line required for the AzureSphereRemoteX.Client library
add_subdirectory("AzureSphereRemoteX.Client" applibs)
target_link_libraries(${PROJECT_NAME} applibs)
# ################################################################################

target_include_directories(${PROJECT_NAME} PUBLIC /usr/local/include)

target_compile_definitions(${PROJECT_NAME} PUBLIC AZURE_IOT_HUB_CONFIGURED)
target_link_libraries(${PROJECT_NAME} applibs pthread c desktop_devx wolfssl curl)

target_link_options(${PROJECT_NAME} PUBLIC "-L/usr/local/lib")


target_include_directories(${PROJECT_NAME} PUBLIC AzureSphereDevX/include )
target_include_directories(${PROJECT_NAME} PUBLIC ../IntercoreContract)
target_include_directories(${PROJECT_NAME} PUBLIC wolfMQTT wolfMQTT/wolfmqtt ./ )
include_directories(${CMAKE_SOURCE_DIR} FrontPanels)
include_directories(${CMAKE_SOURCE_DIR} Altair8800)

if(AVNET OR AVNET_REV_2)
    target_link_libraries (${PROJECT_NAME} avnet_drivers)
    target_include_directories(${PROJECT_NAME} PUBLIC Drivers/AVNET/HL)
endif(AVNET OR AVNET_REV_2)


target_compile_options(${PROJECT_NAME} PRIVATE -Wno-unknown-pragmas)

set_source_files_properties(Altair8800/intel8080.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(Altair8800/intel8080.h PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(Altair8800/memory.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(comms_manager.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(FrontPanels/front_panel_virtual.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(wolfMQTT/examples/mqttnet.c PROPERTIES COMPILE_FLAGS -Wno-sign-conversion)
set_source_files_properties(wolfMQTT/src/mqtt_client.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(wolfMQTT/src/mqtt_client.c PROPERTIES COMPILE_FLAGS -Wno-sign-conversion)
set_source_files_properties(wolfMQTT/src/mqtt_packet.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(wolfMQTT/src/mqtt_packet.c PROPERTIES COMPILE_FLAGS -Wno-sign-conversion)
set_source_files_properties(wolfMQTT/src/mqtt_socket.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(wolfMQTT/src/mqtt_socket.c PROPERTIES COMPILE_FLAGS -Wno-sign-conversion)
set_source_files_properties(wolfMQTT/src/mqttnet.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(wolfMQTT/src/mqttnet.c PROPERTIES COMPILE_FLAGS -Wno-sign-conversion)
set_source_files_properties(wolfMQTT/wolfmqtt/mqtt_client.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(wolfMQTT/wolfmqtt/mqtt_packet.h PROPERTIES COMPILE_FLAGS -Wno-conversion)

# Set hardware definitons

set(BOARD_COUNTER 0)

if(AVNET)
    MATH(EXPR BOARD_COUNTER "${BOARD_COUNTER}+1")
    add_definitions( -DOEM_AVNET=TRUE )
    target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_LIST_DIR}/HardwareDefinitions/avnet_mt3620_sk/inc/")
    message(STATUS "Azure Sphere board selected: AVNET REV 1")
endif(AVNET)

if(AVNET_REV_2)
    MATH(EXPR BOARD_COUNTER "${BOARD_COUNTER}+1")
    add_definitions( -DOEM_AVNET=TRUE )
    target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_LIST_DIR}/HardwareDefinitions/avnet_mt3620_sk_rev2/inc/")
    message(STATUS "Azure Sphere board selected: AVNET REV 2")
endif(AVNET_REV_2)

if (SEEED_STUDIO_RDB)
    MATH(EXPR BOARD_COUNTER "${BOARD_COUNTER}+1")
    add_definitions( -DOEM_SEEED_STUDIO=TRUE )
    target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_LIST_DIR}/HardwareDefinitions/mt3620_rdb/inc/")
    message(STATUS "Azure Sphere board selected: SEEED STUDIO RDB")
endif()

if (SEEED_STUDIO_MINI)
    MATH(EXPR BOARD_COUNTER "${BOARD_COUNTER}+1")
    add_definitions( -DOEM_SEEED_STUDIO_MINI=TRUE )    
    target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_LIST_DIR}/HardwareDefinitions/seeed_mt3620_mdb/inc/")
    message(STATUS "Azure Sphere board selected: SEEED STUDIO MINI")
endif(SEEED_STUDIO_MINI)

if(BOARD_COUNTER EQUAL 0)
    message(FATAL_ERROR "No Azure Sphere boards selected. Ensure one board set")
endif()

if(BOARD_COUNTER GREATER 1)
    message(FATAL_ERROR "Multiple (${BOARD_COUNTER}) Azure Sphere boards selected. Ensure only one board set")
endif()