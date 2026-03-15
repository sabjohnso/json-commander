#include <catch2/catch_test_macros.hpp>
#include <json_commander/config_schema.hpp>

using namespace json_commander;
using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Helpers for building model types without triggering -Wmissing-field
// ---------------------------------------------------------------------------

namespace {

  model::Flag
  make_flag(model::ArgNames names) {
    model::Flag f{};
    f.names = std::move(names);
    f.doc = {"doc"};
    return f;
  }

  model::Flag
  make_flag_with_dest(model::ArgNames names, std::string dest) {
    auto f = make_flag(std::move(names));
    f.dest = std::move(dest);
    return f;
  }

  model::Flag
  make_flag_repeated(model::ArgNames names) {
    auto f = make_flag(std::move(names));
    f.repeated = true;
    return f;
  }

  model::Option
  make_option(model::ArgNames names, model::TypeSpec type) {
    model::Option opt{};
    opt.names = std::move(names);
    opt.doc = {"doc"};
    opt.type = std::move(type);
    return opt;
  }

  model::Option
  make_option_with_dest(
    model::ArgNames names, model::TypeSpec type, std::string dest) {
    auto opt = make_option(std::move(names), std::move(type));
    opt.dest = std::move(dest);
    return opt;
  }

  model::Option
  make_option_repeated(model::ArgNames names, model::TypeSpec type) {
    auto opt = make_option(std::move(names), std::move(type));
    opt.repeated = true;
    return opt;
  }

  model::Option
  make_option_with_choices(
    model::ArgNames names,
    model::TypeSpec type,
    std::vector<std::string> choices) {
    auto opt = make_option(std::move(names), std::move(type));
    opt.choices = std::move(choices);
    return opt;
  }

  model::Option
  make_option_required(model::ArgNames names, model::TypeSpec type) {
    auto opt = make_option(std::move(names), std::move(type));
    opt.required = true;
    return opt;
  }

  model::Option
  make_option_with_default(
    model::ArgNames names, model::TypeSpec type, json default_value) {
    auto opt = make_option(std::move(names), std::move(type));
    opt.default_value = std::move(default_value);
    return opt;
  }

  model::Positional
  make_positional(std::string name, model::TypeSpec type) {
    model::Positional pos{};
    pos.name = std::move(name);
    pos.doc = {"doc"};
    pos.type = std::move(type);
    return pos;
  }

  model::Positional
  make_positional_repeated(std::string name, model::TypeSpec type) {
    auto pos = make_positional(std::move(name), std::move(type));
    pos.repeated = true;
    return pos;
  }

  model::Positional
  make_positional_required(std::string name, model::TypeSpec type) {
    auto pos = make_positional(std::move(name), std::move(type));
    pos.required = true;
    return pos;
  }

  model::Positional
  make_positional_with_default(
    std::string name, model::TypeSpec type, json default_value) {
    auto pos = make_positional(std::move(name), std::move(type));
    pos.default_value = std::move(default_value);
    return pos;
  }

  model::FlagGroupEntry
  make_flag_group_entry(model::ArgNames names, json value) {
    model::FlagGroupEntry e{};
    e.names = std::move(names);
    e.doc = {"doc"};
    e.value = std::move(value);
    return e;
  }

  model::FlagGroup
  make_flag_group(
    std::string dest,
    json default_value,
    std::vector<model::FlagGroupEntry> flags) {
    model::FlagGroup g{};
    g.dest = std::move(dest);
    g.doc = {"doc"};
    g.default_value = std::move(default_value);
    g.flags = std::move(flags);
    return g;
  }

  model::FlagGroup
  make_flag_group_repeated(
    std::string dest,
    json default_value,
    std::vector<model::FlagGroupEntry> flags) {
    auto g = make_flag_group(
      std::move(dest), std::move(default_value), std::move(flags));
    g.repeated = true;
    return g;
  }

} // namespace

// ---------------------------------------------------------------------------
// Phase 1: scalar_type_schema
// ---------------------------------------------------------------------------

TEST_CASE(
  "scalar_type_schema: String maps to {type: string}", "[config_schema]") {
  auto result =
    config_schema::detail::scalar_type_schema(model::ScalarType::String);
  REQUIRE(result == json({{"type", "string"}}));
}

