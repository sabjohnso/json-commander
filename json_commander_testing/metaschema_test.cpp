#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <json_commander/config.hpp>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;
using nlohmann::json_schema::json_validator;

namespace {

const std::string schema_path =
    std::string(json_commander::Config::Info::Paths::schema_dir) + "/json_commander.schema.json";

json load_json(const std::string &path) {
  std::ifstream f(path);
  REQUIRE(f.is_open());
  return json::parse(f);
}

json load_metaschema() { return load_json(schema_path); }

void validate(const json &schema, const json &instance) {
  json_validator validator;
  validator.set_root_schema(schema);
  validator.validate(instance);
}

void expect_valid(const json &schema, const json &instance) {
  REQUIRE_NOTHROW(validate(schema, instance));
}

void expect_invalid(const json &schema, const json &instance) {
  REQUIRE_THROWS(validate(schema, instance));
}

json app(json extra = json::object()) {
  json base = {{"name", "myapp"}, {"doc", {"A test application"}}};
  base.merge_patch(extra);
  return base;
}

} // namespace

// ---------------------------------------------------------------------------
// Basic schema structure
// ---------------------------------------------------------------------------

TEST_CASE("Metaschema is valid JSON", "[metaschema]") {
  REQUIRE_NOTHROW(load_metaschema());
}

TEST_CASE("Metaschema can be loaded as a validator", "[metaschema]") {
  auto schema = load_metaschema();
  json_validator validator;
  REQUIRE_NOTHROW(validator.set_root_schema(schema));
}

TEST_CASE("Minimal valid CLI schema", "[metaschema]") {
  auto schema = load_metaschema();
  expect_valid(schema, app());
}

TEST_CASE("Missing required fields are rejected", "[metaschema]") {
  auto schema = load_metaschema();

  SECTION("missing name") { expect_invalid(schema, {{"doc", {"A simple application"}}}); }

  SECTION("missing doc") { expect_invalid(schema, {{"name", "myapp"}}); }

  SECTION("empty object") { expect_invalid(schema, json::object()); }
}

TEST_CASE("Invalid name formats are rejected", "[metaschema]") {
  auto schema = load_metaschema();

  SECTION("name starting with digit") { expect_invalid(schema, app({{"name", "1app"}})); }

  SECTION("name with spaces") { expect_invalid(schema, app({{"name", "my app"}})); }

  SECTION("empty name") { expect_invalid(schema, app({{"name", ""}})); }
}

TEST_CASE("Version field", "[metaschema]") {
  auto schema = load_metaschema();

  SECTION("valid semver") { expect_valid(schema, app({{"version", "1.2.3"}})); }

  SECTION("two-part version") { expect_valid(schema, app({{"version", "1.0"}})); }

  SECTION("invalid version string") { expect_invalid(schema, app({{"version", "abc"}})); }
}

// ---------------------------------------------------------------------------
// doc_string format
// ---------------------------------------------------------------------------

TEST_CASE("doc_string format", "[metaschema][doc_string]") {
  auto schema = load_metaschema();

  SECTION("single-line doc") { expect_valid(schema, app({{"doc", {"Short description"}}})); }

  SECTION("multi-line doc") {
    expect_valid(schema, app({{"doc",
                               {"This tool processes files according to rules.",
                                "Each rule is applied in order.",
                                "",
                                "Rules can be specified via the command line",
                                "or in a configuration file."}}}));
  }

  SECTION("empty array is valid") { expect_valid(schema, app({{"doc", json::array()}})); }

  SECTION("plain string is rejected") {
    json bad = {{"name", "myapp"}, {"doc", "not an array"}};
    expect_invalid(schema, bad);
  }

  SECTION("array with non-string element is rejected") {
    json bad = {{"name", "myapp"}, {"doc", {"ok", 42}}};
    expect_invalid(schema, bad);
  }
}

// ---------------------------------------------------------------------------
// Flag arguments
// ---------------------------------------------------------------------------

