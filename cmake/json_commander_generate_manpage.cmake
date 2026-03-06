# json_commander_generate_manpage.cmake
#
# CMake -P script: runs an executable with --help-man and writes
# its stdout to a file. Invoked at build time via add_custom_command.
#
# Expected variables (passed via -D on the command line):
#   JCMD_EXECUTABLE  - path to the built executable
#   JCMD_OUTPUT_DIR  - directory for the output .1 file
#   JCMD_NAME        - base name for the man page file (e.g., "my-tool")
#   JCMD_SUBCOMMAND  - (optional) space-separated subcommand path
#   JCMD_SUBCMD_NAME - (optional) hyphenated subcommand name suffix

if(NOT JCMD_EXECUTABLE)
  message(FATAL_ERROR "JCMD_EXECUTABLE is required")
endif()
if(NOT JCMD_OUTPUT_DIR)
  message(FATAL_ERROR "JCMD_OUTPUT_DIR is required")
endif()
if(NOT JCMD_NAME)
  message(FATAL_ERROR "JCMD_NAME is required")
endif()

file(MAKE_DIRECTORY "${JCMD_OUTPUT_DIR}")

# Derive output filename
if(JCMD_SUBCMD_NAME)
  set(_output "${JCMD_OUTPUT_DIR}/${JCMD_NAME}-${JCMD_SUBCMD_NAME}.1")
else()
  set(_output "${JCMD_OUTPUT_DIR}/${JCMD_NAME}.1")
endif()

# Build the command: executable [subcmd1 subcmd2 ...] --help-man
set(_cmd "${JCMD_EXECUTABLE}")
if(JCMD_SUBCOMMAND)
  separate_arguments(_subcmds NATIVE_COMMAND "${JCMD_SUBCOMMAND}")
  list(APPEND _cmd ${_subcmds})
endif()
list(APPEND _cmd --help-man)

execute_process(
  COMMAND ${_cmd}
  OUTPUT_FILE "${_output}"
  RESULT_VARIABLE _rc)

if(NOT _rc EQUAL 0)
  string(REPLACE ";" " " _cmd_str "${_cmd}")
  message(FATAL_ERROR
    "Failed to generate man page: ${_cmd_str} returned ${_rc}")
endif()