TEST_CASE(
  "scalar_type_schema: Int maps to {type: integer}", "[config_schema]") {
  auto result =
    config_schema::detail::scalar_type_schema(model::ScalarType::Int);
  REQUIRE(result == json({{"type", "integer"}}));
}

TEST_CASE(
  "scalar_type_schema: Float maps to {type: number}", "[config_schema]") {
  auto result =
    config_schema::detail::scalar_type_schema(model::ScalarType::Float);
  REQUIRE(result == json({{"type", "number"}}));
}

TEST_CASE(
  "scalar_type_schema: Bool maps to {type: boolean}", "[config_schema]") {
  auto result =
    config_schema::detail::scalar_type_schema(model::ScalarType::Bool);
  REQUIRE(result == json({{"type", "boolean"}}));
}

TEST_CASE(
  "scalar_type_schema: File maps to {type: string}", "[config_schema]") {
  auto result =
    config_schema::detail::scalar_type_schema(model::ScalarType::File);
  REQUIRE(result == json({{"type", "string"}}));
}

TEST_CASE("scalar_type_schema: Dir maps to {type: string}", "[config_schema]") {
  auto result =
    config_schema::detail::scalar_type_schema(model::ScalarType::Dir);
  REQUIRE(result == json({{"type", "string"}}));
}

TEST_CASE(
  "scalar_type_schema: Path maps to {type: string}", "[config_schema]") {
  auto result =
    config_schema::detail::scalar_type_schema(model::ScalarType::Path);
  REQUIRE(result == json({{"type", "string"}}));
}

TEST_CASE(
  "scalar_type_schema: Enum without choices maps to {type: string}",
  "[config_schema]") {
  auto result =
    config_schema::detail::scalar_type_schema(model::ScalarType::Enum);
  REQUIRE(result == json({{"type", "string"}}));
}

TEST_CASE(
  "scalar_type_schema: Enum with choices produces enum array",
  "[config_schema]") {
  std::vector<std::string> choices = {"debug", "release", "profile"};
  auto result =
    config_schema::detail::scalar_type_schema(model::ScalarType::Enum, choices);
  REQUIRE(
    result ==
    json({{"type", "string"}, {"enum", {"debug", "release", "profile"}}}));
}

// ---------------------------------------------------------------------------
// Phase 2: type_spec_schema
// ---------------------------------------------------------------------------

TEST_CASE(
  "type_spec_schema: scalar delegates to scalar_type_schema",
  "[config_schema]") {
  model::TypeSpec ts = model::ScalarType::Int;
  auto result = config_schema::detail::type_spec_schema(ts);
  REQUIRE(result == json({{"type", "integer"}}));
}

TEST_CASE("type_spec_schema: scalar with choices", "[config_schema]") {
  model::TypeSpec ts = model::ScalarType::Enum;
  std::vector<std::string> choices = {"a", "b"};
  auto result = config_schema::detail::type_spec_schema(ts, choices);
  REQUIRE(result == json({{"type", "string"}, {"enum", {"a", "b"}}}));
}

TEST_CASE(
  "type_spec_schema: ListType produces array with items", "[config_schema]") {
  model::ListType lt{};
  lt.element = model::ScalarType::String;
  model::TypeSpec ts = lt;
  auto result = config_schema::detail::type_spec_schema(ts);
  REQUIRE(result == json({{"type", "array"}, {"items", {{"type", "string"}}}}));
}

TEST_CASE(
  "type_spec_schema: PairType produces tuple with items array",
  "[config_schema]") {
  model::PairType pt{};
  pt.first = model::ScalarType::String;
  pt.second = model::ScalarType::Int;
  model::TypeSpec ts = pt;
  auto result = config_schema::detail::type_spec_schema(ts);
  json expected = {
    {"type", "array"},
    {"items", {{{"type", "string"}}, {{"type", "integer"}}}},
    {"additionalItems", false},
    {"minItems", 2},
    {"maxItems", 2}};
  REQUIRE(result == expected);
}