TEST_CASE("Flag argument", "[metaschema][args][flag]") {
  auto schema = load_metaschema();

  SECTION("minimal flag") {
    expect_valid(
        schema,
        app({{"args", {{{"kind", "flag"}, {"names", {"verbose"}}, {"doc", {"Be verbose"}}}}}}));
  }

  SECTION("flag with short name alias") {
    expect_valid(schema, app({{"args",
                               {{{"kind", "flag"},
                                 {"names", {"verbose", "v"}},
                                 {"doc", {"Be verbose"}},
                                 {"dest", "verbose"},
                                 {"env", "MYAPP_VERBOSE"}}}}}));
  }

  SECTION("flag with all optional fields") {
    expect_valid(
        schema,
        app({{"args",
              {{{"kind", "flag"},
                {"names", {"verbose", "v"}},
                {"doc", {"Be verbose"}},
                {"dest", "verbose"},
                {"env", {{"var", "MYAPP_VERBOSE"}, {"doc", {"Enable verbose"}}}},
                {"repeated", true},
                {"deprecated", "Use --log-level instead"},
                {"docs", "COMMON OPTIONS"}}}}}));
  }

  SECTION("flag missing names") {
    expect_invalid(schema, app({{"args", {{{"kind", "flag"}, {"doc", {"Be verbose"}}}}}}));
  }

  SECTION("flag missing doc") {
    expect_invalid(schema, app({{"args", {{{"kind", "flag"}, {"names", {"verbose"}}}}}}));
  }

  SECTION("flag with empty names array") {
    expect_invalid(
        schema,
        app({{"args", {{{"kind", "flag"}, {"names", json::array()}, {"doc", {"x"}}}}}}));
  }
}

// ---------------------------------------------------------------------------
// Option arguments
// ---------------------------------------------------------------------------

TEST_CASE("Option argument", "[metaschema][args][option]") {
  auto schema = load_metaschema();

  SECTION("minimal option") {
    expect_valid(schema, app({{"args",
                               {{{"kind", "option"},
                                 {"names", {"output", "o"}},
                                 {"doc", {"Output file"}},
                                 {"type", "string"}}}}}));
  }

  SECTION("option with all fields") {
    expect_valid(schema,
                 app({{"args",
                       {{{"kind", "option"},
                         {"names", {"output", "o"}},
                         {"doc", {"Output file"}},
                         {"docv", "FILE"},
                         {"type", "file"},
                         {"default", "-"},
                         {"required", false},
                         {"repeated", false},
                         {"must_exist", true},
                         {"dest", "output"},
                         {"env", "MYAPP_OUTPUT"},
                         {"docs", "OPTIONS"}}}}}));
  }

  SECTION("option with enum type and choices") {
    expect_valid(schema,
                 app({{"args",
                       {{{"kind", "option"},
                         {"names", {"format"}},
                         {"doc", {"Output format"}},
                         {"type", "enum"},
                         {"choices", {"json", "yaml", "toml"}}}}}}));
  }

  SECTION("option with list type") {
    expect_valid(
        schema,
        app({{"args",
              {{{"kind", "option"},
                {"names", {"includes"}},
                {"doc", {"Include paths"}},
                {"type", {{"list", {{"element", "string"}, {"separator", ","}}}}}}}}}));
  }

  SECTION("option with pair type") {
    expect_valid(
        schema,
        app({{"args",
              {{{"kind", "option"},
                {"names", {"mapping"}},
                {"doc", {"Key-value mapping"}},
                {"type", {{"pair", {{"first", "string"}, {"second", "int"}}}}}}}}}));
  }

  SECTION("option with nested compound type is rejected") {
    expect_invalid(
        schema,
        app({{"args",
              {{{"kind", "option"},
                {"names", {"data"}},
                {"doc", {"Data list"}},
                {"type",
                 {{"list",
                   {{"element",
                     {{"pair", {{"first", "string"}, {"second", "int"}}}}}}}}}}}}}));
  }

  SECTION("option with triple type") {
    expect_valid(
        schema,
        app({{"args",
              {{{"kind", "option"},
                {"names", {"color"}},
                {"doc", {"RGB color"}},
                {"type",
                 {{"triple",
                   {{"first", "int"}, {"second", "int"}, {"third", "int"}}}}}}}}}));
  }

  SECTION("option missing type") {
    expect_invalid(
        schema,
        app({{"args",
              {{{"kind", "option"}, {"names", {"output"}}, {"doc", {"Output file"}}}}}}));
  }

  SECTION("option with invalid scalar type") {
    expect_invalid(schema, app({{"args",
                                 {{{"kind", "option"},
                                   {"names", {"x"}},
                                   {"doc", {"x"}},
                                   {"type", "invalid_type"}}}}}));
  }
}

