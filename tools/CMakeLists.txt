# Embed the tool schema JSON as a string literal
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/json_commander.json" JCMD_TOOL_SCHEMA_CONTENT)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/json_commander.json")
configure_file(json_commander_schema.hpp.in
  "${CMAKE_CURRENT_BINARY_DIR}/json_commander_schema.hpp" @ONLY)

add_executable(json-commander json_commander.cpp)
target_link_libraries(json-commander PRIVATE
  json_commander::header
  json_commander::library
  nlohmann_json::nlohmann_json
  nlohmann_json_schema_validator)
target_include_directories(json-commander PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
set_target_properties(json-commander PROPERTIES
  CXX_STANDARD ${json_commander_CXX_STANDARD})

install(TARGETS json-commander
  EXPORT json_commander_EXPORTS
  RUNTIME DESTINATION ${json_commander_INSTALL_BINDIR})

install(FILES json_commander.json
  DESTINATION ${CMAKE_INSTALL_DATADIR}/json_commander)

# Output directory for generated man pages and completions
set(_gen_dir "${CMAKE_CURRENT_BINARY_DIR}/json-commander_generated")
set(_man_script "${json_commander_TEMPLATE_DIR}/json_commander_generate_manpage.cmake")
set(_exe_base_name "$<TARGET_FILE_BASE_NAME:json-commander>")

# Man page generation — root command
set(_man_commands
  COMMAND ${CMAKE_COMMAND}
    -DJCMD_EXECUTABLE=$<TARGET_FILE:json-commander>
    -DJCMD_OUTPUT_DIR=${_gen_dir}
    -DJCMD_NAME=${_exe_base_name}
    -P "${_man_script}")

# Collect subcommand man pages
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/json_commander.json" _tool_schema)
set(_subcmd_names "")
set(_subcmd_args "")
_jcmd_collect_subcommands("${_tool_schema}" "" "" _subcmd_names _subcmd_args)

list(LENGTH _subcmd_names _subcmd_count)
if(_subcmd_count GREATER 0)
  math(EXPR _subcmd_last "${_subcmd_count} - 1")
  foreach(_idx RANGE 0 ${_subcmd_last})
    list(GET _subcmd_names ${_idx} _sname)
    list(GET _subcmd_args ${_idx} _sargs)
    list(APPEND _man_commands
      COMMAND ${CMAKE_COMMAND}
        -DJCMD_EXECUTABLE=$<TARGET_FILE:json-commander>
        -DJCMD_OUTPUT_DIR=${_gen_dir}
        -DJCMD_NAME=${_exe_base_name}
        "-DJCMD_SUBCOMMAND=${_sargs}"
        "-DJCMD_SUBCMD_NAME=${_sname}"
        -P "${_man_script}")
  endforeach()
endif()

add_custom_target(json-commander_manpage ALL
  ${_man_commands}
  DEPENDS json-commander
  COMMENT "Generating man pages for json-commander")

install(CODE "
  file(GLOB _manpages \"${_gen_dir}/*.1\")
  foreach(_mp IN LISTS _manpages)
    file(INSTALL \"\${_mp}\"
      DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_MANDIR}/man1\")
  endforeach()
")

# Shell completion generation and installation
set(_comp_script "${json_commander_TEMPLATE_DIR}/json_commander_generate_completion.cmake")

add_custom_target(json-commander_completions ALL
  COMMAND ${CMAKE_COMMAND}
    -DJCMD_EXECUTABLE=$<TARGET_FILE:json-commander>
    -DJCMD_SHELL=bash
    -DJCMD_OUTPUT_DIR=${_gen_dir}
    -DJCMD_NAME=${_exe_base_name}
    -P "${_comp_script}"
  COMMAND ${CMAKE_COMMAND}
    -DJCMD_EXECUTABLE=$<TARGET_FILE:json-commander>
    -DJCMD_SHELL=zsh
    -DJCMD_OUTPUT_DIR=${_gen_dir}
    -DJCMD_NAME=${_exe_base_name}
    -P "${_comp_script}"
  COMMAND ${CMAKE_COMMAND}
    -DJCMD_EXECUTABLE=$<TARGET_FILE:json-commander>
    -DJCMD_SHELL=fish
    -DJCMD_OUTPUT_DIR=${_gen_dir}
    -DJCMD_NAME=${_exe_base_name}
    -P "${_comp_script}"
  DEPENDS json-commander
  COMMENT "Generating shell completions for json-commander")

install(CODE "
  set(_gen_dir \"${_gen_dir}\")
  set(_exe_name \"$<TARGET_FILE_BASE_NAME:json-commander>\")
  file(INSTALL \"\${_gen_dir}/\${_exe_name}.bash\"
    DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/bash-completion/completions\"
    RENAME \"\${_exe_name}\")
  file(INSTALL \"\${_gen_dir}/_\${_exe_name}\"
    DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/zsh/site-functions\")
  file(INSTALL \"\${_gen_dir}/\${_exe_name}.fish\"
    DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/fish/vendor_completions.d\")
")