TEST_CASE(
  "type_spec_schema: TripleType produces 3-element tuple", "[config_schema]") {
  model::TripleType tt{};
  tt.first = model::ScalarType::Int;
  tt.second = model::ScalarType::Float;
  tt.third = model::ScalarType::Bool;
  model::TypeSpec ts = tt;
  auto result = config_schema::detail::type_spec_schema(ts);
  json expected = {
    {"type", "array"},
    {"items",
     {{{"type", "integer"}}, {{"type", "number"}}, {{"type", "boolean"}}}},
    {"additionalItems", false},
    {"minItems", 3},
    {"maxItems", 3}};
  REQUIRE(result == expected);
}

// ---------------------------------------------------------------------------
// Phase 3: arg_schema
// ---------------------------------------------------------------------------

TEST_CASE(
  "arg_schema: Flag produces boolean with resolved dest", "[config_schema]") {
  model::Argument arg = make_flag({"v", "verbose"});
  auto [dest, schema] = config_schema::detail::arg_schema(arg);
  REQUIRE(dest == "verbose");
  REQUIRE(schema == json({{"type", "boolean"}}));
}

TEST_CASE("arg_schema: Flag with explicit dest", "[config_schema]") {
  model::Argument arg = make_flag_with_dest({"q", "quiet"}, "be_quiet");
  auto [dest, schema] = config_schema::detail::arg_schema(arg);
  REQUIRE(dest == "be_quiet");
  REQUIRE(schema == json({{"type", "boolean"}}));
}

TEST_CASE(
  "arg_schema: Flag repeated produces integer with minimum 0",
  "[config_schema]") {
  model::Argument arg = make_flag_repeated({"v", "verbose"});
  auto [dest, schema] = config_schema::detail::arg_schema(arg);
  REQUIRE(dest == "verbose");
  REQUIRE(schema == json({{"type", "integer"}, {"minimum", 0}}));
}

TEST_CASE("arg_schema: Option maps to type_spec_schema", "[config_schema]") {
  model::Argument arg = make_option({"o", "output"}, model::ScalarType::String);
  auto [dest, schema] = config_schema::detail::arg_schema(arg);
  REQUIRE(dest == "output");
  REQUIRE(schema == json({{"type", "string"}}));
}

TEST_CASE("arg_schema: Option with explicit dest", "[config_schema]") {
  model::Argument arg = make_option_with_dest(
    {"o", "output"}, model::ScalarType::String, "out_file");
  auto [dest, schema] = config_schema::detail::arg_schema(arg);
  REQUIRE(dest == "out_file");
}

TEST_CASE("arg_schema: Option repeated wraps in array", "[config_schema]") {
  model::Argument arg =
    make_option_repeated({"i", "include"}, model::ScalarType::String);
  auto [dest, schema] = config_schema::detail::arg_schema(arg);
  REQUIRE(dest == "include");
  REQUIRE(schema == json({{"type", "array"}, {"items", {{"type", "string"}}}}));
}

TEST_CASE(
  "arg_schema: Option with choices passes through to type", "[config_schema]") {
  model::Argument arg = make_option_with_choices(
    {"l", "level"}, model::ScalarType::Enum, {"info", "warn", "error"});
  auto [dest, schema] = config_schema::detail::arg_schema(arg);
  REQUIRE(dest == "level");
  REQUIRE(
    schema == json({{"type", "string"}, {"enum", {"info", "warn", "error"}}}));
}

TEST_CASE("arg_schema: Positional uses name as dest", "[config_schema]") {
  model::Argument arg = make_positional("file", model::ScalarType::File);
  auto [dest, schema] = config_schema::detail::arg_schema(arg);
  REQUIRE(dest == "file");
  REQUIRE(schema == json({{"type", "string"}}));
}

TEST_CASE("arg_schema: Positional repeated wraps in array", "[config_schema]") {
  model::Argument arg =
    make_positional_repeated("files", model::ScalarType::File);
  auto [dest, schema] = config_schema::detail::arg_schema(arg);
  REQUIRE(dest == "files");
  REQUIRE(schema == json({{"type", "array"}, {"items", {{"type", "string"}}}}));
}

TEST_CASE("arg_schema: FlagGroup uses group.dest", "[config_schema]") {
  model::Argument arg = make_flag_group(
    "format",
    "text",
    {make_flag_group_entry({"json"}, "json"),
     make_flag_group_entry({"text"}, "text")});
  auto [dest, schema] = config_schema::detail::arg_schema(arg);
  REQUIRE(dest == "format");
  REQUIRE(schema == json::object());
}

