CMAKE_MINIMUM_REQUIRED(VERSION 3.8)
PROJECT(max7219_driver C)

################################################################################
# Source groups
################################################################################
set(Source
    "max7219.c"
    "graphics.c"
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

set_source_files_properties(graphics.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
target_include_directories(${PROJECT_NAME} PUBLIC "." )