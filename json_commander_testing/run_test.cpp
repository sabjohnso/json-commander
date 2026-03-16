#include <catch2/catch_test_macros.hpp>
#include <json_commander/run.hpp>

using namespace json_commander;
using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Test fixture: a simple CLI with an --output option (default "out.txt")
// ---------------------------------------------------------------------------

static model::Root
make_test_cli() {
  model::Option output;
  output.names = {"output", "o"};
  output.doc = {"Output file."};
  output.type = model::ScalarType::String;
  output.default_value = "out.txt";

  model::Root root;
  root.name = "test-app";
  root.doc = {"A test application."};
  root.version = "1.0.0";
  root.args = std::vector<model::Argument>{output};
  return root;
}

static const char* test_cli_json = R"({
  "name": "test-app",
  "doc": ["A test application."],
  "version": "1.0.0",
  "args": [
    {
      "kind": "option",
      "names": ["output", "o"],
      "doc": ["Output file."],
      "type": "string",
      "default": "out.txt"
    }
  ]
})";

// ---------------------------------------------------------------------------
// Helper: build argv from initializer list
// ---------------------------------------------------------------------------

struct Argv {
  std::vector<std::string> storage;
  std::vector<char*> ptrs;

  Argv(std::initializer_list<std::string> args)
      : storage(args) {
    for (auto& s : storage) {
      ptrs.push_back(s.data());
    }
  }

  int
  argc() const {
    return static_cast<int>(ptrs.size());
  }
  char**
  argv() {
    return ptrs.data();
  }
};

// ===========================================================================
// Tests for run(const model::Root &, ...)
// ===========================================================================

TEST_CASE("run: parse ok", "[run]") {
  auto cli = make_test_cli();
  Argv args{"test-app", "--output", "foo.txt"};

  json captured;
  int rc =
    json_commander::run(cli, args.argc(), args.argv(), [&](const json& config) {
      captured = config;
      return 0;
    });

  REQUIRE(rc == 0);
  REQUIRE(captured["output"] == "foo.txt");
}

TEST_CASE("run: defaults applied", "[run]") {
  auto cli = make_test_cli();
  Argv args{"test-app"};

  json captured;
  int rc =
    json_commander::run(cli, args.argc(), args.argv(), [&](const json& config) {
      captured = config;
      return 0;
    });

  REQUIRE(rc == 0);
  REQUIRE(captured["output"] == "out.txt");
}

TEST_CASE("run: callback return value propagated", "[run]") {
  auto cli = make_test_cli();
  Argv args{"test-app"};

  int rc = json_commander::run(
    cli, args.argc(), args.argv(), [](const json&) { return 42; });

  REQUIRE(rc == 42);
}

TEST_CASE("run: --help returns 0, callback not called", "[run]") {
  auto cli = make_test_cli();
  Argv args{"test-app", "--help"};

  bool called = false;
  int rc = json_commander::run(cli, args.argc(), args.argv(), [&](const json&) {
    called = true;
    return 0;
  });

  REQUIRE(rc == 0);
  REQUIRE_FALSE(called);
}

TEST_CASE("run: --version returns 0, callback not called", "[run]") {
  auto cli = make_test_cli();
  Argv args{"test-app", "--version"};

  bool called = false;
  int rc = json_commander::run(cli, args.argc(), args.argv(), [&](const json&) {
    called = true;
    return 0;
  });

  REQUIRE(rc == 0);
  REQUIRE_FALSE(called);
}

TEST_CASE("run: --help-man returns 0, callback not called", "[run]") {
  auto cli = make_test_cli();
  Argv args{"test-app", "--help-man"};

  bool called = false;
  int rc = json_commander::run(cli, args.argc(), args.argv(), [&](const json&) {
    called = true;
    return 0;
  });

  REQUIRE(rc == 0);
  REQUIRE_FALSE(called);
}

TEST_CASE("run: parse error returns 1, callback not called", "[run]") {
  auto cli = make_test_cli();
  Argv args{"test-app", "--unknown"};

  bool called = false;
  int rc = json_commander::run(cli, args.argc(), args.argv(), [&](const json&) {
    called = true;
    return 0;
  });

  REQUIRE(rc == 1);
  REQUIRE_FALSE(called);
}

// ===========================================================================
// Tests for run(const std::string &, ...)  — JSON string overload
// ===========================================================================

TEST_CASE("run: json string overload parse ok", "[run]") {
  Argv args{"test-app", "--output", "bar.txt"};

  json captured;
  int rc = json_commander::run(
    std::string(test_cli_json),
    args.argc(),
    args.argv(),
    [&](const json& config) {
      captured = config;
      return 0;
    });

  REQUIRE(rc == 0);
  REQUIRE(captured["output"] == "bar.txt");
}

TEST_CASE("run: invalid schema throws", "[run]") {
  Argv args{"test-app"};

  bool called = false;
  REQUIRE_THROWS([&] {
    json_commander::run(
      std::string("{}"), args.argc(), args.argv(), [&](const json&) {
        called = true;
        return 0;
      });
  }());
  REQUIRE_FALSE(called);
}

TEST_CASE("run: bad json throws", "[run]") {
  Argv args{"test-app"};

  bool called = false;
  REQUIRE_THROWS_AS(
    json_commander::run(
      std::string("not json"),
      args.argc(),
      args.argv(),
      [&](const json&) {
        called = true;
        return 0;
      }),
    nlohmann::json::exception);
  REQUIRE_FALSE(called);
}

