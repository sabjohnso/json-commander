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

TEST_CASE("run: invalid schema returns 1", "[run]") {
  Argv args{"test-app"};

  bool called = false;
  int rc = json_commander::run(
    std::string("{}"), args.argc(), args.argv(), [&](const json&) {
      called = true;
      return 0;
    });

  REQUIRE(rc == 1);
  REQUIRE_FALSE(called);
}

TEST_CASE("run: bad json returns 1", "[run]") {
  Argv args{"test-app"};

  bool called = false;
  int rc = json_commander::run(
    std::string("not json"), args.argc(), args.argv(), [&](const json&) {
      called = true;
      return 0;
    });

  REQUIRE(rc == 1);
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
