# Recursively collect subcommand paths from a JSON schema string.
# Populates two parallel lists (by name suffix) in PARENT_SCOPE:
#   ${out_names}  - hyphen-joined display names for filenames (e.g., "config-schema")
#   ${out_args}   - space-separated argument strings for the executable (e.g., "config-schema")
# The prefix_name and prefix_args track the current nesting path.
function(_jcmd_collect_subcommands json_content prefix_name prefix_args out_names out_args)
  set(_names "${${out_names}}")
  set(_args "${${out_args}}")

  # Check if "commands" key exists
  string(JSON _commands_type ERROR_VARIABLE _err TYPE "${json_content}" "commands")
  if(_err OR NOT _commands_type STREQUAL "ARRAY")
    set(${out_names} "${_names}" PARENT_SCOPE)
    set(${out_args} "${_args}" PARENT_SCOPE)
    return()
  endif()

  string(JSON _count LENGTH "${json_content}" "commands")
  if(_count EQUAL 0)
    set(${out_names} "${_names}" PARENT_SCOPE)
    set(${out_args} "${_args}" PARENT_SCOPE)
    return()
  endif()

  math(EXPR _last "${_count} - 1")
  foreach(_i RANGE 0 ${_last})
    string(JSON _cmd_name GET "${json_content}" "commands" ${_i} "name")
    if(prefix_name)
      set(_cur_name "${prefix_name}-${_cmd_name}")
      set(_cur_args "${prefix_args} ${_cmd_name}")
    else()
      set(_cur_name "${_cmd_name}")
      set(_cur_args "${_cmd_name}")
    endif()
    list(APPEND _names "${_cur_name}")
    list(APPEND _args "${_cur_args}")

    # Recurse into nested commands
    string(JSON _subcmd GET "${json_content}" "commands" ${_i})
    _jcmd_collect_subcommands("${_subcmd}" "${_cur_name}" "${_cur_args}" _names _args)
  endforeach()

  set(${out_names} "${_names}" PARENT_SCOPE)
  set(${out_args} "${_args}" PARENT_SCOPE)
endfunction()

function(json_commander_add_executable name)
  cmake_parse_arguments(JCMD
    "WIN32;MACOSX_BUNDLE;EXCLUDE_FROM_ALL;NO_INSTALL"
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

  # Read schema content for embedding into the executable
  get_filename_component(_schema_abs "${JCMD_SCHEMA}" ABSOLUTE)
  file(READ "${_schema_abs}" JCMD_SCHEMA_CONTENT)
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_schema_abs}")

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

  # C++ standard
  set_target_properties(${name} PROPERTIES
    CXX_STANDARD ${json_commander_CXX_STANDARD})

  # Ensure FROM_HEADER can be found relative to the caller's source dir
  target_include_directories(${name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

  if(NOT JCMD_NO_INSTALL)
    include(GNUInstallDirs)

    # Install schema file for reference
    install(FILES "${_schema_abs}"
      DESTINATION "${CMAKE_INSTALL_DATADIR}/${name}")

    # Output directory for generated man pages and completions
    set(_gen_dir "${CMAKE_CURRENT_BINARY_DIR}/${name}_generated")
    set(_man_script "${json_commander_TEMPLATE_DIR}/json_commander_generate_manpage.cmake")
    set(_exe_base_name "$<TARGET_FILE_BASE_NAME:${name}>")

    # Man page generation — root command
    set(_man_commands
      COMMAND ${CMAKE_COMMAND}
        -DJCMD_EXECUTABLE=$<TARGET_FILE:${name}>
        -DJCMD_OUTPUT_DIR=${_gen_dir}
        -DJCMD_NAME=${_exe_base_name}
        -P "${_man_script}")

    # Collect subcommand man pages
    set(_subcmd_names "")
    set(_subcmd_args "")
    _jcmd_collect_subcommands("${JCMD_SCHEMA_CONTENT}" "" "" _subcmd_names _subcmd_args)

    list(LENGTH _subcmd_names _subcmd_count)
    if(_subcmd_count GREATER 0)
      math(EXPR _subcmd_last "${_subcmd_count} - 1")
      foreach(_idx RANGE 0 ${_subcmd_last})
        list(GET _subcmd_names ${_idx} _sname)
        list(GET _subcmd_args ${_idx} _sargs)
        list(APPEND _man_commands
          COMMAND ${CMAKE_COMMAND}
            -DJCMD_EXECUTABLE=$<TARGET_FILE:${name}>
            -DJCMD_OUTPUT_DIR=${_gen_dir}
            -DJCMD_NAME=${_exe_base_name}
            "-DJCMD_SUBCOMMAND=${_sargs}"
            "-DJCMD_SUBCMD_NAME=${_sname}"
            -P "${_man_script}")
      endforeach()
    endif()

    add_custom_target(${name}_manpage ALL
      ${_man_commands}
      DEPENDS ${name}
      COMMENT "Generating man pages for ${name}")

    install(CODE "
      file(GLOB _manpages \"${_gen_dir}/*.1\")
      foreach(_mp IN LISTS _manpages)
        file(INSTALL \"\${_mp}\"
          DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_MANDIR}/man1\")
      endforeach()
    ")

    # Shell completion generation and installation
    set(_comp_script "${json_commander_TEMPLATE_DIR}/json_commander_generate_completion.cmake")

    add_custom_target(${name}_completions ALL
      COMMAND ${CMAKE_COMMAND}
        -DJCMD_EXECUTABLE=$<TARGET_FILE:${name}>
        -DJCMD_SHELL=bash
        -DJCMD_OUTPUT_DIR=${_gen_dir}
        -DJCMD_NAME=${_exe_base_name}
        -P "${_comp_script}"
      COMMAND ${CMAKE_COMMAND}
        -DJCMD_EXECUTABLE=$<TARGET_FILE:${name}>
        -DJCMD_SHELL=zsh
        -DJCMD_OUTPUT_DIR=${_gen_dir}
        -DJCMD_NAME=${_exe_base_name}
        -P "${_comp_script}"
      COMMAND ${CMAKE_COMMAND}
        -DJCMD_EXECUTABLE=$<TARGET_FILE:${name}>
        -DJCMD_SHELL=fish
        -DJCMD_OUTPUT_DIR=${_gen_dir}
        -DJCMD_NAME=${_exe_base_name}
        -P "${_comp_script}"
      DEPENDS ${name}
      COMMENT "Generating shell completions for ${name}")

    install(CODE "
      set(_gen_dir \"${_gen_dir}\")
      set(_exe_name \"$<TARGET_FILE_BASE_NAME:${name}>\")
      file(INSTALL \"\${_gen_dir}/\${_exe_name}.bash\"
        DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/bash-completion/completions\"
        RENAME \"\${_exe_name}\")
      file(INSTALL \"\${_gen_dir}/_\${_exe_name}\"
        DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/zsh/site-functions\")
      file(INSTALL \"\${_gen_dir}/\${_exe_name}.fish\"
        DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/fish/vendor_completions.d\")
    ")
  endif()
endfunction()
