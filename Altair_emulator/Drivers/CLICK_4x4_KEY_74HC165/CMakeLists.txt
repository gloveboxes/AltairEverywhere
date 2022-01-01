CMAKE_MINIMUM_REQUIRED(VERSION 3.8)
PROJECT(74HC165_driver C)

################################################################################
# Source groups
################################################################################
set(Source
    "74HC165.c"
)
source_group("Source" FILES ${Source})

set(ALL_FILES
    ${Source}
)

################################################################################
# Target
################################################################################
add_library(${PROJECT_NAME} STATIC ${ALL_FILES})
target_link_libraries (${PROJECT_NAME} applibs gcc_s c )

target_include_directories(${PROJECT_NAME} PUBLIC "." )
