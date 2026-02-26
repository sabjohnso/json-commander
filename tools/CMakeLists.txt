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
