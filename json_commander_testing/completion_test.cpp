#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <json_commander/completion.hpp>

using namespace json_commander;
using Catch::Matchers::ContainsSubstring;

// ---------------------------------------------------------------------------
// Test fixture: simple CLI model
// ---------------------------------------------------------------------------

static model::Root
make_simple_cli() {
  model::Flag verbose;
  verbose.names = {"verbose", "v"};
  verbose.doc = {"Enable verbose output."};

  model::Option output;
  output.names = {"output", "o"};
  output.doc = {"Output file path."};
  output.type = model::ScalarType::File;

  model::Option format;
  format.names = {"format"};
  format.doc = {"Output format."};
  format.type = model::ScalarType::Enum;
  format.choices = std::vector<std::string>{"json", "yaml", "toml"};

  model::Positional input;
  input.name = "input";
  input.doc = {"Input file."};
  input.type = model::ScalarType::File;

  model::Root root;
  root.name = "mytool";
  root.doc = {"A test tool."};
  root.version = "1.0.0";
  root.args = std::vector<model::Argument>{verbose, output, format, input};
  return root;
}

// ---------------------------------------------------------------------------
// Test fixture: CLI with subcommands
// ---------------------------------------------------------------------------

static model::Root
make_subcommand_cli() {
  model::Option target;
  target.names = {"target", "t"};
  target.doc = {"Build target."};
  target.type = model::ScalarType::String;

  model::Command build;
  build.name = "build";
  build.doc = {"Build the project."};
  build.args = std::vector<model::Argument>{target};

  model::Flag all;
  all.names = {"all"};
  all.doc = {"Run all tests."};

  model::Command test;
  test.name = "test";
  test.doc = {"Run tests."};
  test.args = std::vector<model::Argument>{all};

  model::Flag verbose;
  verbose.names = {"verbose", "v"};
  verbose.doc = {"Verbose output."};

  model::Root root;
  root.name = "mytool";
  root.doc = {"A test tool."};
  root.version = "2.0.0";
  root.args = std::vector<model::Argument>{verbose};
  root.commands = std::vector<model::Command>{build, test};
  return root;
}

// ---------------------------------------------------------------------------
// Test fixture: CLI with flag group and dir type
// ---------------------------------------------------------------------------

static model::Root
make_flaggroup_cli() {
  model::FlagGroupEntry json_entry;
  json_entry.names = {"json"};
  json_entry.doc = {"JSON output."};
  json_entry.value = "json";

  model::FlagGroupEntry yaml_entry;
  yaml_entry.names = {"yaml"};
  yaml_entry.doc = {"YAML output."};
  yaml_entry.value = "yaml";

  model::FlagGroup group;
  group.dest = "format";
  group.doc = {"Output format."};
  group.default_value = "json";
  group.flags = {json_entry, yaml_entry};

  model::Option dir;
  dir.names = {"dir", "d"};
  dir.doc = {"Working directory."};
  dir.type = model::ScalarType::Dir;

  model::Root root;
  root.name = "fgtool";
  root.doc = {"Flag group tool."};
  root.args = std::vector<model::Argument>{group, dir};
  return root;
}

// ===========================================================================
// Bash completion tests
// ===========================================================================

TEST_CASE(
  "bash: contains function name from program name", "[completion][bash]") {
  auto root = make_simple_cli();
  auto result = completion::to_bash(root);
  REQUIRE_THAT(result, ContainsSubstring("_mytool"));
}

TEST_CASE("bash: contains long option names", "[completion][bash]") {
  auto root = make_simple_cli();
  auto result = completion::to_bash(root);
  REQUIRE_THAT(result, ContainsSubstring("--verbose"));
  REQUIRE_THAT(result, ContainsSubstring("--output"));
  REQUIRE_THAT(result, ContainsSubstring("--format"));
}

TEST_CASE("bash: contains short option names", "[completion][bash]") {
  auto root = make_simple_cli();
  auto result = completion::to_bash(root);
  REQUIRE_THAT(result, ContainsSubstring("-v"));
  REQUIRE_THAT(result, ContainsSubstring("-o"));
}

TEST_CASE("bash: contains builtin flags", "[completion][bash]") {
  auto root = make_simple_cli();
  auto result = completion::to_bash(root);
  REQUIRE_THAT(result, ContainsSubstring("--help"));
  REQUIRE_THAT(result, ContainsSubstring("--version"));
  REQUIRE_THAT(result, ContainsSubstring("--help-man"));
  REQUIRE_THAT(result, ContainsSubstring("--help-completion"));
}

TEST_CASE("bash: enum choices appear for --format", "[completion][bash]") {
  auto root = make_simple_cli();
  auto result = completion::to_bash(root);
  REQUIRE_THAT(result, ContainsSubstring("json"));
  REQUIRE_THAT(result, ContainsSubstring("yaml"));
  REQUIRE_THAT(result, ContainsSubstring("toml"));
}

