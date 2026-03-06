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

# Man page generation and installation
set(_manpage "${CMAKE_CURRENT_BINARY_DIR}/json-commander.1")
set(_man_script "${json_commander_TEMPLATE_DIR}/json_commander_generate_manpage.cmake")

add_custom_command(
  OUTPUT "${_manpage}"
  COMMAND ${CMAKE_COMMAND}
    -DJCMD_EXECUTABLE=$<TARGET_FILE:json-commander>
    -DJCMD_OUTPUT=${_manpage}
    -P "${_man_script}"
  DEPENDS json-commander
  COMMENT "Generating man page for json-commander")

# Collect subcommand man pages
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/json_commander.json" _tool_schema)
set(_subcmd_names "")
set(_subcmd_args "")
_jcmd_collect_subcommands("${_tool_schema}" "" "" _subcmd_names _subcmd_args)
set(_all_manpages "${_manpage}")

list(LENGTH _subcmd_names _subcmd_count)
if(_subcmd_count GREATER 0)
  math(EXPR _subcmd_last "${_subcmd_count} - 1")
  foreach(_idx RANGE 0 ${_subcmd_last})
    list(GET _subcmd_names ${_idx} _sname)
    list(GET _subcmd_args ${_idx} _sargs)
    set(_sub_manpage "${CMAKE_CURRENT_BINARY_DIR}/json-commander-${_sname}.1")
    add_custom_command(
      OUTPUT "${_sub_manpage}"
      COMMAND ${CMAKE_COMMAND}
        -DJCMD_EXECUTABLE=$<TARGET_FILE:json-commander>
        "-DJCMD_SUBCOMMAND=${_sargs}"
        -DJCMD_OUTPUT=${_sub_manpage}
        -P "${_man_script}"
      DEPENDS json-commander
      COMMENT "Generating man page for json-commander-${_sname}")
    list(APPEND _all_manpages "${_sub_manpage}")
  endforeach()
endif()

add_custom_target(json-commander_manpage ALL DEPENDS ${_all_manpages})

install(FILES ${_all_manpages}
  DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")

# Shell completion generation and installation
set(_comp_script "${json_commander_TEMPLATE_DIR}/json_commander_generate_completion.cmake")

# Bash completion
set(_bash_comp "${CMAKE_CURRENT_BINARY_DIR}/json-commander.bash")
add_custom_command(
  OUTPUT "${_bash_comp}"
  COMMAND ${CMAKE_COMMAND}
    -DJCMD_EXECUTABLE=$<TARGET_FILE:json-commander>
    -DJCMD_SHELL=bash
    -DJCMD_OUTPUT=${_bash_comp}
    -P "${_comp_script}"
  DEPENDS json-commander
  COMMENT "Generating bash completion for json-commander")

# Zsh completion
set(_zsh_comp "${CMAKE_CURRENT_BINARY_DIR}/_json-commander")
add_custom_command(
  OUTPUT "${_zsh_comp}"
  COMMAND ${CMAKE_COMMAND}
    -DJCMD_EXECUTABLE=$<TARGET_FILE:json-commander>
    -DJCMD_SHELL=zsh
    -DJCMD_OUTPUT=${_zsh_comp}
    -P "${_comp_script}"
  DEPENDS json-commander
  COMMENT "Generating zsh completion for json-commander")

# Fish completion
set(_fish_comp "${CMAKE_CURRENT_BINARY_DIR}/json-commander.fish")
add_custom_command(
  OUTPUT "${_fish_comp}"
  COMMAND ${CMAKE_COMMAND}
    -DJCMD_EXECUTABLE=$<TARGET_FILE:json-commander>
    -DJCMD_SHELL=fish
    -DJCMD_OUTPUT=${_fish_comp}
    -P "${_comp_script}"
  DEPENDS json-commander
  COMMENT "Generating fish completion for json-commander")

add_custom_target(json-commander_completions ALL
  DEPENDS "${_bash_comp}" "${_zsh_comp}" "${_fish_comp}")

install(FILES "${_bash_comp}"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/bash-completion/completions"
  RENAME "json-commander")

install(FILES "${_zsh_comp}"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/zsh/site-functions")

install(FILES "${_fish_comp}"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/fish/vendor_completions.d")
