#pragma once

#include <iostream>
#include <string>

#include <nlohmann/json.hpp>

namespace serve {

  inline int
  run(const nlohmann::json& config) {
    auto port = config["port"].get<int>();
    auto host = config["host"].get<std::string>();
    auto dir = config["dir"].get<std::string>();
    auto verbose = config["verbose"].get<bool>();

    std::cout << "Serving " << dir << " on " << host << ":" << port;
    if (verbose) { std::cout << " (verbose)"; }
    std::cout << "\n";
    return 0;
  }

} // namespace serve