TEST_CASE("bash: file type triggers file completion", "[completion][bash]") {
  auto root = make_simple_cli();
  auto result = completion::to_bash(root);
  // compopt -o filenames or similar file completion mechanism
  REQUIRE_THAT(result, ContainsSubstring("compopt"));
}

TEST_CASE("bash: subcommand names listed", "[completion][bash]") {
  auto root = make_subcommand_cli();
  auto result = completion::to_bash(root);
  REQUIRE_THAT(result, ContainsSubstring("build"));
  REQUIRE_THAT(result, ContainsSubstring("test"));
}

TEST_CASE("bash: subcommand options included", "[completion][bash]") {
  auto root = make_subcommand_cli();
  auto result = completion::to_bash(root);
  REQUIRE_THAT(result, ContainsSubstring("--target"));
  REQUIRE_THAT(result, ContainsSubstring("--all"));
}

TEST_CASE("bash: flag group entries appear as options", "[completion][bash]") {
  auto root = make_flaggroup_cli();
  auto result = completion::to_bash(root);
  REQUIRE_THAT(result, ContainsSubstring("--json"));
  REQUIRE_THAT(result, ContainsSubstring("--yaml"));
}

TEST_CASE(
  "bash: dir type triggers directory completion", "[completion][bash]") {
  auto root = make_flaggroup_cli();
  auto result = completion::to_bash(root);
  REQUIRE_THAT(result, ContainsSubstring("compopt"));
}

TEST_CASE("bash: registers with complete command", "[completion][bash]") {
  auto root = make_simple_cli();
  auto result = completion::to_bash(root);
  REQUIRE_THAT(result, ContainsSubstring("complete -F _mytool mytool"));
}

// ===========================================================================
// Zsh completion tests
// ===========================================================================

TEST_CASE("zsh: contains compdef directive", "[completion][zsh]") {
  auto root = make_simple_cli();
  auto result = completion::to_zsh(root);
  REQUIRE_THAT(result, ContainsSubstring("compdef _mytool mytool"));
}

TEST_CASE("zsh: contains function definition", "[completion][zsh]") {
  auto root = make_simple_cli();
  auto result = completion::to_zsh(root);
  REQUIRE_THAT(result, ContainsSubstring("_mytool"));
}

TEST_CASE("zsh: contains option names with descriptions", "[completion][zsh]") {
  auto root = make_simple_cli();
  auto result = completion::to_zsh(root);
  REQUIRE_THAT(result, ContainsSubstring("--verbose"));
  REQUIRE_THAT(result, ContainsSubstring("--output"));
  REQUIRE_THAT(result, ContainsSubstring("--format"));
}

TEST_CASE("zsh: contains short options", "[completion][zsh]") {
  auto root = make_simple_cli();
  auto result = completion::to_zsh(root);
  REQUIRE_THAT(result, ContainsSubstring("-v"));
  REQUIRE_THAT(result, ContainsSubstring("-o"));
}

TEST_CASE("zsh: file type uses _files", "[completion][zsh]") {
  auto root = make_simple_cli();
  auto result = completion::to_zsh(root);
  REQUIRE_THAT(result, ContainsSubstring("_files"));
}

TEST_CASE("zsh: enum choices listed", "[completion][zsh]") {
  auto root = make_simple_cli();
  auto result = completion::to_zsh(root);
  REQUIRE_THAT(result, ContainsSubstring("json"));
  REQUIRE_THAT(result, ContainsSubstring("yaml"));
  REQUIRE_THAT(result, ContainsSubstring("toml"));
}

TEST_CASE("zsh: subcommand names appear", "[completion][zsh]") {
  auto root = make_subcommand_cli();
  auto result = completion::to_zsh(root);
  REQUIRE_THAT(result, ContainsSubstring("build"));
  REQUIRE_THAT(result, ContainsSubstring("test"));
}

TEST_CASE("zsh: subcommand descriptions appear", "[completion][zsh]") {
  auto root = make_subcommand_cli();
  auto result = completion::to_zsh(root);
  REQUIRE_THAT(result, ContainsSubstring("Build the project"));
  REQUIRE_THAT(result, ContainsSubstring("Run tests"));
}

TEST_CASE("zsh: dir type uses _directories", "[completion][zsh]") {
  auto root = make_flaggroup_cli();
  auto result = completion::to_zsh(root);
  REQUIRE_THAT(result, ContainsSubstring("_directories"));
}