TEST_CASE("arg_schema: FlagGroup repeated wraps in array", "[config_schema]") {
  model::Argument arg = make_flag_group_repeated(
    "tags", json::array(), {make_flag_group_entry({"alpha"}, "a")});
  auto [dest, schema] = config_schema::detail::arg_schema(arg);
  REQUIRE(dest == "tags");
  REQUIRE(schema == json({{"type", "array"}}));
}

TEST_CASE(
  "arg_schema: Flag single short name uses it as dest", "[config_schema]") {
  model::Argument arg = make_flag({"v"});
  auto [dest, schema] = config_schema::detail::arg_schema(arg);
  REQUIRE(dest == "v");
  REQUIRE(schema == json({{"type", "boolean"}}));
}

// ---------------------------------------------------------------------------
// Phase 4: is_required
// ---------------------------------------------------------------------------

TEST_CASE(
  "is_required: Flag is always required (defaults to false)",
  "[config_schema]") {
  model::Argument arg = make_flag({"verbose"});
  REQUIRE(config_schema::detail::is_required(arg));
}

TEST_CASE(
  "is_required: Flag repeated is always required (defaults to 0)",
  "[config_schema]") {
  model::Argument arg = make_flag_repeated({"v", "verbose"});
  REQUIRE(config_schema::detail::is_required(arg));
}

TEST_CASE(
  "is_required: FlagGroup is always required (has default_value)",
  "[config_schema]") {
  model::Argument arg = make_flag_group(
    "format", "text", {make_flag_group_entry({"json"}, "json")});
  REQUIRE(config_schema::detail::is_required(arg));
}

TEST_CASE("is_required: Option with required=true", "[config_schema]") {
  model::Argument arg =
    make_option_required({"output"}, model::ScalarType::String);
  REQUIRE(config_schema::detail::is_required(arg));
}

TEST_CASE("is_required: Option with default_value", "[config_schema]") {
  model::Argument arg =
    make_option_with_default({"output"}, model::ScalarType::String, "out.txt");
  REQUIRE(config_schema::detail::is_required(arg));
}

TEST_CASE(
  "is_required: Option without required or default is not required",
  "[config_schema]") {
  model::Argument arg = make_option({"output"}, model::ScalarType::String);
  REQUIRE_FALSE(config_schema::detail::is_required(arg));
}

TEST_CASE("is_required: Positional with required=true", "[config_schema]") {
  model::Argument arg =
    make_positional_required("file", model::ScalarType::File);
  REQUIRE(config_schema::detail::is_required(arg));
}

TEST_CASE("is_required: Positional with default_value", "[config_schema]") {
  model::Argument arg = make_positional_with_default(
    "file", model::ScalarType::File, "default.txt");
  REQUIRE(config_schema::detail::is_required(arg));
}

TEST_CASE(
  "is_required: Positional without required or default is not required",
  "[config_schema]") {
  model::Argument arg = make_positional("file", model::ScalarType::File);
  REQUIRE_FALSE(config_schema::detail::is_required(arg));
}

// ---------------------------------------------------------------------------
// Phase 5: generate
// ---------------------------------------------------------------------------

namespace {

  model::Root
  make_root(std::string name, std::vector<model::Argument> args = {}) {
    model::Root r{};
    r.name = std::move(name);
    r.doc = {"A test program"};
    if (!args.empty()) { r.args = std::move(args); }
    return r;
  }

} // namespace

TEST_CASE("generate: empty args produces minimal schema", "[config_schema]") {
  auto root = make_root("mytool");
  auto args = root.args.value_or(std::vector<model::Argument>{});
  auto schema = config_schema::detail::generate(args, "mytool");
  REQUIRE(schema["$schema"] == "http://json-schema.org/draft-07/schema#");
  REQUIRE(schema["title"] == "mytool configuration");
  REQUIRE(schema["type"] == "object");
  REQUIRE(schema["additionalProperties"] == false);
  REQUIRE(schema["properties"] == json::object());
  REQUIRE(schema["required"] == json::array());
}

TEST_CASE(
  "generate: single flag produces correct property and required",
  "[config_schema]") {
  auto root = make_root("mytool", {make_flag({"verbose"})});
  auto schema = config_schema::detail::generate(*root.args, "mytool");
  REQUIRE(schema["properties"]["verbose"] == json({{"type", "boolean"}}));
  auto req = schema["required"].get<std::vector<std::string>>();
  REQUIRE(std::find(req.begin(), req.end(), "verbose") != req.end());
}

