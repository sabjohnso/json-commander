# Build-tree config.hpp (paths point to source/build dirs)
set(JSON_COMMANDER_SCHEMA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/schema")
set(JSON_COMMANDER_TOOL_SCHEMA "${PROJECT_SOURCE_DIR}/tools/json_commander.json")
configure_file(config.hpp.in config.hpp)

# Install-tree config.hpp (paths point to install dirs)
set(JSON_COMMANDER_SCHEMA_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/json_commander/schema")
set(JSON_COMMANDER_TOOL_SCHEMA "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/json_commander/json_commander.json")
configure_file(config.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/install/json_commander/config.hpp)

add_library(json_commander_header INTERFACE)
set_target_properties(json_commander_header PROPERTIES EXPORT_NAME header)
target_include_directories(json_commander_header
  INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
  $<INSTALL_INTERFACE:${json_commander_INSTALL_INCLUDEDIR}>)

add_library(json_commander::header ALIAS json_commander_header)

add_library(json_commander_library INTERFACE)
set_target_properties(json_commander_library PROPERTIES EXPORT_NAME library)
add_library(json_commander::library ALIAS json_commander_library)

install(TARGETS json_commander_header json_commander_library
  EXPORT json_commander_EXPORTS
  RUNTIME DESTINATION ${json_commander_INSTALL_BINDIR}
  LIBRARY DESTINATION ${json_commander_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${json_commander_INSTALL_LIBDIR})

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/install/json_commander/config.hpp
  arg.hpp
  cmd.hpp
  config_schema.hpp
  conv.hpp
  manpage.hpp
  model.hpp
  model_json.hpp
  parse.hpp
  run.hpp
  schema_loader.hpp
  validate.hpp
  DESTINATION ${json_commander_INSTALL_INCLUDEDIR}/json_commander)

install(DIRECTORY schema/
  DESTINATION ${CMAKE_INSTALL_DATADIR}/json_commander/schema
  FILES_MATCHING PATTERN "*.json")