TEST_CASE("zsh: builtin flags present", "[completion][zsh]") {
  auto root = make_simple_cli();
  auto result = completion::to_zsh(root);
  REQUIRE_THAT(result, ContainsSubstring("--help"));
  REQUIRE_THAT(result, ContainsSubstring("--version"));
  REQUIRE_THAT(result, ContainsSubstring("--help-man"));
  REQUIRE_THAT(result, ContainsSubstring("--help-completion"));
}

// ===========================================================================
// Fish completion tests
// ===========================================================================

TEST_CASE("fish: contains complete commands", "[completion][fish]") {
  auto root = make_simple_cli();
  auto result = completion::to_fish(root);
  REQUIRE_THAT(result, ContainsSubstring("complete -c mytool"));
}

TEST_CASE("fish: contains long option names", "[completion][fish]") {
  auto root = make_simple_cli();
  auto result = completion::to_fish(root);
  REQUIRE_THAT(result, ContainsSubstring("-l verbose"));
  REQUIRE_THAT(result, ContainsSubstring("-l output"));
  REQUIRE_THAT(result, ContainsSubstring("-l format"));
}

TEST_CASE("fish: contains short option names", "[completion][fish]") {
  auto root = make_simple_cli();
  auto result = completion::to_fish(root);
  REQUIRE_THAT(result, ContainsSubstring("-s v"));
  REQUIRE_THAT(result, ContainsSubstring("-s o"));
}

TEST_CASE("fish: contains descriptions", "[completion][fish]") {
  auto root = make_simple_cli();
  auto result = completion::to_fish(root);
  REQUIRE_THAT(result, ContainsSubstring("Enable verbose output"));
  REQUIRE_THAT(result, ContainsSubstring("Output file path"));
}

TEST_CASE("fish: file type uses -F flag", "[completion][fish]") {
  auto root = make_simple_cli();
  auto result = completion::to_fish(root);
  REQUIRE_THAT(result, ContainsSubstring("-F"));
}

TEST_CASE("fish: enum choices appear", "[completion][fish]") {
  auto root = make_simple_cli();
  auto result = completion::to_fish(root);
  REQUIRE_THAT(result, ContainsSubstring("json"));
  REQUIRE_THAT(result, ContainsSubstring("yaml"));
  REQUIRE_THAT(result, ContainsSubstring("toml"));
}

TEST_CASE("fish: subcommand names listed", "[completion][fish]") {
  auto root = make_subcommand_cli();
  auto result = completion::to_fish(root);
  REQUIRE_THAT(result, ContainsSubstring("build"));
  REQUIRE_THAT(result, ContainsSubstring("test"));
}

TEST_CASE("fish: subcommand descriptions present", "[completion][fish]") {
  auto root = make_subcommand_cli();
  auto result = completion::to_fish(root);
  REQUIRE_THAT(result, ContainsSubstring("Build the project"));
  REQUIRE_THAT(result, ContainsSubstring("Run tests"));
}

TEST_CASE("fish: builtin flags present", "[completion][fish]") {
  auto root = make_simple_cli();
  auto result = completion::to_fish(root);
  REQUIRE_THAT(result, ContainsSubstring("help"));
  REQUIRE_THAT(result, ContainsSubstring("version"));
  REQUIRE_THAT(result, ContainsSubstring("help-man"));
  REQUIRE_THAT(result, ContainsSubstring("help-completion"));
}

TEST_CASE("fish: flag group entries appear", "[completion][fish]") {
  auto root = make_flaggroup_cli();
  auto result = completion::to_fish(root);
  REQUIRE_THAT(result, ContainsSubstring("-l json"));
  REQUIRE_THAT(result, ContainsSubstring("-l yaml"));
}

// ===========================================================================
// Edge case: no arguments, no commands
// ===========================================================================

TEST_CASE("bash: minimal CLI (no args, no commands)", "[completion][bash]") {
  model::Root root;
  root.name = "minimal";
  root.doc = {"Minimal tool."};
  auto result = completion::to_bash(root);
  REQUIRE_THAT(result, ContainsSubstring("_minimal"));
  REQUIRE_THAT(result, ContainsSubstring("--help"));
}

TEST_CASE("zsh: minimal CLI (no args, no commands)", "[completion][zsh]") {
  model::Root root;
  root.name = "minimal";
  root.doc = {"Minimal tool."};
  auto result = completion::to_zsh(root);
  REQUIRE_THAT(result, ContainsSubstring("_minimal"));
  REQUIRE_THAT(result, ContainsSubstring("--help"));
}

TEST_CASE("fish: minimal CLI (no args, no commands)", "[completion][fish]") {
  model::Root root;
  root.name = "minimal";
  root.doc = {"Minimal tool."};
  auto result = completion::to_fish(root);
  REQUIRE_THAT(result, ContainsSubstring("complete -c minimal"));
  REQUIRE_THAT(result, ContainsSubstring("help"));
}
