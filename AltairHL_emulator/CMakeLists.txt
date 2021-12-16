#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required (VERSION 3.10)
project (AltairHL_emulator C)

get_filename_component(PARENT_DIR ${PROJECT_SOURCE_DIR} DIRECTORY)

include(cmake/altair_config.cmake)

if (EXISTS "${PARENT_DIR}/Tools/cmake/azsphere_config.cmake")
    include(../Tools/cmake/azsphere_config.cmake)
    auto_generate_azsphere_config()
else()

    # For information on setting tools revision and target api set see 
    # https://docs.microsoft.com/en-us/azure-sphere/app-development/using-cmake-functions

    azsphere_configure_tools(TOOLS_REVISION "21.07")
    azsphere_configure_api(TARGET_API_SET "10")

endif()

# MQTT BROKER CONFIG ##############################################################################################
#
add_compile_definitions(ALTAIR_MQTT_SECURE_PORT_WOLF=8884)
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
# add_compile_definitions(WOLFMQTT_NONBLOCK)
add_compile_definitions(WOLFMQTT_NO_STDIO)
add_compile_definitions(WOLFSSL_USER_SETTINGS)
# add_compile_definitions(WOLFMQTT_V5)
add_compile_definitions(__linux__)
add_compile_definitions(MQTT_USE_WOLFSSL)
add_compile_definitions(ENABLE_MQTT_TLS)
add_compile_definitions(NO_DH)
add_compile_definitions(AZURE_SPHERE)
add_compile_definitions(WOLFMQTT_USER_SETTINGS)


###################################################################################################################
# IoT Hub Polling time overide
add_compile_definitions(IOT_HUB_POLL_TIME_SECONDS=1)
add_compile_definitions(IOT_HUB_POLL_TIME_NANOSECONDS=500000000)
###################################################################################################################


add_subdirectory("AzureSphereDevX" out)

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
    "weather.c"
)
source_group("Source" FILES ${Source})

set(wolfMQTT
    "wolfMQTT/src/mqtt_client.c"
    "wolfMQTT/src/mqtt_packet.c"
    "wolfMQTT/src/mqtt_socket.c"
    "wolfMQTT/examples/mqttnet.c"
)
source_group("wolfMQTT" FILES ${wolfMQTT})


set(ALL_FILES
    ${Source}
    ${Oem}
    ${wolfMQTT}
)

# Create executable
add_executable(${PROJECT_NAME} ${ALL_FILES})

target_compile_definitions(${PROJECT_NAME} PUBLIC AZURE_IOT_HUB_CONFIGURED)
target_link_libraries(${PROJECT_NAME} applibs pthread gcc_s c azure_sphere_devx wolfssl tlsutils curl)
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


if(ALTAIR_FRONT_PANEL_CLICK)

add_subdirectory("Drivers/CLICK_8x8_LED_MAX7219" max7219_driver)
include_directories(${CMAKE_SOURCE_DIR} Drivers/CLICK_8x8_LED_MAX7219)
target_link_libraries(${PROJECT_NAME} max7219_driver)
target_include_directories(${PROJECT_NAME} PUBLIC Drivers/CLICK_8x8_LED_MAX7219 )

add_subdirectory("Drivers/CLICK_4x4_KEY_74HC165" 74HC165_driver)
include_directories(${CMAKE_SOURCE_DIR} Drivers/CLICK_4x4_KEY_74HC165)
target_link_libraries(${PROJECT_NAME} 74HC165_driver)
target_include_directories(${PROJECT_NAME} PUBLIC Drivers/CLICK_4x4_KEY_74HC165 )

endif(ALTAIR_FRONT_PANEL_CLICK)


if(ALTAIR_FRONT_PANEL_RETRO_CLICK)

add_subdirectory("Drivers/CLICK_8800_RETRO_AS1115" as1115_driver)
include_directories(${PROJECT_NAME} Drivers/CLICK_8800_RETRO_AS1115)
target_link_libraries(${PROJECT_NAME} as1115_driver)
target_include_directories(${PROJECT_NAME} PUBLIC Drivers/CLICK_8800_RETRO_AS1115 )

endif(ALTAIR_FRONT_PANEL_RETRO_CLICK)

if(AVNET)    
    add_compile_definitions(OEM_AVNET)
    azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../HardwareDefinitions/avnet_mt3620_sk" TARGET_DEFINITION "azure_sphere_learning_path.json")
endif(AVNET)

if(AVNET_REV_2)
    add_compile_definitions(OEM_AVNET)
    azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../HardwareDefinitions/avnet_mt3620_sk_rev2" TARGET_DEFINITION "azure_sphere_learning_path.json")
endif(AVNET_REV_2)

if (SEEED_STUDIO_RDB)   
    add_compile_definitions(OEM_SEEED_STUDIO)
    azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../HardwareDefinitions/mt3620_rdb" TARGET_DEFINITION "azure_sphere_learning_path.json")
endif()


# If MICRO_SD_CLICK enabled then don't deploy the "Disks/cpm63k.dsk" disk image as CP/M will be booted from microSD Card. Reduces deploymemt time.
if (MICRO_SD_CLICK)
    azsphere_target_add_image_package(${PROJECT_NAME} RESOURCE_FILES  "certs/ca.crt" "certs/client.key" "certs/client.crt" "BasicSamples/STARTREK.BAS" "BasicSamples/TICTACTOE.BAS" "BasicSamples/SIMPLE.BAS" "BasicSamples/LOOPY.BAS" "BasicSamples/WEATHER.BAS" "BasicSamples/IOT.BAS" "Disks/88dskrom.bin")
else (MICRO_SD_CLICK)
    azsphere_target_add_image_package(${PROJECT_NAME} RESOURCE_FILES  "certs/ca.crt" "certs/client.key" "certs/client.crt" "BasicSamples/STARTREK.BAS" "BasicSamples/TICTACTOE.BAS" "BasicSamples/SIMPLE.BAS" "BasicSamples/LOOPY.BAS" "BasicSamples/WEATHER.BAS" "BasicSamples/IOT.BAS" "Disks/88dskrom.bin" "Disks/cpm63k.dsk")
endif (MICRO_SD_CLICK)
