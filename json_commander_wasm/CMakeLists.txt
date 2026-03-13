if(NOT EMSCRIPTEN)
  message(STATUS "Skipping json_commander_wasm: not an Emscripten build")
  return()
endif()

# Exceptions must be enabled globally for json-schema-validator compatibility.
# This is set here rather than in the root CMakeLists.txt so that native builds
# are not affected.
add_compile_options(-fexceptions)
add_link_options(-fexceptions)

add_executable(json_commander_wasm wasm_shim.cpp)

set_target_properties(json_commander_wasm PROPERTIES
  CXX_STANDARD ${json_commander_CXX_STANDARD}
  CXX_STANDARD_REQUIRED ON)

target_link_libraries(json_commander_wasm PRIVATE
  json_commander::header
  nlohmann_json::nlohmann_json
  nlohmann_json_schema_validator)

set_target_properties(json_commander_wasm PROPERTIES
  OUTPUT_NAME json_commander_wasm
  SUFFIX ".js")

target_link_options(json_commander_wasm PRIVATE
  --bind
  -Oz
  -flto
  "SHELL:-s MODULARIZE=1"
  "SHELL:-s EXPORT_NAME=createJsonCommanderModule"
  "SHELL:-s ALLOW_MEMORY_GROWTH=1"
  "SHELL:-s ENVIRONMENT=web,node"
  "SHELL:-s DISABLE_EXCEPTION_CATCHING=0"
  "SHELL:-s EXPORT_ES6=1")
