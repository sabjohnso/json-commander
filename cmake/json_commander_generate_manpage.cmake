# json_commander_generate_manpage.cmake
#
# CMake -P script: runs an executable with --help-man and writes
# its stdout to a file. Invoked at build time via add_custom_command.
#
# Expected variables (passed via -D on the command line):
#   JCMD_EXECUTABLE  - path to the built executable
#   JCMD_OUTPUT      - path to the output .1 file

if(NOT JCMD_EXECUTABLE)
  message(FATAL_ERROR "JCMD_EXECUTABLE is required")
endif()
if(NOT JCMD_OUTPUT)
  message(FATAL_ERROR "JCMD_OUTPUT is required")
endif()

execute_process(
  COMMAND "${JCMD_EXECUTABLE}" --help-man
  OUTPUT_FILE "${JCMD_OUTPUT}"
  RESULT_VARIABLE _rc)

if(NOT _rc EQUAL 0)
  message(FATAL_ERROR
    "Failed to generate man page: ${JCMD_EXECUTABLE} --help-man returned ${_rc}")
endif()
