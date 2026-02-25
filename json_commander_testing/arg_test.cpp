#include <catch2/catch_test_macros.hpp>
#include <json_commander/arg.hpp>

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

  model::FlagGroup
  make_flag_group(
    std::string dest, std::vector<model::FlagGroupEntry> flags = {}) {
    model::FlagGroup g{};
    g.dest = std::move(dest);
    g.doc = {"doc"};
    g.default_value = json("default");
    g.flags = std::move(flags);
    return g;
  }

  model::FlagGroupEntry
  make_flag_group_entry(model::ArgNames names, json value) {
    model::FlagGroupEntry e{};
    e.names = std::move(names);
    e.doc = {"doc"};
    e.value = std::move(value);
    return e;
  }

  model::Option
  make_option(model::ArgNames names, model::TypeSpec type) {
    model::Option opt{};
    opt.names = std::move(names);
    opt.doc = {"doc"};
    opt.type = std::move(type);
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

} // namespace

// ---------------------------------------------------------------------------
// Phase 1: EnvSpec and resolve helpers
// ---------------------------------------------------------------------------

TEST_CASE("EnvSpec equality for identical structs", "[arg]") {
  arg::EnvSpec a{"MY_VAR", std::nullopt};
  arg::EnvSpec b{"MY_VAR", std::nullopt};
  REQUIRE(a == b);
}

TEST_CASE("EnvSpec inequality for different var", "[arg]") {
  arg::EnvSpec a{"MY_VAR", std::nullopt};
  arg::EnvSpec b{"OTHER_VAR", std::nullopt};
  REQUIRE_FALSE(a == b);
}

TEST_CASE("EnvSpec equality with doc", "[arg]") {
  model::DocString doc = {"help text"};
  arg::EnvSpec a{"MY_VAR", doc};
  arg::EnvSpec b{"MY_VAR", doc};
  REQUIRE(a == b);
}

TEST_CASE("resolve_env from string produces EnvSpec with no doc", "[arg]") {
  model::EnvBinding binding = std::string("MY_VAR");
  auto spec = arg::detail::resolve_env(binding);
  REQUIRE(spec == arg::EnvSpec{"MY_VAR", std::nullopt});
}

TEST_CASE("resolve_env from EnvBindingObj produces EnvSpec with doc", "[arg]") {
  model::DocString doc = {"help text"};
  model::EnvBinding binding = model::EnvBindingObj{"MY_VAR", doc};
  auto spec = arg::detail::resolve_env(binding);
  REQUIRE(spec == arg::EnvSpec{"MY_VAR", doc});
}

TEST_CASE("resolve_env_opt from nullopt produces nullopt", "[arg]") {
  auto result = arg::detail::resolve_env_opt(std::nullopt);
  REQUIRE_FALSE(result.has_value());
}

TEST_CASE("resolve_env_opt from string EnvBinding produces EnvSpec", "[arg]") {
  std::optional<model::EnvBinding> binding = std::string("MY_VAR");
  auto result = arg::detail::resolve_env_opt(binding);
  REQUIRE(result.has_value());
  REQUIRE(*result == arg::EnvSpec{"MY_VAR", std::nullopt});
}

// ---------------------------------------------------------------------------
// Phase 2: resolve_dest helper
// ---------------------------------------------------------------------------

TEST_CASE("resolve_dest picks first long name", "[arg]") {
  REQUIRE(arg::detail::resolve_dest({"verbose", "v"}) == "verbose");
}

TEST_CASE("resolve_dest skips short names to find long name", "[arg]") {
  REQUIRE(arg::detail::resolve_dest({"v", "verbose"}) == "verbose");
}

TEST_CASE("resolve_dest falls back to first name when all short", "[arg]") {
  REQUIRE(arg::detail::resolve_dest({"v"}) == "v");
}

TEST_CASE("resolve_dest with single long name", "[arg]") {
  REQUIRE(arg::detail::resolve_dest({"output"}) == "output");
}

// ---------------------------------------------------------------------------
// Phase 3: FlagSpec factory
// ---------------------------------------------------------------------------

TEST_CASE("make(Flag) resolves dest from first long name", "[arg]") {
  auto flag = make_flag({"verbose", "v"});
  auto spec = arg::make(flag);
  REQUIRE(spec.dest == "verbose");
}

TEST_CASE("make(Flag) uses explicit dest when provided", "[arg]") {
  auto flag = make_flag({"verbose", "v"});
  flag.dest = "my_dest";
  auto spec = arg::make(flag);
  REQUIRE(spec.dest == "my_dest");
}

