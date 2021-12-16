# Install script for directory: /Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/deps/umock-c

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/build/desktop_devx/out/deps/umock-c/libumock_c.a")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libumock_c.a" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libumock_c.a")
    execute_process(COMMAND "/usr/bin/ranlib" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libumock_c.a")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/umock_c" TYPE FILE FILES
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/deps/umock-c/./inc/umock_c/umock_c.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/deps/umock-c/./inc/umock_c/umock_c_internal.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/deps/umock-c/./inc/umock_c/umock_c_negative_tests.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/deps/umock-c/./inc/umock_c/umock_c_prod.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/deps/umock-c/./inc/umock_c/umockalloc.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/deps/umock-c/./inc/umock_c/umockcall.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/deps/umock-c/./inc/umock_c/umockcallrecorder.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/deps/umock-c/./inc/umock_c/umocktypename.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/deps/umock-c/./inc/umock_c/umocktypes.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/deps/umock-c/./inc/umock_c/umocktypes_bool.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/deps/umock-c/./inc/umock_c/umocktypes_c.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/deps/umock-c/./inc/umock_c/umocktypes_stdint.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/deps/umock-c/./inc/umock_c/umocktypes_charptr.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/deps/umock-c/./inc/umock_c/umocktypes_struct.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/deps/umock-c/./inc/umock_c/umocktypes_wcharptr.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/deps/umock-c/./inc/umock_c/umockcallpairs.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/deps/umock-c/./inc/umock_c/umockstring.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/deps/umock-c/./inc/umock_c/umockautoignoreargs.h"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/deps/umock-c/./inc/umock_c/umock_log.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/umock_cTargets.cmake")
    file(DIFFERENT EXPORT_FILE_CHANGED FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/umock_cTargets.cmake"
         "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/build/desktop_devx/out/deps/umock-c/CMakeFiles/Export/cmake/umock_cTargets.cmake")
    if(EXPORT_FILE_CHANGED)
      file(GLOB OLD_CONFIG_FILES "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/umock_cTargets-*.cmake")
      if(OLD_CONFIG_FILES)
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/umock_cTargets.cmake\" will be replaced.  Removing files [${OLD_CONFIG_FILES}].")
        file(REMOVE ${OLD_CONFIG_FILES})
      endif()
    endif()
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/build/desktop_devx/out/deps/umock-c/CMakeFiles/Export/cmake/umock_cTargets.cmake")
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/build/desktop_devx/out/deps/umock-c/CMakeFiles/Export/cmake/umock_cTargets-debug.cmake")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/deps/umock-c/configs/umock_cFunctions.cmake"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/deps/umock-c/configs/umock_cConfig.cmake"
    "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/build/desktop_devx/out/deps/umock-c/umock_c/umock_cConfigVersion.cmake"
    )
endif()

