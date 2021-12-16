#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "msr_riot" for configuration "Debug"
set_property(TARGET msr_riot APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(msr_riot PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libmsr_riot.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS msr_riot )
list(APPEND _IMPORT_CHECK_FILES_FOR_msr_riot "${_IMPORT_PREFIX}/lib/libmsr_riot.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