TEST_CASE("make(Flag) defaults repeated to false", "[arg]") {
  auto flag = make_flag({"verbose"});
  auto spec = arg::make(flag);
  REQUIRE(spec.repeated == false);
}

TEST_CASE("make(Flag) preserves repeated true", "[arg]") {
  auto flag = make_flag({"verbose"});
  flag.repeated = true;
  auto spec = arg::make(flag);
  REQUIRE(spec.repeated == true);
}

TEST_CASE("make(Flag) resolves env from string", "[arg]") {
  auto flag = make_flag({"verbose"});
  flag.env = std::string("VERBOSE");
  auto spec = arg::make(flag);
  REQUIRE(spec.env.has_value());
  REQUIRE(*spec.env == arg::EnvSpec{"VERBOSE", std::nullopt});
}

TEST_CASE("make(Flag) resolves env from EnvBindingObj", "[arg]") {
  model::DocString doc = {"env help"};
  auto flag = make_flag({"verbose"});
  flag.env = model::EnvBindingObj{"VERBOSE", doc};
  auto spec = arg::make(flag);
  REQUIRE(spec.env.has_value());
  REQUIRE(*spec.env == arg::EnvSpec{"VERBOSE", doc});
}

TEST_CASE("make(Flag) with no env produces nullopt", "[arg]") {
  auto flag = make_flag({"verbose"});
  auto spec = arg::make(flag);
  REQUIRE_FALSE(spec.env.has_value());
}

TEST_CASE("make(Flag) preserves deprecated", "[arg]") {
  auto flag = make_flag({"old-flag"});
  flag.deprecated = "Use --new-flag instead";
  auto spec = arg::make(flag);
  REQUIRE(spec.deprecated.has_value());
  REQUIRE(*spec.deprecated == "Use --new-flag instead");
}

TEST_CASE("make(Flag) with no deprecated produces nullopt", "[arg]") {
  auto flag = make_flag({"verbose"});
  auto spec = arg::make(flag);
  REQUIRE_FALSE(spec.deprecated.has_value());
}

TEST_CASE("make(Flag) preserves names", "[arg]") {
  auto flag = make_flag({"verbose", "v"});
  auto spec = arg::make(flag);
  REQUIRE(spec.names == model::ArgNames{"verbose", "v"});
}

// ---------------------------------------------------------------------------
// Phase 4: FlagGroupSpec factory
// ---------------------------------------------------------------------------

TEST_CASE("make(FlagGroup) carries dest directly", "[arg]") {
  auto group = make_flag_group("format");
  auto spec = arg::make(group);
  REQUIRE(spec.dest == "format");
}

TEST_CASE("make(FlagGroup) carries default_value", "[arg]") {
  auto group = make_flag_group("format");
  group.default_value = json("text");
  auto spec = arg::make(group);
  REQUIRE(spec.default_value == json("text"));
}

TEST_CASE("make(FlagGroup) converts entries dropping doc", "[arg]") {
  auto e1 = make_flag_group_entry({"json", "j"}, json("json"));
  auto e2 = make_flag_group_entry({"text", "t"}, json("text"));
  auto group = make_flag_group("format", {e1, e2});
  auto spec = arg::make(group);
  REQUIRE(spec.entries.size() == 2);
  REQUIRE(spec.entries[0].names == model::ArgNames{"json", "j"});
  REQUIRE(spec.entries[0].value == json("json"));
  REQUIRE(spec.entries[1].names == model::ArgNames{"text", "t"});
  REQUIRE(spec.entries[1].value == json("text"));
}

TEST_CASE("make(FlagGroup) defaults repeated to false", "[arg]") {
  auto group = make_flag_group("format");
  auto spec = arg::make(group);
  REQUIRE(spec.repeated == false);
}

TEST_CASE("make(FlagGroup) preserves repeated true", "[arg]") {
  auto group = make_flag_group("format");
  group.repeated = true;
  auto spec = arg::make(group);
  REQUIRE(spec.repeated == true);
}

// ---------------------------------------------------------------------------
// Phase 5: OptionSpec factory
// ---------------------------------------------------------------------------

TEST_CASE("make(Option) resolves dest from first long name", "[arg]") {
  auto opt = make_option({"output", "o"}, model::ScalarType::String);
  auto spec = arg::make(opt);
  REQUIRE(spec.dest == "output");
}

