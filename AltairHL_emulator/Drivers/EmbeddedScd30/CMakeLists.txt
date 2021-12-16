CMAKE_MINIMUM_REQUIRED(VERSION 3.8)
PROJECT(scd30_lib C)

include_directories(
    "./scd30" 
    "./embedded-common"
    "./scd-common"
    )

################################################################################
# Source groups
################################################################################
set(Source
    "./scd30/scd30.c"
    "./embedded-common/sensirion_common.c"
    "./embedded-common/hw_i2c/sensirion_hw_i2c_implementation.c"
    "./scd30/scd30.h"
)
source_group("Source" FILES ${Source})

set(ALL_FILES
    ${Source}
)

################################################################################
# Target
################################################################################
add_library(${PROJECT_NAME} STATIC ${ALL_FILES})

SET(GCC_COVERAGE_COMPILE_FLAGS "-Wall -fstrict-aliasing -Wstrict-aliasing=1 -Wsign-conversion -fPIC")

set_source_files_properties( ./embedded-common/sensirion_common.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
add_definitions(${GCC_COVERAGE_COMPILE_FLAGS})

target_link_libraries (${PROJECT_NAME} applibs pthread gcc_s c)


if(AVNET)
    azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "HardwareDefinitions/avnet_mt3620_sk" TARGET_DEFINITION "co2_monitor.json")
endif(AVNET)

if(AVNET_REV_2)
    azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "HardwareDefinitions/avnet_mt3620_sk_rev2" TARGET_DEFINITION "co2_monitor.json")
endif(AVNET_REV_2)