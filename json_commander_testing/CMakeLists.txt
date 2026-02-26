find_package(Catch2 REQUIRED)

macro(json_commander_add_test test_name)
  set(exe_name ${test_name}_test)
  set(src_name ${exe_name}.cpp)
  add_executable(${exe_name} ${src_name})
  target_link_libraries(${exe_name} PRIVATE
    json_commander::header
    json_commander::library
    nlohmann_json::nlohmann_json
    nlohmann_json_schema_validator
    Catch2::Catch2WithMain)
  set_target_properties(${exe_name}
    PROPERTIES
    CXX_STANDARD ${json_commander_CXX_STANDARD})
  add_test(${test_name} ${exe_name})
endmacro()

json_commander_add_test(metaschema)
json_commander_add_test(model)
json_commander_add_test(schema_loader)
json_commander_add_test(conv)
json_commander_add_test(validate)
json_commander_add_test(arg)
json_commander_add_test(cmd)
json_commander_add_test(manpage)
json_commander_add_test(parse)
json_commander_add_test(config_schema)
json_commander_add_test(completion)

json_commander_add_test(run)
target_compile_definitions(run_test PRIVATE
  SERVE_SCHEMA="${CMAKE_SOURCE_DIR}/examples/serve/serve.json")
