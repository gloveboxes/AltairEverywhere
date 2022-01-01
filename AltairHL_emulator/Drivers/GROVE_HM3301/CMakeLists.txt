CMAKE_MINIMUM_REQUIRED(VERSION 3.8)
PROJECT(hm330x_driver C)

################################################################################
# Source groups
################################################################################
set(Source
    "hm330x.c"
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