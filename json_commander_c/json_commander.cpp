#include "json_commander.h"

#include <json_commander/run.hpp>

#include <iostream>

int
jcmd_run(const char* cli_json, int argc, char* argv[], jcmd_main_fn main_fn) {
  try {
    return json_commander::run(
      std::string(cli_json),
      argc,
      argv,
      [main_fn](const nlohmann::json& config) {
        auto config_str = config.dump();
        return main_fn(config_str.c_str());
      });
  } catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
}