// ---------------------------------------------------------------------------
// Positional arguments
// ---------------------------------------------------------------------------

TEST_CASE("Positional argument", "[metaschema][args][positional]") {
  auto schema = load_metaschema();

  SECTION("minimal positional") {
    expect_valid(
        schema,
        app({{"args",
              {{{"kind", "positional"},
                {"name", "input"},
                {"doc", {"Input file"}},
                {"type", "string"}}}}}));
  }

  SECTION("positional with all fields") {
    expect_valid(schema,
                 app({{"args",
                       {{{"kind", "positional"},
                         {"name", "input"},
                         {"doc", {"Input files"}},
                         {"docv", "FILE"},
                         {"type", "file"},
                         {"default", nullptr},
                         {"required", true},
                         {"repeated", true},
                         {"must_exist", true},
                         {"docs", "ARGUMENTS"}}}}}));
  }

  SECTION("positional missing name") {
    expect_invalid(
        schema,
        app({{"args",
              {{{"kind", "positional"}, {"doc", {"Input"}}, {"type", "string"}}}}}));
  }

  SECTION("positional missing type") {
    expect_invalid(
        schema,
        app({{"args", {{{"kind", "positional"}, {"name", "input"}, {"doc", {"Input"}}}}}}));
  }
}

// ---------------------------------------------------------------------------
// Flag group arguments
// ---------------------------------------------------------------------------

TEST_CASE("Flag group argument", "[metaschema][args][flag_group]") {
  auto schema = load_metaschema();

  SECTION("valid flag group") {
    expect_valid(
        schema,
        app({{"args",
              {{{"kind", "flag_group"},
                {"dest", "log_level"},
                {"doc", {"Logging level"}},
                {"default", "normal"},
                {"flags",
                 {{{"names", {"quiet", "q"}}, {"doc", {"Quiet"}}, {"value", "quiet"}},
                  {{"names", {"verbose", "v"}},
                   {"doc", {"Verbose"}},
                   {"value", "verbose"}}}}}}}}));
  }

  SECTION("flag group with repeated") {
    expect_valid(
        schema,
        app({{"args",
              {{{"kind", "flag_group"},
                {"dest", "log_level"},
                {"doc", {"Logging level"}},
                {"default", "normal"},
                {"repeated", true},
                {"flags",
                 {{{"names", {"quiet"}}, {"doc", {"Quiet"}}, {"value", "quiet"}},
                  {{"names", {"verbose"}},
                   {"doc", {"Verbose"}},
                   {"value", "verbose"}}}}}}}}));
  }

  SECTION("flag group missing dest") {
    expect_invalid(
        schema,
        app({{"args",
              {{{"kind", "flag_group"},
                {"doc", {"Logging level"}},
                {"default", "normal"},
                {"flags",
                 {{{"names", {"quiet"}}, {"doc", {"Quiet"}}, {"value", "quiet"}}}}}}}}));
  }

  SECTION("flag group missing flags") {
    expect_invalid(schema,
                   app({{"args",
                         {{{"kind", "flag_group"},
                           {"dest", "log_level"},
                           {"doc", {"Logging"}},
                           {"default", "normal"}}}}}));
  }

  SECTION("flag group with empty flags array") {
    expect_invalid(
        schema,
        app({{"args",
              {{{"kind", "flag_group"},
                {"dest", "log_level"},
                {"doc", {"Logging"}},
                {"default", "normal"},
                {"flags", json::array()}}}}}));
  }
}

// ---------------------------------------------------------------------------
// Mixed arguments
// ---------------------------------------------------------------------------

TEST_CASE("Multiple argument kinds together", "[metaschema][args]") {
  auto schema = load_metaschema();

  expect_valid(
      schema,
      app({{"args",
            {{{"kind", "flag"}, {"names", {"verbose", "v"}}, {"doc", {"Verbose"}}},
             {{"kind", "option"},
              {"names", {"output", "o"}},
              {"doc", {"Output"}},
              {"type", "file"}},
             {{"kind", "positional"},
              {"name", "input"},
              {"doc", {"Input"}},
              {"type", "string"}}}}}));
}

