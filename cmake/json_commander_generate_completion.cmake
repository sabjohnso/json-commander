# json_commander_generate_completion.cmake
#
# CMake -P script: runs an executable with --help-completion <shell> and writes
# its stdout to a file. Invoked at build time via add_custom_command.
#
# Expected variables (passed via -D on the command line):
#   JCMD_EXECUTABLE  - path to the built executable
#   JCMD_SHELL       - shell name (bash, zsh, or fish)
#   JCMD_OUTPUT_DIR  - directory for the output completion file
#   JCMD_NAME        - base name for the completion file (e.g., "my-tool")

if(NOT JCMD_EXECUTABLE)
  message(FATAL_ERROR "JCMD_EXECUTABLE is required")
endif()
if(NOT JCMD_SHELL)
  message(FATAL_ERROR "JCMD_SHELL is required")
endif()
if(NOT JCMD_OUTPUT_DIR)
  message(FATAL_ERROR "JCMD_OUTPUT_DIR is required")
endif()
if(NOT JCMD_NAME)
  message(FATAL_ERROR "JCMD_NAME is required")
endif()

file(MAKE_DIRECTORY "${JCMD_OUTPUT_DIR}")

# Derive output filename based on shell convention
if(JCMD_SHELL STREQUAL "bash")
  set(_output "${JCMD_OUTPUT_DIR}/${JCMD_NAME}.bash")
elseif(JCMD_SHELL STREQUAL "zsh")
  set(_output "${JCMD_OUTPUT_DIR}/_${JCMD_NAME}")
elseif(JCMD_SHELL STREQUAL "fish")
  set(_output "${JCMD_OUTPUT_DIR}/${JCMD_NAME}.fish")
else()
  message(FATAL_ERROR "Unsupported shell: ${JCMD_SHELL}")
endif()

execute_process(
  COMMAND "${JCMD_EXECUTABLE}" --help-completion "${JCMD_SHELL}"
  OUTPUT_FILE "${_output}"
  RESULT_VARIABLE _rc)

if(NOT _rc EQUAL 0)
  message(FATAL_ERROR
    "Failed to generate ${JCMD_SHELL} completion: ${JCMD_EXECUTABLE} --help-completion ${JCMD_SHELL} returned ${_rc}")
endif()