TEST_CASE("make(Option) uses explicit dest when provided", "[arg]") {
  auto opt = make_option({"output", "o"}, model::ScalarType::String);
  opt.dest = "out_path";
  auto spec = arg::make(opt);
  REQUIRE(spec.dest == "out_path");
}

TEST_CASE(
  "make(Option) creates converter from type — parse 42 for Int", "[arg]") {
  auto opt = make_option({"count"}, model::ScalarType::Int);
  auto spec = arg::make(opt);
  REQUIRE(spec.converter.parse("42") == json(42));
}

TEST_CASE(
  "make(Option) creates validator — required rejects nullopt", "[arg]") {
  auto opt = make_option({"count"}, model::ScalarType::Int);
  opt.required = true;
  auto spec = arg::make(opt);
  REQUIRE_THROWS_AS(
    spec.validator.check("--count", std::nullopt), validate::Error);
}

TEST_CASE("make(Option) carries default_value", "[arg]") {
  auto opt = make_option({"count"}, model::ScalarType::Int);
  opt.default_value = json(10);
  auto spec = arg::make(opt);
  REQUIRE(spec.default_value.has_value());
  REQUIRE(*spec.default_value == json(10));
}

TEST_CASE("make(Option) defaults repeated to false", "[arg]") {
  auto opt = make_option({"count"}, model::ScalarType::Int);
  auto spec = arg::make(opt);
  REQUIRE(spec.repeated == false);
}

TEST_CASE("make(Option) resolves env", "[arg]") {
  auto opt = make_option({"count"}, model::ScalarType::Int);
  opt.env = std::string("COUNT");
  auto spec = arg::make(opt);
  REQUIRE(spec.env.has_value());
  REQUIRE(*spec.env == arg::EnvSpec{"COUNT", std::nullopt});
}

TEST_CASE(
  "make(Option) with enum type and choices validates choices", "[arg]") {
  auto opt = make_option({"format"}, model::ScalarType::Enum);
  opt.choices = std::vector<std::string>{"json", "text", "csv"};
  auto spec = arg::make(opt);
  REQUIRE(spec.converter.parse("json") == json("json"));
  REQUIRE_THROWS_AS(spec.converter.parse("xml"), conv::Error);
}

TEST_CASE("make(Option) preserves names", "[arg]") {
  auto opt = make_option({"output", "o"}, model::ScalarType::String);
  auto spec = arg::make(opt);
  REQUIRE(spec.names == model::ArgNames{"output", "o"});
}

// ---------------------------------------------------------------------------
// Phase 6: PositionalSpec factory
// ---------------------------------------------------------------------------

TEST_CASE("make(Positional) uses name as dest", "[arg]") {
  auto pos = make_positional("FILE", model::ScalarType::String);
  auto spec = arg::make(pos);
  REQUIRE(spec.dest == "FILE");
}

TEST_CASE(
  "make(Positional) creates converter — parse hello for String", "[arg]") {
  auto pos = make_positional("FILE", model::ScalarType::String);
  auto spec = arg::make(pos);
  REQUIRE(spec.converter.parse("hello") == json("hello"));
}

TEST_CASE(
  "make(Positional) creates validator — required rejects nullopt", "[arg]") {
  auto pos = make_positional("FILE", model::ScalarType::String);
  pos.required = true;
  auto spec = arg::make(pos);
  REQUIRE_THROWS_AS(
    spec.validator.check("FILE", std::nullopt), validate::Error);
}

TEST_CASE("make(Positional) carries default_value", "[arg]") {
  auto pos = make_positional("FILE", model::ScalarType::String);
  pos.default_value = json("default.txt");
  auto spec = arg::make(pos);
  REQUIRE(spec.default_value.has_value());
  REQUIRE(*spec.default_value == json("default.txt"));
}

TEST_CASE("make(Positional) defaults repeated to false", "[arg]") {
  auto pos = make_positional("FILE", model::ScalarType::String);
  auto spec = arg::make(pos);
  REQUIRE(spec.repeated == false);
}

TEST_CASE("make(Positional) preserves name", "[arg]") {
  auto pos = make_positional("FILE", model::ScalarType::String);
  auto spec = arg::make(pos);
  REQUIRE(spec.name == "FILE");
}

// ---------------------------------------------------------------------------
// Phase 7: ArgSpec variant and make_all
// ---------------------------------------------------------------------------