// ---------------------------------------------------------------------------
// Subcommands
// ---------------------------------------------------------------------------

TEST_CASE("Subcommands", "[metaschema][commands]") {
  auto schema = load_metaschema();

  SECTION("simple subcommand") {
    expect_valid(
        schema,
        app({{"commands", {{{"name", "build"}, {"doc", {"Build the project"}}}}}}));
  }

  SECTION("nested subcommands") {
    expect_valid(
        schema,
        app({{"commands",
              {{{"name", "remote"},
                {"doc", {"Manage remotes"}},
                {"commands",
                 {{{"name", "add"}, {"doc", {"Add a remote"}}},
                  {{"name", "remove"}, {"doc", {"Remove a remote"}}}}}}}}}));
  }

  SECTION("subcommand with args") {
    expect_valid(
        schema,
        app({{"commands",
              {{{"name", "build"},
                {"doc", {"Build the project"}},
                {"args",
                 {{{"kind", "flag"},
                   {"names", {"release"}},
                   {"doc", {"Release build"}}}}}}}}}));
  }

  SECTION("command with both args and subcommands (group with default)") {
    expect_valid(
        schema,
        app({{"args",
              {{{"kind", "flag"}, {"names", {"verbose"}}, {"doc", {"Verbose"}}}}},
             {"commands", {{{"name", "build"}, {"doc", {"Build"}}}}}}));
  }
}

// ---------------------------------------------------------------------------
// Man page
// ---------------------------------------------------------------------------

TEST_CASE("Man page info", "[metaschema][man]") {
  auto schema = load_metaschema();

  SECTION("man with section number only") {
    expect_valid(schema, app({{"man", {{"section", 1}}}}));
  }

  SECTION("man with sections and blocks") {
    expect_valid(
        schema,
        app({{"man",
              {{"section", 1},
               {"sections",
                {{{"name", "DESCRIPTION"},
                  {"blocks", {{{"paragraph", {"A useful tool."}}}}}},
                 {{"name", "EXAMPLES"},
                  {"blocks",
                   {{{"paragraph", {"Basic usage:"}}},
                    {{"pre",
                      {"$ myapp --verbose input.txt"}}}}}}}}}}}));
  }

  SECTION("man paragraph with multi-line doc_string") {
    json para = {{"paragraph",
                  {"This tool processes files.",
                   "It supports multiple formats.",
                   "",
                   "See EXAMPLES for usage patterns."}}};
    json man = {{"sections", {{{"name", "DESCRIPTION"}, {"blocks", {para}}}}}};
    expect_valid(schema, app({{"man", man}}));
  }

  SECTION("man block with label and text") {
    json block = {{"label", "--verbose"}, {"text", {"Enable verbose output"}}};
    json man = {{"sections", {{{"name", "OPTIONS"}, {"blocks", {block}}}}}};
    expect_valid(schema, app({{"man", man}}));
  }

  SECTION("man block noblank") {
    json man = {{"sections",
                 {{{"name", "DESCRIPTION"},
                   {"blocks", {{{"noblank", true}}, {{"paragraph", {"text"}}}}}}}}};
    expect_valid(schema, app({{"man", man}}));
  }

  SECTION("man with xrefs") {
    expect_valid(schema,
                 app({{"man",
                       {{"xrefs",
                         {{{"name", "git"}, {"section", 1}},
                          {{"name", "mylib"}, {"section", 3}}}}}}}));
  }

  SECTION("invalid man section number") {
    expect_invalid(schema, app({{"man", {{"section", 0}}}}));
    expect_invalid(schema, app({{"man", {{"section", 10}}}}));
  }

  SECTION("man block with unknown key") {
    expect_invalid(
        schema,
        app({{"man",
              {{"sections",
                {{{"name", "DESC"},
                  {"blocks", {{{"unknown_key", "value"}}}}}}}}}}));
  }
}

// ---------------------------------------------------------------------------
// Environment variables (command-level)
// ---------------------------------------------------------------------------