TEST_CASE(
  "generate: optional argument not in required list", "[config_schema]") {
  auto root =
    make_root("mytool", {make_option({"output"}, model::ScalarType::String)});
  auto schema = config_schema::detail::generate(*root.args, "mytool");
  REQUIRE(schema["properties"].contains("output"));
  auto req = schema["required"].get<std::vector<std::string>>();
  REQUIRE(std::find(req.begin(), req.end(), "output") == req.end());
}

TEST_CASE("generate: mixed required and optional args", "[config_schema]") {
  auto root = make_root(
    "mytool",
    {make_flag({"verbose"}),
     make_option({"output"}, model::ScalarType::String),
     make_option_required({"input"}, model::ScalarType::File)});
  auto schema = config_schema::detail::generate(*root.args, "mytool");
  REQUIRE(schema["properties"].contains("verbose"));
  REQUIRE(schema["properties"].contains("output"));
  REQUIRE(schema["properties"].contains("input"));
  auto req = schema["required"].get<std::vector<std::string>>();
  REQUIRE(std::find(req.begin(), req.end(), "verbose") != req.end());
  REQUIRE(std::find(req.begin(), req.end(), "input") != req.end());
  REQUIRE(std::find(req.begin(), req.end(), "output") == req.end());
}

// ---------------------------------------------------------------------------
// Phase 6: to_config_schema
// ---------------------------------------------------------------------------

namespace {

  model::Command
  make_command(std::string name, std::vector<model::Argument> args = {}) {
    model::Command c{};
    c.name = std::move(name);
    c.doc = {"A test command"};
    if (!args.empty()) { c.args = std::move(args); }
    return c;
  }

  model::Root
  make_root_with_commands(
    std::string name,
    std::vector<model::Argument> args,
    std::vector<model::Command> commands) {
    auto r = make_root(std::move(name), std::move(args));
    r.commands = std::move(commands);
    return r;
  }

} // namespace

TEST_CASE("to_config_schema: root with no path", "[config_schema]") {
  auto root = make_root("mytool", {make_flag({"verbose"})});
  auto schema = config_schema::to_config_schema(root);
  REQUIRE(schema["title"] == "mytool configuration");
  REQUIRE(schema["properties"].contains("verbose"));
}

TEST_CASE("to_config_schema: root with empty path", "[config_schema]") {
  auto root = make_root("mytool", {make_flag({"verbose"})});
  auto schema = config_schema::to_config_schema(root, {});
  REQUIRE(schema["title"] == "mytool configuration");
  REQUIRE(schema["properties"].contains("verbose"));
}

TEST_CASE(
  "to_config_schema: subcommand path uses definitions and const discriminant",
  "[config_schema]") {
  auto build_cmd =
    make_command("build", {make_option({"target"}, model::ScalarType::String)});
  auto root =
    make_root_with_commands("mytool", {make_flag({"verbose"})}, {build_cmd});
  auto schema = config_schema::to_config_schema(root, {"build"});
  REQUIRE(schema["title"] == "mytool-build configuration");
  REQUIRE(schema.contains("definitions"));
  REQUIRE(schema["definitions"].contains("build"));
  REQUIRE(schema["definitions"]["build"]["properties"].contains("target"));
  REQUIRE(schema["properties"].contains("verbose"));
  REQUIRE(schema["properties"]["command"]["const"] == "build");
  REQUIRE(schema["properties"]["build"]["$ref"] == "#/definitions/build");
  REQUIRE(schema["additionalProperties"] == false);
}

TEST_CASE(
  "to_config_schema: nested path produces qualified definitions",
  "[config_schema]") {
  auto push_cmd = make_command("push", {make_flag({"force"})});
  auto stash_cmd = make_command(
    "stash", {make_option({"message"}, model::ScalarType::String)});
  stash_cmd.commands = std::vector<model::Command>{push_cmd};
  auto root =
    make_root_with_commands("git", {make_flag({"verbose"})}, {stash_cmd});
  auto schema = config_schema::to_config_schema(root, {"stash", "push"});
  REQUIRE(schema["title"] == "git-stash-push configuration");
  REQUIRE(schema.contains("definitions"));
  REQUIRE(schema["definitions"].contains("stash"));
  REQUIRE(schema["definitions"].contains("stash.push"));
  REQUIRE(schema["definitions"]["stash.push"]["properties"].contains("force"));
  REQUIRE(schema["properties"]["command"]["const"] == "stash");
  REQUIRE(schema["properties"]["stash"]["$ref"] == "#/definitions/stash");
}

