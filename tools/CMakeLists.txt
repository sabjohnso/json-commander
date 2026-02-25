add_executable(json-commander json_commander.cpp)
target_link_libraries(json-commander PRIVATE
  json_commander::header
  json_commander::library
  nlohmann_json::nlohmann_json
  nlohmann_json_schema_validator)
set_target_properties(json-commander PROPERTIES
  CXX_STANDARD ${json_commander_CXX_STANDARD})

install(TARGETS json-commander
  EXPORT json_commander_EXPORTS
  RUNTIME DESTINATION ${json_commander_INSTALL_BINDIR})

install(FILES json_commander.json
  DESTINATION ${CMAKE_INSTALL_DATADIR}/json_commander)
