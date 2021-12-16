CMAKE_MINIMUM_REQUIRED(VERSION 3.8)
PROJECT(MT3620_Grove_Shield_Library C)

################################################################################
# Source groups
################################################################################
set(no_group_source_files
    # "applibs_versions.h"
    # "Grove.h"
    # "mt3620_rdb.h"
)
source_group("" FILES ${no_group_source_files})

set(Common
    "Common/Delay.c"
)
source_group("Common" FILES ${Common})

set(HAL
    "HAL/GroveI2C.c"
    "HAL/GroveShield.c"
    "HAL/GroveUART.c"
)
source_group("HAL" FILES ${HAL})

set(Sensors
    "Sensors/Grove4DigitDisplay.c"
    "Sensors/GroveAD7992.c"
    "Sensors/GroveLEDButton.c"
    "Sensors/GroveLightSensor.c"
    "Sensors/GroveOledDisplay96x96.c"
    "Sensors/GroveRelay.c"
    "Sensors/GroveRotaryAngleSensor.c"
    "Sensors/GroveTempHumiBaroBME280.c"
    "Sensors/GroveTempHumiSHT31.c"
)
source_group("Sensors" FILES ${Sensors})

set(ALL_FILES
    ${no_group_source_files}
    ${Common}
    ${HAL}
    ${Sensors}
)

################################################################################
# Target
################################################################################
add_library(${PROJECT_NAME} STATIC ${ALL_FILES})


set(ROOT_NAMESPACE MT3620_Grove_Shield_Library)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_GLOBAL_KEYWORD "AzureSphere"
)


set_source_files_properties(Sensors/GroveOledDisplay96x96.c PROPERTIES COMPILE_FLAGS -Wno-conversion)

target_link_libraries (${PROJECT_NAME} applibs pthread gcc_s c)


azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../../HardwareDefinitions/mt3620_rdb" TARGET_DEFINITION "azure_sphere_learning_path.json")
