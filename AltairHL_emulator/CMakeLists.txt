#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required (VERSION 3.10)
project (Altair_emulator C)

include(altair_config.cmake)


add_compile_definitions(PRINTF=Log_Debug)


# wolfMQTT Settings ###############################################################################################
#
add_compile_definitions(WOLFMQTT_MULTITHREAD)
add_compile_definitions(WOLFMQTT_DISCONNECT_CB)
add_compile_definitions(WOLFMQTT_NONBLOCK)
add_compile_definitions(WOLFSSL_USER_SETTINGS)
# add_compile_definitions(WOLFMQTT_V5)
add_compile_definitions(NO_DH)
add_compile_definitions(WOLFMQTT_USER_SETTINGS)
###################################################################################################################


set(Source
    "Altair8800/88dcdd.c"
    "Altair8800/intel8080.c"
    "Altair8800/memory.c"
    "comms_manager_wolf.c"
    "FrontPanels/front_panel_virtual.c"
    "iotc_manager.c"
    "location_from_ip.c"
    "main.c"
    # "onboard_sensors.c"
    "storage.c"
    "utils.c"
    "weather.c"
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


