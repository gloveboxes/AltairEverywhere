# Install script for directory: /Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/build/desktop_devx/out/libparson.a")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libparson.a" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libparson.a")
    execute_process(COMMAND "/usr/bin/ranlib" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libparson.a")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/DesktopDevX/azure-iot-sdk-c/deps/parson/parson.h")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/parson/parsonConfig.cmake")
    file(DIFFERENT EXPORT_FILE_CHANGED FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/parson/parsonConfig.cmake"
         "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/build/desktop_devx/out/CMakeFiles/Export/lib/cmake/parson/parsonConfig.cmake")
    if(EXPORT_FILE_CHANGED)
      file(GLOB OLD_CONFIG_FILES "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/parson/parsonConfig-*.cmake")
      if(OLD_CONFIG_FILES)
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/parson/parsonConfig.cmake\" will be replaced.  Removing files [${OLD_CONFIG_FILES}].")
        file(REMOVE ${OLD_CONFIG_FILES})
      endif()
    endif()
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/parson" TYPE FILE FILES "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/build/desktop_devx/out/CMakeFiles/Export/lib/cmake/parson/parsonConfig.cmake")
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/parson" TYPE FILE FILES "/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/build/desktop_devx/out/CMakeFiles/Export/lib/cmake/parson/parsonConfig-debug.cmake")
  endif()
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/build/desktop_devx/out/deps/azure-macro-utils-c/cmake_install.cmake")
  include("/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/build/desktop_devx/out/deps/umock-c/cmake_install.cmake")
  include("/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/build/desktop_devx/out/c-utility/cmake_install.cmake")
  include("/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/build/desktop_devx/out/umqtt/cmake_install.cmake")
  include("/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/build/desktop_devx/out/provisioning_client/cmake_install.cmake")
  include("/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/build/desktop_devx/out/iothub_client/cmake_install.cmake")
  include("/Users/dave/GitHub/AzureSphereAltair8800/AltairHL_emulator/build/desktop_devx/out/serializer/cmake_install.cmake")

endif()

