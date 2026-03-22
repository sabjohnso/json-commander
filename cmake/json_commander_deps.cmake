if(NOT EXISTS ${PROJECT_SOURCE_DIR}/cmake_utilities/FindCMakeUtilities.cmake)
  find_package(Git REQUIRED)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
endif()
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake_utilities)
find_package(CMakeUtilities)

include(FetchContent)

find_package(CTestDeps REQUIRED)

# Pin dependency versions internally so downstream consumers
# don't need to set these in their own presets.
if(NOT DEFINED nlohmann_json_GIT_TAG)
  set(nlohmann_json_GIT_TAG "v3.12.0")
endif()

if(NOT DEFINED nlohmann_json_schema_validator_GIT_TAG)
  set(nlohmann_json_schema_validator_GIT_TAG "2.4.0")
endif()

# Suppress tests and examples from dependencies.
set(JSON_VALIDATOR_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(JSON_VALIDATOR_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# Enable install rules so dependencies are installed alongside json-commander.
set(JSON_Install ON CACHE BOOL "" FORCE)
set(JSON_VALIDATOR_INSTALL ON CACHE BOOL "" FORCE)

# Fetch dependencies directly (without EXCLUDE_FROM_ALL) so their
# install rules are included in our install target.
if(NOT nlohmann_json_FOUND)
  FetchContent_Declare(nlohmann_json
    SYSTEM
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG "${nlohmann_json_GIT_TAG}")
  FetchContent_MakeAvailable(nlohmann_json)
  set(nlohmann_json_FOUND TRUE)
endif()

if(NOT nlohmann_json_schema_validator_FOUND)
  FetchContent_Declare(nlohmann_json_schema_validator
    SYSTEM
    GIT_REPOSITORY https://github.com/pboettch/json-schema-validator.git
    GIT_TAG "${nlohmann_json_schema_validator_GIT_TAG}")
  FetchContent_MakeAvailable(nlohmann_json_schema_validator)
  set(nlohmann_json_schema_validator_FOUND TRUE)
endif()
