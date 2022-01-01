#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required (VERSION 3.10)

project (Retro8800Click C)

azsphere_configure_tools(TOOLS_REVISION "21.04")
azsphere_configure_api(TARGET_API_SET "9")

# Create executable
add_executable (${PROJECT_NAME} main.c)
target_link_libraries (${PROJECT_NAME} applibs gcc_s c)

add_subdirectory("../../CLICK_8800_RETRO_AS1115" as1115_driver)
include_directories(${PROJECT_NAME} ../../CLICK_8800_RETRO_AS1115)
target_link_libraries(${PROJECT_NAME} as1115_driver)
target_include_directories(${PROJECT_NAME} PUBLIC ../../CLICK_8800_RETRO_AS1115 )


azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../HardwareDefinitions/avnet_mt3620_sk" TARGET_DEFINITION "azure_sphere_learning_path.json")

azsphere_target_add_image_package(${PROJECT_NAME})
