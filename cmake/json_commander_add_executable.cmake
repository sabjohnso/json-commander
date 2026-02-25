function(json_commander_add_executable name)
  cmake_parse_arguments(JCMD
    "WIN32;MACOSX_BUNDLE;EXCLUDE_FROM_ALL"
    "SCHEMA;MAIN;FROM_HEADER"
    ""
    ${ARGN})

  if(NOT JCMD_SCHEMA)
    message(FATAL_ERROR "json_commander_add_executable: SCHEMA is required")
  endif()
  if(NOT JCMD_MAIN)
    message(FATAL_ERROR "json_commander_add_executable: MAIN is required")
  endif()

  # Build the FROM_HEADER include line
  if(JCMD_FROM_HEADER)
    set(JCMD_FROM_HEADER_INCLUDE "#include \"${JCMD_FROM_HEADER}\"")
  else()
    set(JCMD_FROM_HEADER_INCLUDE "")
  endif()

  set(JCMD_MAIN_FN "${JCMD_MAIN}")

  # Generate main.cpp via configure_file
  set(_generated_main "${CMAKE_CURRENT_BINARY_DIR}/${name}_jcmd_main.cpp")
  configure_file(
    "${json_commander_TEMPLATE_DIR}/json_commander_main.cpp.in"
    "${_generated_main}"
    @ONLY)

  # Build add_executable flags
  set(_exe_flags)
  if(JCMD_WIN32)
    list(APPEND _exe_flags WIN32)
  endif()
  if(JCMD_MACOSX_BUNDLE)
    list(APPEND _exe_flags MACOSX_BUNDLE)
  endif()
  if(JCMD_EXCLUDE_FROM_ALL)
    list(APPEND _exe_flags EXCLUDE_FROM_ALL)
  endif()

  # Create the executable (forward unparsed args as additional sources)
  add_executable(${name} ${_exe_flags} ${_generated_main} ${JCMD_UNPARSED_ARGUMENTS})

  # Link json-commander dependencies
  target_link_libraries(${name} PRIVATE
    json_commander::header
    json_commander::library
    nlohmann_json::nlohmann_json
    nlohmann_json_schema_validator)

  # Schema path as compile definition
  get_filename_component(_schema_abs "${JCMD_SCHEMA}" ABSOLUTE)
  target_compile_definitions(${name} PRIVATE
    JCMD_SCHEMA="${_schema_abs}")

  # C++ standard
  set_target_properties(${name} PROPERTIES
    CXX_STANDARD ${json_commander_CXX_STANDARD})

  # Ensure FROM_HEADER can be found relative to the caller's source dir
  target_include_directories(${name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
endfunction()
