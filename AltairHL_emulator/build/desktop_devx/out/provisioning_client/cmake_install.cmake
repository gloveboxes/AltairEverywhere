# Install script for directory: /Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/azure_prov_client" TYPE FILE FILES
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/./adapters/hsm_client_data.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/./adapters/hsm_client_riot.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/./adapters/hsm_client_tpm.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/./adapters/hsm_client_key.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/./inc/azure_prov_client/internal/prov_auth_client.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/./inc/azure_prov_client/prov_security_factory.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/./inc/azure_prov_client/internal/iothub_auth_client.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/./inc/azure_prov_client/internal/prov_auth_client.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/./inc/azure_prov_client/iothub_security_factory.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/./inc/azure_prov_client/prov_client_const.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/./inc/azure_prov_client/prov_device_ll_client.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/./inc/azure_prov_client/prov_client_const.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/./inc/azure_prov_client/prov_device_client.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/./inc/azure_prov_client/prov_transport.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/./inc/azure_prov_client/prov_transport_mqtt_ws_client.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/./inc/azure_prov_client/internal/prov_transport_private.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/./inc/azure_prov_client/internal/prov_transport_mqtt_common.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/./inc/azure_prov_client/prov_transport.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/./inc/azure_prov_client/prov_transport_mqtt_client.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/./inc/azure_prov_client/internal/prov_transport_private.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/./inc/azure_prov_client/internal/prov_transport_mqtt_common.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/build/desktop_devx/out/provisioning_client/libhsm_security_client.a")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libhsm_security_client.a" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libhsm_security_client.a")
    execute_process(COMMAND "/usr/bin/ranlib" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libhsm_security_client.a")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/build/desktop_devx/out/provisioning_client/libprov_auth_client.a")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libprov_auth_client.a" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libprov_auth_client.a")
    execute_process(COMMAND "/usr/bin/ranlib" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libprov_auth_client.a")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/build/desktop_devx/out/provisioning_client/libprov_device_ll_client.a")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libprov_device_ll_client.a" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libprov_device_ll_client.a")
    execute_process(COMMAND "/usr/bin/ranlib" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libprov_device_ll_client.a")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/build/desktop_devx/out/provisioning_client/libprov_device_client.a")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libprov_device_client.a" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libprov_device_client.a")
    execute_process(COMMAND "/usr/bin/ranlib" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libprov_device_client.a")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/build/desktop_devx/out/provisioning_client/libprov_mqtt_ws_transport.a")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libprov_mqtt_ws_transport.a" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libprov_mqtt_ws_transport.a")
    execute_process(COMMAND "/usr/bin/ranlib" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libprov_mqtt_ws_transport.a")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/build/desktop_devx/out/provisioning_client/libprov_mqtt_transport.a")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libprov_mqtt_transport.a" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libprov_mqtt_transport.a")
    execute_process(COMMAND "/usr/bin/ranlib" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libprov_mqtt_transport.a")
  endif()
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/build/desktop_devx/out/provisioning_client/deps/cmake_install.cmake")
  include("/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/build/desktop_devx/out/provisioning_client/tests/cmake_install.cmake")

endif()

