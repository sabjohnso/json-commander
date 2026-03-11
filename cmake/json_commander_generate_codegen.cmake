# json_commander_generate_codegen.cmake
#
# Run json-commander codegen to generate a C++ model header.
#
# Required variables:
#   JCMD_EXECUTABLE   - path to json-commander binary
#   JCMD_SCHEMA_FILE  - path to the input JSON schema
#   JCMD_OUTPUT_FILE  - path to the output C++ header
#   JCMD_FUNCTION_NAME - name of the generated function

if(NOT JCMD_EXECUTABLE)
  message(FATAL_ERROR "JCMD_EXECUTABLE is required")
endif()
if(NOT JCMD_SCHEMA_FILE)
  message(FATAL_ERROR "JCMD_SCHEMA_FILE is required")
endif()
if(NOT JCMD_OUTPUT_FILE)
  message(FATAL_ERROR "JCMD_OUTPUT_FILE is required")
endif()
if(NOT JCMD_FUNCTION_NAME)
  set(JCMD_FUNCTION_NAME "jcmd_make_root")
endif()

execute_process(
  COMMAND "${JCMD_EXECUTABLE}" codegen "${JCMD_SCHEMA_FILE}"
    --function-name "${JCMD_FUNCTION_NAME}"
  OUTPUT_FILE "${JCMD_OUTPUT_FILE}"
  ERROR_VARIABLE _err
  RESULT_VARIABLE _rc)

if(NOT _rc EQUAL 0)
  message(FATAL_ERROR
    "Failed to generate model header:\n"
    "${JCMD_EXECUTABLE} codegen ${JCMD_SCHEMA_FILE} returned ${_rc}\n"
    "${_err}")
endif()
