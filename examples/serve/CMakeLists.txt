json_commander_add_executable(serve
  SCHEMA serve.json
  MAIN serve::run
  FROM_HEADER serve_main.hpp)
