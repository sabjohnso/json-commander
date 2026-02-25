cmake_minimum_required(VERSION 3.12)

project(json-commander VERSION 0.3.0 LANGUAGES C CXX)

option(json_commander_BUILD_TESTING "Build the json-commander tests" ON)
option(json_commander_BUILD_EXAMPLES "Build the json-commander examples" ON)
option(json_commander_BUILD_TOOLS "Build the json-commander tools" ON)
option(json_commander_BUILD_C_API "Build the json-commander C API shared library" ON)
set(json_commander_CXX_STANDARD 20 CACHE STRING "C++ standard for json-commander")

include(GNUInstallDirs)
set(json_commander_INSTALL_BINDIR ${CMAKE_INSTALL_BINDIR})
set(json_commander_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR})
set(json_commander_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR})
set(json_commander_INSTALL_CONFDIR ${json_commander_INSTALL_LIBDIR}/cmake/json-commander-${PROJECT_VERSION})

if(json_commander_BUILD_C_API)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

list(INSERT CMAKE_MODULE_PATH 0 ${PROJECT_SOURCE_DIR}/cmake)
include(json_commander_deps)

set(json_commander_TEMPLATE_DIR "${PROJECT_SOURCE_DIR}/cmake")
include(json_commander_add_executable)

add_subdirectory(json_commander)

if(json_commander_BUILD_TESTING)
  include(CTest)
  if(BUILD_TESTING)
    add_subdirectory(json_commander_testing)
  endif()
endif()

if(json_commander_BUILD_C_API)
  add_subdirectory(json_commander_c)
  if(json_commander_BUILD_TESTING AND BUILD_TESTING)
    add_subdirectory(json_commander_c_testing)
  endif()
endif()

if(json_commander_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(json_commander_BUILD_TOOLS)
  add_subdirectory(tools)
endif()

install(EXPORT json_commander_EXPORTS
  NAMESPACE json_commander::
  FILE json-commander-exports.cmake
  DESTINATION ${json_commander_INSTALL_CONFDIR})

configure_file(json-commander-config.cmake.in json-commander-config.cmake)
install(FILES ${PROJECT_BINARY_DIR}/json-commander-config.cmake
  DESTINATION ${json_commander_INSTALL_CONFDIR})

install(FILES
  cmake/json_commander_add_executable.cmake
  cmake/json_commander_main.cpp.in
  DESTINATION ${json_commander_INSTALL_CONFDIR})