TEST_CASE(
  "to_config_schema: root with commands uses discriminated union",
  "[config_schema]") {
  auto build_cmd =
    make_command("build", {make_option({"target"}, model::ScalarType::String)});
  auto init_cmd = make_command("init");
  auto root = make_root_with_commands(
    "mytool", {make_flag({"verbose"})}, {build_cmd, init_cmd});
  auto schema = config_schema::to_config_schema(root);
  REQUIRE(schema.contains("oneOf"));
  REQUIRE(schema["oneOf"].size() == 2);
  REQUIRE(schema.contains("definitions"));
  REQUIRE(schema["definitions"].contains("build"));
  REQUIRE(schema["definitions"].contains("init"));
  REQUIRE(schema["oneOf"][0]["properties"]["command"]["const"] == "build");
  REQUIRE(
    schema["oneOf"][0]["properties"]["build"]["$ref"] == "#/definitions/build");
  REQUIRE(schema["oneOf"][0]["additionalProperties"] == false);
  REQUIRE(schema["oneOf"][1]["properties"]["command"]["const"] == "init");
  REQUIRE(
    schema["oneOf"][1]["properties"]["init"]["$ref"] == "#/definitions/init");
}

TEST_CASE("to_config_schema: nonexistent path throws", "[config_schema]") {
  auto root = make_root("mytool");
  REQUIRE_THROWS_AS(
    config_schema::to_config_schema(root, {"nonexistent"}), std::runtime_error);
}

TEST_CASE(
  "to_config_schema: root with no args and no path", "[config_schema]") {
  model::Root r{};
  r.name = "bare";
  r.doc = {"bare tool"};
  auto schema = config_schema::to_config_schema(r);
  REQUIRE(schema["properties"] == json::object());
  REQUIRE(schema["required"] == json::array());
}

// ---------------------------------------------------------------------------
// Phase 7: Integration — validate config against generated schema
// ---------------------------------------------------------------------------

#include <nlohmann/json-schema.hpp>

namespace {

  void
  validate_config(const json& schema, const json& config) {
    nlohmann::json_schema::json_validator validator;
    validator.set_root_schema(schema);
    validator.validate(config);
  }

} // namespace

TEST_CASE(
  "integration: valid config passes schema validation",
  "[config_schema][integration]") {
  auto root = make_root(
    "mytool",
    {make_flag({"verbose"}),
     make_option_with_default({"output"}, model::ScalarType::String, "out.txt"),
     make_positional_required("file", model::ScalarType::File)});
  auto schema = config_schema::to_config_schema(root);

  json config = {
    {"verbose", true}, {"output", "result.txt"}, {"file", "input.txt"}};
  REQUIRE_NOTHROW(validate_config(schema, config));
}

TEST_CASE(
  "integration: config missing required field fails validation",
  "[config_schema][integration]") {
  auto root = make_root(
    "mytool",
    {make_flag({"verbose"}),
     make_positional_required("file", model::ScalarType::File)});
  auto schema = config_schema::to_config_schema(root);

  json config = {{"verbose", true}};
  REQUIRE_THROWS(validate_config(schema, config));
}

TEST_CASE(
  "integration: config with wrong type fails validation",
  "[config_schema][integration]") {
  auto root = make_root("mytool", {make_flag({"verbose"})});
  auto schema = config_schema::to_config_schema(root);

  json config = {{"verbose", "not_a_bool"}};
  REQUIRE_THROWS(validate_config(schema, config));
}

TEST_CASE(
  "integration: config with extra property fails validation",
  "[config_schema][integration]") {
  auto root = make_root("mytool", {make_flag({"verbose"})});
  auto schema = config_schema::to_config_schema(root);

  json config = {{"verbose", true}, {"extra", "field"}};
  REQUIRE_THROWS(validate_config(schema, config));
}