TEST_CASE("make(Argument) with Flag holds FlagSpec", "[arg]") {
  model::Argument argument = make_flag({"verbose"});
  auto spec = arg::make(argument);
  REQUIRE(std::holds_alternative<arg::FlagSpec>(spec));
}

TEST_CASE("make(Argument) with FlagGroup holds FlagGroupSpec", "[arg]") {
  model::Argument argument = make_flag_group("fmt");
  auto spec = arg::make(argument);
  REQUIRE(std::holds_alternative<arg::FlagGroupSpec>(spec));
}

TEST_CASE("make(Argument) with Option holds OptionSpec", "[arg]") {
  model::Argument argument = make_option({"output"}, model::ScalarType::String);
  auto spec = arg::make(argument);
  REQUIRE(std::holds_alternative<arg::OptionSpec>(spec));
}

TEST_CASE("make(Argument) with Positional holds PositionalSpec", "[arg]") {
  model::Argument argument = make_positional("FILE", model::ScalarType::String);
  auto spec = arg::make(argument);
  REQUIRE(std::holds_alternative<arg::PositionalSpec>(spec));
}

TEST_CASE("make_all converts mixed vector preserving order", "[arg]") {
  std::vector<model::Argument> args = {
    make_flag({"verbose"}),
    make_option({"output"}, model::ScalarType::String),
    make_positional("FILE", model::ScalarType::String),
  };
  auto specs = arg::make_all(args);
  REQUIRE(specs.size() == 3);
  REQUIRE(std::holds_alternative<arg::FlagSpec>(specs[0]));
  REQUIRE(std::holds_alternative<arg::OptionSpec>(specs[1]));
  REQUIRE(std::holds_alternative<arg::PositionalSpec>(specs[2]));
}

TEST_CASE("make_all with empty vector produces empty vector", "[arg]") {
  std::vector<model::Argument> args;
  auto specs = arg::make_all(args);
  REQUIRE(specs.empty());
}

// ---------------------------------------------------------------------------
// Phase 8: Integration tests
// ---------------------------------------------------------------------------

TEST_CASE("Option with all fields set resolves completely", "[arg]") {
  model::DocString env_doc = {"Counter env var"};
  auto opt = make_option({"count", "c"}, model::ScalarType::Int);
  opt.dest = "counter";
  opt.default_value = json(0);
  opt.required = true;
  opt.repeated = true;
  opt.env = model::EnvBindingObj{"COUNTER", env_doc};

  auto spec = arg::make(opt);

  REQUIRE(spec.names == model::ArgNames{"count", "c"});
  REQUIRE(spec.dest == "counter");
  REQUIRE(spec.converter.parse("42") == json(42));
  REQUIRE_THROWS_AS(
    spec.validator.check("--count", std::nullopt), validate::Error);
  REQUIRE(spec.default_value == json(0));
  REQUIRE(spec.repeated == true);
  REQUIRE(spec.env.has_value());
  REQUIRE(*spec.env == arg::EnvSpec{"COUNTER", env_doc});
}

TEST_CASE("Flag with only required fields uses all defaults", "[arg]") {
  auto flag = make_flag({"debug"});
  auto spec = arg::make(flag);

  REQUIRE(spec.names == model::ArgNames{"debug"});
  REQUIRE(spec.dest == "debug");
  REQUIRE(spec.repeated == false);
  REQUIRE_FALSE(spec.env.has_value());
  REQUIRE_FALSE(spec.deprecated.has_value());
}

TEST_CASE("make_all preserves argument order", "[arg]") {
  std::vector<model::Argument> args = {
    make_positional("FILE", model::ScalarType::String),
    make_flag({"verbose"}),
    make_flag_group("fmt"),
    make_option({"output"}, model::ScalarType::String),
  };
  auto specs = arg::make_all(args);
  REQUIRE(specs.size() == 4);
  REQUIRE(std::holds_alternative<arg::PositionalSpec>(specs[0]));
  REQUIRE(std::holds_alternative<arg::FlagSpec>(specs[1]));
  REQUIRE(std::holds_alternative<arg::FlagGroupSpec>(specs[2]));
  REQUIRE(std::holds_alternative<arg::OptionSpec>(specs[3]));
}

TEST_CASE("Converter round-trip through OptionSpec", "[arg]") {
  auto opt = make_option({"count"}, model::ScalarType::Int);
  auto spec = arg::make(opt);
  auto parsed = spec.converter.parse("42");
  auto formatted = spec.converter.format(parsed);
  REQUIRE(formatted == "42");
}
