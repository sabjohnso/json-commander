add_executable(c_api_test c_api_test.c)
set_target_properties(c_api_test PROPERTIES
  C_STANDARD 11
  C_STANDARD_REQUIRED ON
  LINKER_LANGUAGE CXX)
target_link_libraries(c_api_test PRIVATE json_commander::c)
ctest_deps_add_test(NAME c_api TARGET c_api_test)