TEST_CASE(
  "integration: enum option validates choices",
  "[config_schema][integration]") {
  auto root = make_root(
    "mytool",
    {make_option_with_choices(
      {"level"}, model::ScalarType::Enum, {"debug", "info", "warn"})});
  auto schema = config_schema::to_config_schema(root);

  json valid = {{"level", "debug"}};
  REQUIRE_NOTHROW(validate_config(schema, valid));

  json invalid = {{"level", "invalid_value"}};
  REQUIRE_THROWS(validate_config(schema, invalid));
}

TEST_CASE(
  "integration: repeated option validates as array",
  "[config_schema][integration]") {
  auto root = make_root(
    "mytool", {make_option_repeated({"include"}, model::ScalarType::String)});
  auto schema = config_schema::to_config_schema(root);

  json valid = {{"include", json::array({"path1", "path2"})}};
  REQUIRE_NOTHROW(validate_config(schema, valid));

  json invalid = {{"include", "not_an_array"}};
  REQUIRE_THROWS(validate_config(schema, invalid));
}

TEST_CASE(
  "integration: subcommand schema validates nested args",
  "[config_schema][integration]") {
  auto build_cmd = make_command(
    "build", {make_option_required({"target"}, model::ScalarType::String)});
  auto root =
    make_root_with_commands("mytool", {make_flag({"verbose"})}, {build_cmd});
  auto schema = config_schema::to_config_schema(root, {"build"});

  json valid = {
    {"verbose", false},
    {"command", "build"},
    {"build", {{"target", "x86_64"}}}};
  REQUIRE_NOTHROW(validate_config(schema, valid));

  json missing_root_arg = {
    {"command", "build"}, {"build", {{"target", "x86_64"}}}};
  REQUIRE_THROWS(validate_config(schema, missing_root_arg));

  json missing_cmd_arg = {
    {"verbose", false}, {"command", "build"}, {"build", json::object()}};
  REQUIRE_THROWS(validate_config(schema, missing_cmd_arg));
}

TEST_CASE(
  "integration: discriminated union rejects wrong subcommand properties",
  "[config_schema][integration]") {
  auto build_cmd = make_command(
    "build",
    {make_option_with_default({"target"}, model::ScalarType::String, "debug")});
  auto init_cmd = make_command("init");
  auto root = make_root_with_commands(
    "mytool", {make_flag({"verbose"})}, {build_cmd, init_cmd});
  auto schema = config_schema::to_config_schema(root);

  json valid = {
    {"verbose", false},
    {"command", "build"},
    {"build", {{"target", "release"}}}};
  REQUIRE_NOTHROW(validate_config(schema, valid));

  json has_extra_subcmd = {
    {"verbose", false},
    {"command", "build"},
    {"build", {{"target", "release"}}},
    {"init", json::object()}};
  REQUIRE_THROWS(validate_config(schema, has_extra_subcmd));
}

TEST_CASE(
  "integration: integer option validates correctly",
  "[config_schema][integration]") {
  auto root = make_root(
    "mytool", {make_option_with_default({"jobs"}, model::ScalarType::Int, 4)});
  auto schema = config_schema::to_config_schema(root);

  json valid = {{"jobs", 8}};
  REQUIRE_NOTHROW(validate_config(schema, valid));

  json invalid = {{"jobs", "not_an_int"}};
  REQUIRE_THROWS(validate_config(schema, invalid));
}

// ---------------------------------------------------------------------------
// Phase 8: Round-trip — parse real argv, validate output against schema
// ---------------------------------------------------------------------------

#include <json_commander/cmd.hpp>
#include <json_commander/parse.hpp>

namespace {

  json
  parse_to_config(
    const model::Root& root,
    std::vector<std::string> argv,
    parse::EnvLookup env = parse::no_env()) {
    auto spec = cmd::make(root);
    auto result = parse::parse(spec, argv, std::move(env));
    return std::get<parse::ParseOk>(result).config;
  }

  parse::EnvLookup
  make_env(std::initializer_list<std::pair<std::string, std::string>> pairs) {
    auto map = std::make_shared<std::unordered_map<std::string, std::string>>();
    for (const auto& p : pairs) {
      map->emplace(p.first, p.second);
    }
    return [map](const std::string& var) -> std::optional<std::string> {
      auto it = map->find(var);
      if (it == map->end()) { return std::nullopt; }
      return it->second;
    };
  }

} // namespace

