CMAKE_MINIMUM_REQUIRED(VERSION 3.8)
PROJECT(eeprom2_driver C)

################################################################################
# Source groups
################################################################################
set(Source
    "eeprom2.c"
    "eeprom2.h"
)
source_group("Source" FILES ${Source})

set(ALL_FILES
    ${Source}
)

################################################################################
# Target
################################################################################
add_library(${PROJECT_NAME} STATIC ${ALL_FILES})
target_link_libraries (${PROJECT_NAME} gcc_s applibs c)

target_include_directories(${PROJECT_NAME} PUBLIC "." )
