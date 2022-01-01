CMAKE_MINIMUM_REQUIRED(VERSION 3.8)
PROJECT(avnet_drivers C)



################################################################################
# Source groups
################################################################################
set(Source
    "imu_temp_pressure.c"
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

target_include_directories(${PROJECT_NAME} PUBLIC
                           ../Common
                          )

# set(ROOT_NAMESPACE azsphere_libs)

#set_target_properties(${PROJECT_NAME} PROPERTIES
#    VS_GLOBAL_KEYWORD "AzureSphere"
#)

set_source_files_properties(../Common/lsm6dso_reg.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(../Common/lps22hh_reg.c PROPERTIES COMPILE_FLAGS -Wno-conversion)

set_source_files_properties(imu_temp_pressure.c PROPERTIES COMPILE_FLAGS -Wno-float-conversion)
set_source_files_properties(imu_temp_pressure.c PROPERTIES COMPILE_FLAGS -Wno-incompatible-pointer-types)
set_source_files_properties(imu_temp_pressure.c PROPERTIES COMPILE_FLAGS -Wno-conversion)

target_link_libraries (${PROJECT_NAME} applibs pthread gcc_s c )

# azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../HardwareDefinitions/avnet_mt3620_sk" TARGET_DEFINITION "azure_sphere_learning_path.json")