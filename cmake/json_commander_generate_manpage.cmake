# json_commander_generate_manpage.cmake
#
# CMake -P script: runs an executable with --help-man and writes
# its stdout to a file. Invoked at build time via add_custom_command.
#
# Expected variables (passed via -D on the command line):
#   JCMD_EXECUTABLE  - path to the built executable
#   JCMD_OUTPUT      - path to the output .1 file
#   JCMD_SUBCOMMAND  - (optional) space-separated subcommand path

if(NOT JCMD_EXECUTABLE)
  message(FATAL_ERROR "JCMD_EXECUTABLE is required")
endif()
if(NOT JCMD_OUTPUT)
  message(FATAL_ERROR "JCMD_OUTPUT is required")
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
  OUTPUT_FILE "${JCMD_OUTPUT}"
  RESULT_VARIABLE _rc)

if(NOT _rc EQUAL 0)
  string(REPLACE ";" " " _cmd_str "${_cmd}")
  message(FATAL_ERROR
    "Failed to generate man page: ${_cmd_str} returned ${_rc}")
endif()
