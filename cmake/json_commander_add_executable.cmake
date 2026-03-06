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

    # Man page generation and installation
    set(_manpage "${CMAKE_CURRENT_BINARY_DIR}/${name}.1")
    set(_man_script "${json_commander_TEMPLATE_DIR}/json_commander_generate_manpage.cmake")

    add_custom_command(
      OUTPUT "${_manpage}"
      COMMAND ${CMAKE_COMMAND}
        -DJCMD_EXECUTABLE=$<TARGET_FILE:${name}>
        -DJCMD_OUTPUT=${_manpage}
        -P "${_man_script}"
      DEPENDS ${name}
      COMMENT "Generating man page for ${name}")

    # Collect subcommand man pages
    set(_subcmd_names "")
    set(_subcmd_args "")
    _jcmd_collect_subcommands("${JCMD_SCHEMA_CONTENT}" "" "" _subcmd_names _subcmd_args)
    set(_all_manpages "${_manpage}")

    list(LENGTH _subcmd_names _subcmd_count)
    if(_subcmd_count GREATER 0)
      math(EXPR _subcmd_last "${_subcmd_count} - 1")
      foreach(_idx RANGE 0 ${_subcmd_last})
        list(GET _subcmd_names ${_idx} _sname)
        list(GET _subcmd_args ${_idx} _sargs)
        set(_sub_manpage "${CMAKE_CURRENT_BINARY_DIR}/${name}-${_sname}.1")
        add_custom_command(
          OUTPUT "${_sub_manpage}"
          COMMAND ${CMAKE_COMMAND}
            -DJCMD_EXECUTABLE=$<TARGET_FILE:${name}>
            "-DJCMD_SUBCOMMAND=${_sargs}"
            -DJCMD_OUTPUT=${_sub_manpage}
            -P "${_man_script}"
          DEPENDS ${name}
          COMMENT "Generating man page for ${name}-${_sname}")
        list(APPEND _all_manpages "${_sub_manpage}")
      endforeach()
    endif()

    add_custom_target(${name}_manpage ALL DEPENDS ${_all_manpages})

    install(FILES ${_all_manpages}
      DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")

    # Shell completion generation and installation
    set(_comp_script "${json_commander_TEMPLATE_DIR}/json_commander_generate_completion.cmake")

    # Bash completion
    set(_bash_comp "${CMAKE_CURRENT_BINARY_DIR}/${name}.bash")
    add_custom_command(
      OUTPUT "${_bash_comp}"
      COMMAND ${CMAKE_COMMAND}
        -DJCMD_EXECUTABLE=$<TARGET_FILE:${name}>
        -DJCMD_SHELL=bash
        -DJCMD_OUTPUT=${_bash_comp}
        -P "${_comp_script}"
      DEPENDS ${name}
      COMMENT "Generating bash completion for ${name}")

    # Zsh completion
    set(_zsh_comp "${CMAKE_CURRENT_BINARY_DIR}/_${name}")
    add_custom_command(
      OUTPUT "${_zsh_comp}"
      COMMAND ${CMAKE_COMMAND}
        -DJCMD_EXECUTABLE=$<TARGET_FILE:${name}>
        -DJCMD_SHELL=zsh
        -DJCMD_OUTPUT=${_zsh_comp}
        -P "${_comp_script}"
      DEPENDS ${name}
      COMMENT "Generating zsh completion for ${name}")

    # Fish completion
    set(_fish_comp "${CMAKE_CURRENT_BINARY_DIR}/${name}.fish")
    add_custom_command(
      OUTPUT "${_fish_comp}"
      COMMAND ${CMAKE_COMMAND}
        -DJCMD_EXECUTABLE=$<TARGET_FILE:${name}>
        -DJCMD_SHELL=fish
        -DJCMD_OUTPUT=${_fish_comp}
        -P "${_comp_script}"
      DEPENDS ${name}
      COMMENT "Generating fish completion for ${name}")

    add_custom_target(${name}_completions ALL
      DEPENDS "${_bash_comp}" "${_zsh_comp}" "${_fish_comp}")

    install(FILES "${_bash_comp}"
      DESTINATION "${CMAKE_INSTALL_DATADIR}/bash-completion/completions"
      RENAME "${name}")

    install(FILES "${_zsh_comp}"
      DESTINATION "${CMAKE_INSTALL_DATADIR}/zsh/site-functions")

    install(FILES "${_fish_comp}"
      DESTINATION "${CMAKE_INSTALL_DATADIR}/fish/vendor_completions.d")
  endif()
endfunction()
