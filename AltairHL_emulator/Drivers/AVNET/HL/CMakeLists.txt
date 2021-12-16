CMAKE_MINIMUM_REQUIRED(VERSION 3.8)
PROJECT(avnet_drivers C)

################################################################################
# Source groups
################################################################################
set(Source
    "imu_temp_pressure.c"
    "light_sensor.c"
    "../Common/lps22hh_reg.c"
    "../Common/lsm6dso_reg.c"
)
source_group("Source" FILES ${Source})

set(ALL_FILES
    ${Source}
)

################################################################################
# Target
################################################################################
add_library(${PROJECT_NAME} STATIC ${ALL_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC "../Common" ".")

set_source_files_properties(../Common/lsm6dso_reg.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(../Common/lps22hh_reg.c PROPERTIES COMPILE_FLAGS -Wno-conversion)

set_source_files_properties(imu_temp_pressure.c PROPERTIES COMPILE_FLAGS -Wno-float-conversion)
set_source_files_properties(imu_temp_pressure.c PROPERTIES COMPILE_FLAGS -Wno-incompatible-pointer-types)
set_source_files_properties(imu_temp_pressure.c PROPERTIES COMPILE_FLAGS -Wno-conversion)

target_link_libraries(${PROJECT_NAME} applibs gcc_s c )