// ===========================================================================
// Tests for run_file
// ===========================================================================

TEST_CASE(
  "run: --help-completion bash returns 0, callback not called", "[run]") {
  auto cli = make_test_cli();
  Argv args{"test-app", "--help-completion", "bash"};

  bool called = false;
  int rc = json_commander::run(cli, args.argc(), args.argv(), [&](const json&) {
    called = true;
    return 0;
  });

  REQUIRE(rc == 0);
  REQUIRE_FALSE(called);
}

TEST_CASE(
  "run: --help-completion zsh returns 0, callback not called", "[run]") {
  auto cli = make_test_cli();
  Argv args{"test-app", "--help-completion", "zsh"};

  bool called = false;
  int rc = json_commander::run(cli, args.argc(), args.argv(), [&](const json&) {
    called = true;
    return 0;
  });

  REQUIRE(rc == 0);
  REQUIRE_FALSE(called);
}

TEST_CASE(
  "run: --help-completion fish returns 0, callback not called", "[run]") {
  auto cli = make_test_cli();
  Argv args{"test-app", "--help-completion", "fish"};

  bool called = false;
  int rc = json_commander::run(cli, args.argc(), args.argv(), [&](const json&) {
    called = true;
    return 0;
  });

  REQUIRE(rc == 0);
  REQUIRE_FALSE(called);
}

// ===========================================================================
// Tests for subcommand dispatch through run()
// ===========================================================================

static model::Root
make_subcmd_cli() {
  model::Option target;
  target.names = {"target", "t"};
  target.doc = {"Build target."};
  target.type = model::ScalarType::String;
  target.default_value = "debug";

  model::Command build;
  build.name = "build";
  build.doc = {"Build the project."};
  build.args = std::vector<model::Argument>{target};

  model::Command init;
  init.name = "init";
  init.doc = {"Initialize a project."};

  model::Flag verbose;
  verbose.names = {"verbose", "v"};
  verbose.doc = {"Enable verbose output."};

  model::Root root;
  root.name = "tool";
  root.doc = {"A test tool."};
  root.version = "1.0.0";
  root.args = std::vector<model::Argument>{verbose};
  root.commands = std::vector<model::Command>{build, init};
  return root;
}

TEST_CASE("run: subcommand config is nested and validated", "[run]") {
  auto cli = make_subcmd_cli();
  Argv args{"tool", "--verbose", "build", "--target", "release"};

  json captured;
  int rc =
    json_commander::run(cli, args.argc(), args.argv(), [&](const json& config) {
      captured = config;
      return 0;
    });

  REQUIRE(rc == 0);
  REQUIRE(captured["command"] == "build");
  REQUIRE(captured["verbose"] == true);
  REQUIRE(captured["build"]["target"] == "release");
}

TEST_CASE("run: subcommand with defaults is validated", "[run]") {
  auto cli = make_subcmd_cli();
  Argv args{"tool", "build"};

  json captured;
  int rc =
    json_commander::run(cli, args.argc(), args.argv(), [&](const json& config) {
      captured = config;
      return 0;
    });

  REQUIRE(rc == 0);
  REQUIRE(captured["command"] == "build");
  REQUIRE(captured["verbose"] == false);
  REQUIRE(captured["build"]["target"] == "debug");
}

TEST_CASE(
  "run: missing subcommand shows help and returns 1, callback not called",
  "[run]") {
  auto cli = make_subcmd_cli();
  Argv args{"tool"};

  bool called = false;
  int rc = json_commander::run(cli, args.argc(), args.argv(), [&](const json&) {
    called = true;
    return 0;
  });

  REQUIRE(rc == 1);
  REQUIRE_FALSE(called);
}

TEST_CASE("run: missing nested subcommand shows help and returns 1", "[run]") {
  model::Command set_cmd;
  set_cmd.name = "set";
  set_cmd.doc = {"Set a config value."};

  model::Command get_cmd;
  get_cmd.name = "get";
  get_cmd.doc = {"Get a config value."};

  model::Command config_cmd;
  config_cmd.name = "config";
  config_cmd.doc = {"Manage configuration."};
  config_cmd.commands = std::vector<model::Command>{set_cmd, get_cmd};

  model::Root root;
  root.name = "tool";
  root.doc = {"A test tool."};
  root.version = "1.0.0";
  root.commands = std::vector<model::Command>{config_cmd};

  Argv args{"tool", "config"};

  bool called = false;
  int rc =
    json_commander::run(root, args.argc(), args.argv(), [&](const json&) {
      called = true;
      return 0;
    });

  REQUIRE(rc == 1);
  REQUIRE_FALSE(called);
}

// ===========================================================================
// Tests for run_file
// ===========================================================================

TEST_CASE("run_file: loads schema and parses ok", "[run]") {
  Argv args{"serve", "--port", "3000"};

  json captured;
  int rc = json_commander::run_file(
    std::filesystem::path(SERVE_SCHEMA),
    args.argc(),
    args.argv(),
    [&](const json& config) {
      captured = config;
      return 0;
    });

  REQUIRE(rc == 0);
  REQUIRE(captured["port"] == 3000);
}
