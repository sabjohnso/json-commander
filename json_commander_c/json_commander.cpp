#include "json_commander.h"

#include <json_commander/run.hpp>

int
jcmd_run(const char* cli_json, int argc, char* argv[], jcmd_main_fn main_fn) {
  return json_commander::run(
    std::string(cli_json), argc, argv, [main_fn](const nlohmann::json& config) {
      return main_fn(config.dump().c_str());
    });
}
