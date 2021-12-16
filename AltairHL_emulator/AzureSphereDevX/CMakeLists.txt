CMAKE_MINIMUM_REQUIRED(VERSION 3.8)
PROJECT(azure_sphere_devx C)

################################################################################
# Source groups
################################################################################
set(Source
    "./src/dx_azure_iot.c"
    "./src/dx_config.c"
    "./src/dx_device_twins.c"
    "./src/dx_direct_methods.c"
    "./src/eventloop_timer_utilities.c"
    "./src/dx_intercore.c"
    "./src/parson.c"
    "./src/dx_gpio.c"
    "./src/dx_i2c.c"
    "./src/dx_pwm.c"
    "./src/dx_terminate.c"
    "./src/dx_timer.c"
    "./src/dx_utilities.c"
    "./src/dx_json_serializer.c"
    "./src/dx_deferred_update.c"	
    "./src/dx_avnet_iot_connect.c"	
    "./src/dx_uart.c"
)
source_group("Source" FILES ${Source})

set(ALL_FILES
    ${Source}
)

################################################################################
# Target
################################################################################
add_library(${PROJECT_NAME} STATIC ${ALL_FILES})


set(ROOT_NAMESPACE azsphere_libs)
target_include_directories(${PROJECT_NAME} PUBLIC ${AZURE_SPHERE_API_SET_DIR}/usr/include/azureiot intercore_contract ./include)
set_target_properties(${PROJECT_NAME} PROPERTIES VS_GLOBAL_KEYWORD "AzureSphere"
)

target_link_libraries (${PROJECT_NAME} applibs pthread gcc_s c azureiot)
