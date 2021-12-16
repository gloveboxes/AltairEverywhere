# Install script for directory: /Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/deps

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/build/desktop_devx/out/provisioning_client/deps/libmsr_riot.a")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libmsr_riot.a" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libmsr_riot.a")
    execute_process(COMMAND "/usr/bin/ranlib" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libmsr_riot.a")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/azureiot/msr_riot" TYPE FILE FILES
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/deps/RIoT/Simulation/DICE/DiceSha256.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/deps/RIoT/Simulation/RIoT/Core/RIoT.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/deps/RIoT/Simulation/RIoT/Core/RiotStatus.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/deps/RIoT/Simulation/RIoT/Core/RIoTCrypt/include/RiotCrypt.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/deps/RIoT/Simulation/RIoT/Core/RIoTCrypt/include/base64.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/deps/RIoT/Simulation/RIoT/Core/RIoTCrypt/include/derenc.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/deps/RIoT/Simulation/RIoT/Core/RIoTCrypt/include/x509bldr.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/deps/RIoT/Simulation/RIoT/Core/RIoTCrypt/include/mbedtls/aes.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/deps/RIoT/Simulation/RIoT/Core/RIoTCrypt/include/mbedtls/ecdh.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/deps/RIoT/Simulation/RIoT/Core/RIoTCrypt/include/mbedtls/ecdsa.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/deps/RIoT/Simulation/RIoT/Core/RIoTCrypt/include/mbedtls/ecp.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/deps/RIoT/Simulation/RIoT/Core/RIoTCrypt/include/mbedtls/bignum.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/deps/RIoT/Simulation/RIoT/Core/RIoTCrypt/include/mbedtls/hkdf.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/deps/RIoT/Simulation/RIoT/Core/RIoTCrypt/include/mbedtls/hmac_drbg.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/deps/RIoT/Simulation/RIoT/Core/RIoTCrypt/include/mbedtls/md.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/deps/RIoT/Simulation/RIoT/Core/RIoTCrypt/include/mbedtls/platform.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/deps/RIoT/Simulation/RIoT/Core/RIoTCrypt/include/mbedtls/platform_util.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/deps/RIoT/Simulation/RIoT/Core/RIoTCrypt/include/mbedtls/sha1.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/provisioning_client/deps/RIoT/Simulation/RIoT/Core/RIoTCrypt/include/mbedtls/sha256.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/azure_iot_sdksTargets.cmake")
    file(DIFFERENT EXPORT_FILE_CHANGED FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/azure_iot_sdksTargets.cmake"
         "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/build/desktop_devx/out/provisioning_client/deps/CMakeFiles/Export/cmake/azure_iot_sdksTargets.cmake")
    if(EXPORT_FILE_CHANGED)
      file(GLOB OLD_CONFIG_FILES "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/azure_iot_sdksTargets-*.cmake")
      if(OLD_CONFIG_FILES)
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/azure_iot_sdksTargets.cmake\" will be replaced.  Removing files [${OLD_CONFIG_FILES}].")
        file(REMOVE ${OLD_CONFIG_FILES})
      endif()
    endif()
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/build/desktop_devx/out/provisioning_client/deps/CMakeFiles/Export/cmake/azure_iot_sdksTargets.cmake")
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/build/desktop_devx/out/provisioning_client/deps/CMakeFiles/Export/cmake/azure_iot_sdksTargets-debug.cmake")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/build/desktop_devx/out/provisioning_client/deps/azure_iot_sdks/azure_iot_sdksConfigVersion.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/build/desktop_devx/out/provisioning_client/deps/utpm/cmake_install.cmake")

endif()