TEST_CASE("Command-level environment variables", "[metaschema][envs]") {
  auto schema = load_metaschema();

  SECTION("valid envs array") {
    expect_valid(
        schema,
        app({{"envs",
              {{{"var", "MYAPP_CONFIG"}, {"doc", {"Path to config file"}}},
               {{"var", "MYAPP_COLOR"}, {"doc", {"Enable colored output"}}}}}}));
  }

  SECTION("env var without doc") {
    expect_valid(schema, app({{"envs", {{{"var", "MYAPP_DEBUG"}}}}}));
  }

  SECTION("invalid env var name") {
    expect_invalid(schema, app({{"envs", {{{"var", "lowercase_var"}}}}}));
  }

  SECTION("env missing var field") {
    expect_invalid(schema, app({{"envs", {{{"doc", {"some doc"}}}}}}));
  }
}

// ---------------------------------------------------------------------------
// Exit codes
// ---------------------------------------------------------------------------

TEST_CASE("Exit code info", "[metaschema][exits]") {
  auto schema = load_metaschema();

  SECTION("valid exits") {
    expect_valid(schema,
                 app({{"exits",
                       {{{"code", 0}, {"doc", {"Success"}}},
                        {{"code", 1}, {"doc", {"General error"}}}}}}));
  }

  SECTION("exit with range") {
    expect_valid(
        schema,
        app({{"exits",
              {{{"code", 1}, {"max", 99}, {"doc", {"Application error"}}}}}}));
  }

  SECTION("exit missing doc") {
    expect_invalid(schema, app({{"exits", {{{"code", 0}}}}}));
  }

  SECTION("exit missing code") {
    expect_invalid(schema, app({{"exits", {{{"doc", {"Success"}}}}}}));
  }

  SECTION("exit code out of range") {
    expect_invalid(schema, app({{"exits", {{{"code", 256}, {"doc", {"Too high"}}}}}}));
    expect_invalid(schema, app({{"exits", {{{"code", -1}, {"doc", {"Negative"}}}}}}));
  }
}

// ---------------------------------------------------------------------------
// Configuration files
// ---------------------------------------------------------------------------

TEST_CASE("Config specification", "[metaschema][config]") {
  auto schema = load_metaschema();

  SECTION("full config") {
    expect_valid(schema,
                 app({{"config",
                       {{"format", "json"},
                        {"paths",
                         {{"system", "/etc/myapp/config.json"},
                          {"user", "~/.config/myapp/config.json"},
                          {"local", ".myapp.json"}}}}}}));
  }

  SECTION("config with format only") {
    expect_valid(schema, app({{"config", {{"format", "yaml"}}}}));
  }

  SECTION("config with partial paths") {
    expect_valid(
        schema,
        app({{"config",
              {{"format", "toml"}, {"paths", {{"local", ".myapp.toml"}}}}}}));
  }

  SECTION("config missing format") {
    expect_invalid(schema, app({{"config", {{"paths", {}}}}}));
  }

  SECTION("invalid format") {
    expect_invalid(schema, app({{"config", {{"format", "xml"}}}}));
  }

  SECTION("config not allowed on subcommands") {
    expect_invalid(
        schema,
        app({{"commands",
              {{{"name", "sub"},
                {"doc", {"A subcommand"}},
                {"config", {{"format", "json"}}}}}}}));
  }
}

// ---------------------------------------------------------------------------
// Cross-cutting: unknown properties rejected
// ---------------------------------------------------------------------------

TEST_CASE("Unknown top-level properties are rejected", "[metaschema]") {
  auto schema = load_metaschema();
  expect_invalid(schema, app({{"unknown_field", "value"}}));
}

TEST_CASE("Unknown argument properties are rejected", "[metaschema][args]") {
  auto schema = load_metaschema();
  expect_invalid(schema,
                 app({{"args",
                       {{{"kind", "flag"},
                         {"names", {"verbose"}},
                         {"doc", {"Verbose"}},
                         {"unknown_field", true}}}}}));
}

// ---------------------------------------------------------------------------
// Integration: realistic CLI schema
// ---------------------------------------------------------------------------

