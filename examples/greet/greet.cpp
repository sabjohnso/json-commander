// greet — A simple CLI example using JSON-Commander.
//
// Demonstrates:
//   - Building a model::Root programmatically
//   - Using json_commander::run() for zero-boilerplate dispatch

#include <json_commander/run.hpp>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>

using namespace json_commander;

// ---------------------------------------------------------------------------
// CLI definition
// ---------------------------------------------------------------------------

model::Root
make_cli() {
  model::Flag loud;
  loud.names = {"loud", "l"};
  loud.doc = {"Print the greeting in uppercase."};

  model::Positional name;
  name.name = "name";
  name.doc = {"The name to greet."};
  name.type = model::ScalarType::String;
  name.required = true;

  model::Root root;
  root.name = "greet";
  root.doc = {"A friendly greeting tool."};
  root.version = "1.0.0";
  root.args = std::vector<model::Argument>{loud, name};
  return root;
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int
main(int argc, char* argv[]) {
  return json_commander::run(
    make_cli(), argc, argv, [](const nlohmann::json& config) {
      std::string greeting =
        "Hello, " + config["name"].get<std::string>() + "!";
      if (config["loud"].get<bool>()) {
        std::transform(
          greeting.begin(),
          greeting.end(),
          greeting.begin(),
          [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
      }
      std::cout << greeting << "\n";
      return 0;
    });
}
