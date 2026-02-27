if(NOT json_commander_INSTALL_EXAMPLES)
  set(_serve_no_install NO_INSTALL)
endif()

json_commander_add_executable(serve
  ${_serve_no_install}
  SCHEMA serve.json
  MAIN serve::run
  FROM_HEADER serve_main.hpp)