TEST_CASE(
  "round-trip: root-only CLI config validates against schema",
  "[config_schema][round_trip]") {
  auto root = make_root(
    "mytool",
    {make_flag({"verbose"}),
     make_option_with_default({"output"}, model::ScalarType::String, "out.txt"),
     make_positional_required("file", model::ScalarType::File)});
  auto schema = config_schema::to_config_schema(root);
  auto config = parse_to_config(root, {"--verbose", "--output", "x.txt", "f"});
  REQUIRE_NOTHROW(validate_config(schema, config));
}

TEST_CASE(
  "round-trip: root-only defaults validate against schema",
  "[config_schema][round_trip]") {
  auto root = make_root(
    "mytool",
    {make_flag({"verbose"}),
     make_option_with_default(
       {"output"}, model::ScalarType::String, "out.txt")});
  auto schema = config_schema::to_config_schema(root);
  auto config = parse_to_config(root, {});
  REQUIRE_NOTHROW(validate_config(schema, config));
}

TEST_CASE(
  "round-trip: single subcommand config validates against schema",
  "[config_schema][round_trip]") {
  auto build_cmd = make_command(
    "build",
    {make_option_with_default({"target"}, model::ScalarType::String, "debug")});
  auto root =
    make_root_with_commands("mytool", {make_flag({"verbose"})}, {build_cmd});
  auto schema = config_schema::to_config_schema(root, {"build"});
  auto config =
    parse_to_config(root, {"--verbose", "build", "--target", "release"});
  REQUIRE_NOTHROW(validate_config(schema, config));
}

TEST_CASE(
  "round-trip: subcommand with defaults validates against schema",
  "[config_schema][round_trip]") {
  auto build_cmd = make_command(
    "build",
    {make_option_with_default({"target"}, model::ScalarType::String, "debug")});
  auto root =
    make_root_with_commands("mytool", {make_flag({"verbose"})}, {build_cmd});
  auto schema = config_schema::to_config_schema(root, {"build"});
  auto config = parse_to_config(root, {"build"});
  REQUIRE_NOTHROW(validate_config(schema, config));
}

TEST_CASE(
  "round-trip: nested subcommands validate against schema",
  "[config_schema][round_trip]") {
  auto set_cmd = make_command(
    "set",
    {make_positional_required("key", model::ScalarType::String),
     make_positional_required("value", model::ScalarType::String)});
  auto config_cmd = make_command("config");
  config_cmd.commands = std::vector<model::Command>{set_cmd};
  auto root =
    make_root_with_commands("tool", {make_flag({"verbose"})}, {config_cmd});
  auto schema = config_schema::to_config_schema(root, {"config", "set"});
  auto config = parse_to_config(root, {"config", "set", "user.name", "Alice"});
  REQUIRE_NOTHROW(validate_config(schema, config));
}

TEST_CASE(
  "round-trip: env fallback config validates against schema",
  "[config_schema][round_trip]") {
  auto opt =
    make_option_with_default({"target"}, model::ScalarType::String, "debug");
  opt.env = model::EnvBinding{std::string("BUILD_TARGET")};
  auto build_cmd = make_command("build", {opt});
  auto root =
    make_root_with_commands("mytool", {make_flag({"verbose"})}, {build_cmd});
  auto schema = config_schema::to_config_schema(root, {"build"});
  auto config =
    parse_to_config(root, {"build"}, make_env({{"BUILD_TARGET", "release"}}));
  REQUIRE_NOTHROW(validate_config(schema, config));
}

TEST_CASE(
  "round-trip: root-only schema (no path) validates subcommand config",
  "[config_schema][round_trip]") {
  auto build_cmd = make_command(
    "build",
    {make_option_with_default({"target"}, model::ScalarType::String, "debug")});
  auto init_cmd = make_command("init");
  auto root = make_root_with_commands(
    "mytool", {make_flag({"verbose"})}, {build_cmd, init_cmd});
  auto schema = config_schema::to_config_schema(root);
  auto config =
    parse_to_config(root, {"--verbose", "build", "--target", "release"});
  REQUIRE_NOTHROW(validate_config(schema, config));
}