TEST_CASE("Realistic CLI schema validates", "[metaschema][integration]") {
  auto schema = load_metaschema();

  json set_cmd = {
      {"name", "set"},
      {"doc", {"Set a configuration value"}},
      {"args",
       {{{"kind", "positional"},
         {"name", "key"},
         {"doc", {"Configuration key"}},
         {"type", "string"},
         {"required", true}},
        {{"kind", "positional"},
         {"name", "value"},
         {"doc", {"Configuration value"}},
         {"type", "string"},
         {"required", true}}}}};

  json config_cmd = {
      {"name", "config"},
      {"doc", {"Manage configuration"}},
      {"commands",
       {{{"name", "show"}, {"doc", {"Show current configuration"}}}, set_cmd}}};

  json validate_cmd = {
      {"name", "validate"},
      {"doc", {"Validate file format"}},
      {"args",
       {{{"kind", "flag"},
         {"names", {"strict", "s"}},
         {"doc", {"Enable strict mode"}}},
        {{"kind", "positional"},
         {"name", "file"},
         {"doc", {"File to validate"}},
         {"type", "file"},
         {"required", true}}}},
      {"envs", {{{"var", "MYAPP_STRICT"}, {"doc", {"Default strict mode"}}}}}};

  json convert_cmd = {
      {"name", "convert"},
      {"doc", {"Convert files between formats"}},
      {"args",
       {{{"kind", "option"},
         {"names", {"target-format"}},
         {"doc", {"Target format"}},
         {"type", "enum"},
         {"choices", {"json", "yaml", "csv"}}},
        {{"kind", "positional"},
         {"name", "input"},
         {"doc", {"Input files"}},
         {"docv", "FILE"},
         {"type", "file"},
         {"required", true},
         {"repeated", true},
         {"must_exist", true}}}}};

  json realistic = {
      {"name", "myapp"},
      {"version", "2.1.0"},
      {"doc", {"A file processing tool"}},
      {"config",
       {{"format", "json"},
        {"paths",
         {{"system", "/etc/myapp/config.json"},
          {"user", "~/.config/myapp/config.json"},
          {"local", ".myapp.json"}}}}},
      {"man",
       {{"section", 1},
        {"sections",
         {{{"name", "DESCRIPTION"},
           {"blocks",
            {{{"paragraph",
               {"myapp processes files according to rules.",
                "It supports multiple input formats.",
                "",
                "Output can be written to a file or stdout."}}}}}},
          {{"name", "EXAMPLES"},
           {"blocks",
            {{{"paragraph", {"Convert a file:"}}},
             {{"pre", {"$ myapp --format json input.txt"}}}}}}}},
        {"xrefs", {{{"name", "jq"}, {"section", 1}}}}}},
      {"envs",
       {{{"var", "MYAPP_CONFIG"}, {"doc", {"Path to configuration file"}}},
        {{"var", "MYAPP_COLOR"}, {"doc", {"Enable colored output"}}}}},
      {"exits",
       {{{"code", 0}, {"doc", {"Success"}}},
        {{"code", 1}, {"doc", {"General error"}}},
        {{"code", 2}, {"max", 63}, {"doc", {"Application-specific error"}}}}},
      {"args",
       {{{"kind", "flag_group"},
         {"dest", "verbosity"},
         {"doc", {"Set verbosity level"}},
         {"default", "normal"},
         {"flags",
          {{{"names", {"quiet", "q"}}, {"doc", {"Quiet mode"}}, {"value", "quiet"}},
           {{"names", {"verbose", "v"}},
            {"doc", {"Verbose mode"}},
            {"value", "verbose"}},
           {{"names", {"debug"}}, {"doc", {"Debug mode"}}, {"value", "debug"}}}}},
        {{"kind", "option"},
         {"names", {"format", "f"}},
         {"doc", {"Output format"}},
         {"type", "enum"},
         {"choices", {"json", "yaml", "text"}},
         {"default", "text"},
         {"env", "MYAPP_FORMAT"},
         {"docs", "OPTIONS"}},
        {{"kind", "option"},
         {"names", {"output", "o"}},
         {"doc", {"Output file"}},
         {"docv", "FILE"},
         {"type", "file"},
         {"default", "-"},
         {"env", {{"var", "MYAPP_OUTPUT"}, {"doc", {"Default output path"}}}},
         {"docs", "OPTIONS"}},
        {{"kind", "option"},
         {"names", {"tags"}},
         {"doc", {"Comma-separated tags"}},
         {"type", {{"list", {{"element", "string"}, {"separator", ","}}}}},
         {"repeated", false},
         {"docs", "OPTIONS"}}}},
      {"commands", {convert_cmd, validate_cmd, config_cmd}}};

  expect_valid(schema, realistic);
}
