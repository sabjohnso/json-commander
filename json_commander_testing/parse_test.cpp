#include <catch2/catch_test_macros.hpp>
#include <json_commander/parse.hpp>

using namespace json_commander;
using json = nlohmann::json;

// ===========================================================================
// Phase 1: Types, Error, and Result
// ===========================================================================

TEST_CASE("parse::Error carries message", "[parse][phase1]") {
  parse::Error err("something went wrong");
  REQUIRE(std::string(err.what()) == "something went wrong");
}

TEST_CASE("parse::Error is catchable as std::runtime_error", "[parse][phase1]") {
  bool caught = false;
  try {
    throw parse::Error("test");
  } catch (const std::runtime_error &e) {
    caught = true;
    REQUIRE(std::string(e.what()) == "test");
  }
  REQUIRE(caught);
}

TEST_CASE("ParseResult holds ParseOk", "[parse][phase1]") {
  parse::ParseResult result = parse::ParseOk{json({{"verbose", true}}), {"config", "set"}};
  REQUIRE(std::holds_alternative<parse::ParseOk>(result));
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config == json({{"verbose", true}}));
  REQUIRE(ok.command_path == std::vector<std::string>{"config", "set"});
}

TEST_CASE("ParseResult holds HelpRequest", "[parse][phase1]") {
  parse::ParseResult result = parse::HelpRequest{{"sub"}};
  REQUIRE(std::holds_alternative<parse::HelpRequest>(result));
  REQUIRE(std::get<parse::HelpRequest>(result).command_path == std::vector<std::string>{"sub"});
}

TEST_CASE("ParseResult holds VersionRequest", "[parse][phase1]") {
  parse::ParseResult result = parse::VersionRequest{};
  REQUIRE(std::holds_alternative<parse::VersionRequest>(result));
}

TEST_CASE("ParseResult holds ManpageRequest", "[parse][phase1]") {
  parse::ParseResult result = parse::ManpageRequest{{"sub"}};
  REQUIRE(std::holds_alternative<parse::ManpageRequest>(result));
  REQUIRE(std::get<parse::ManpageRequest>(result).command_path ==
          std::vector<std::string>{"sub"});
}

TEST_CASE("no_env always returns nullopt", "[parse][phase1]") {
  auto env = parse::no_env();
  REQUIRE_FALSE(env("HOME").has_value());
  REQUIRE_FALSE(env("PATH").has_value());
  REQUIRE_FALSE(env("").has_value());
}

// ===========================================================================
// Phase 2: Name Index
// ===========================================================================

namespace {

arg::FlagSpec
make_flag(model::ArgNames names) {
  model::Flag f{};
  f.names = std::move(names);
  f.doc = {"doc"};
  return arg::make(f);
}

arg::OptionSpec
make_option(model::ArgNames names, model::TypeSpec type = model::ScalarType::String) {
  model::Option o{};
  o.names = std::move(names);
  o.doc = {"doc"};
  o.type = std::move(type);
  return arg::make(o);
}

arg::PositionalSpec
make_positional(std::string name, model::TypeSpec type = model::ScalarType::String) {
  model::Positional p{};
  p.name = std::move(name);
  p.doc = {"doc"};
  p.type = std::move(type);
  return arg::make(p);
}

arg::FlagGroupSpec
make_flag_group(std::string dest, std::vector<arg::FlagGroupEntrySpec> entries) {
  return arg::FlagGroupSpec{
      std::move(dest),
      json("default"),
      std::move(entries),
      false,
  };
}

[[maybe_unused]] cmd::RootSpec
make_root(std::string name) {
  cmd::RootSpec r{};
  r.name = std::move(name);
  r.doc = {"doc"};
  return r;
}

[[maybe_unused]] cmd::CommandSpec
make_command(std::string name) {
  cmd::CommandSpec c{};
  c.name = std::move(name);
  c.doc = {"doc"};
  return c;
}

} // namespace

TEST_CASE("build_index: single long flag", "[parse][phase2]") {
  std::vector<arg::ArgSpec> args = {make_flag({"verbose"})};
  auto index = parse::detail::build_index(args);
  auto result = index.lookup("--verbose");
  REQUIRE(result.has_value());
  REQUIRE(result->arg_index == 0);
  REQUIRE(result->kind == parse::detail::MatchKind::Flag);
}

TEST_CASE("build_index: single short flag", "[parse][phase2]") {
  std::vector<arg::ArgSpec> args = {make_flag({"v"})};
  auto index = parse::detail::build_index(args);
  auto result = index.lookup("-v");
  REQUIRE(result.has_value());
  REQUIRE(result->arg_index == 0);
  REQUIRE(result->kind == parse::detail::MatchKind::Flag);
}

TEST_CASE("build_index: flag with both long and short names", "[parse][phase2]") {
  std::vector<arg::ArgSpec> args = {make_flag({"verbose", "v"})};
  auto index = parse::detail::build_index(args);

  auto long_result = index.lookup("--verbose");
  REQUIRE(long_result.has_value());
  REQUIRE(long_result->arg_index == 0);

  auto short_result = index.lookup("-v");
  REQUIRE(short_result.has_value());
  REQUIRE(short_result->arg_index == 0);
}

TEST_CASE("build_index: option registers as Option kind", "[parse][phase2]") {
  std::vector<arg::ArgSpec> args = {make_option({"output", "o"})};
  auto index = parse::detail::build_index(args);

  auto long_result = index.lookup("--output");
  REQUIRE(long_result.has_value());
  REQUIRE(long_result->kind == parse::detail::MatchKind::Option);

  auto short_result = index.lookup("-o");
  REQUIRE(short_result.has_value());
  REQUIRE(short_result->kind == parse::detail::MatchKind::Option);
}

TEST_CASE("build_index: flag group entries with entry indices", "[parse][phase2]") {
  std::vector<arg::ArgSpec> args = {
      make_flag_group("level", {
                                   {{"quiet", "q"}, json("quiet")},
                                   {{"verbose", "v"}, json("verbose")},
                               }),
  };
  auto index = parse::detail::build_index(args);

  auto q_result = index.lookup("--quiet");
  REQUIRE(q_result.has_value());
  REQUIRE(q_result->arg_index == 0);
  REQUIRE(q_result->kind == parse::detail::MatchKind::FlagGroup);
  REQUIRE(q_result->entry_index == 0);

  auto v_result = index.lookup("--verbose");
  REQUIRE(v_result.has_value());
  REQUIRE(v_result->arg_index == 0);
  REQUIRE(v_result->kind == parse::detail::MatchKind::FlagGroup);
  REQUIRE(v_result->entry_index == 1);

  auto v_short = index.lookup("-v");
  REQUIRE(v_short.has_value());
  REQUIRE(v_short->entry_index == 1);
}

TEST_CASE("build_index: positionals are not indexed", "[parse][phase2]") {
  std::vector<arg::ArgSpec> args = {make_positional("file")};
  auto index = parse::detail::build_index(args);
  REQUIRE_FALSE(index.lookup("--file").has_value());
  REQUIRE_FALSE(index.lookup("-f").has_value());
  REQUIRE_FALSE(index.lookup("file").has_value());
}

TEST_CASE("build_index: empty args produce empty index", "[parse][phase2]") {
  std::vector<arg::ArgSpec> args;
  auto index = parse::detail::build_index(args);
  REQUIRE_FALSE(index.lookup("--anything").has_value());
}

TEST_CASE("build_index: multiple args get correct indices", "[parse][phase2]") {
  std::vector<arg::ArgSpec> args = {
      make_flag({"verbose", "v"}),
      make_option({"output", "o"}),
      make_flag({"force", "f"}),
  };
  auto index = parse::detail::build_index(args);

  REQUIRE(index.lookup("--verbose")->arg_index == 0);
  REQUIRE(index.lookup("-v")->arg_index == 0);
  REQUIRE(index.lookup("--output")->arg_index == 1);
  REQUIRE(index.lookup("-o")->arg_index == 1);
  REQUIRE(index.lookup("--force")->arg_index == 2);
  REQUIRE(index.lookup("-f")->arg_index == 2);
}

// ===========================================================================
// Phase 3: Token Classification
// ===========================================================================

TEST_CASE("classify_token: --verbose is LongOption", "[parse][phase3]") {
  REQUIRE(parse::detail::classify_token("--verbose") == parse::detail::TokenKind::LongOption);
}

TEST_CASE("classify_token: --foo=bar is LongOption", "[parse][phase3]") {
  REQUIRE(parse::detail::classify_token("--foo=bar") == parse::detail::TokenKind::LongOption);
}

TEST_CASE("classify_token: -v is ShortGroup", "[parse][phase3]") {
  REQUIRE(parse::detail::classify_token("-v") == parse::detail::TokenKind::ShortGroup);
}

TEST_CASE("classify_token: -abc is ShortGroup", "[parse][phase3]") {
  REQUIRE(parse::detail::classify_token("-abc") == parse::detail::TokenKind::ShortGroup);
}

TEST_CASE("classify_token: -- is DoubleDash", "[parse][phase3]") {
  REQUIRE(parse::detail::classify_token("--") == parse::detail::TokenKind::DoubleDash);
}

TEST_CASE("classify_token: bare word is Positional", "[parse][phase3]") {
  REQUIRE(parse::detail::classify_token("hello") == parse::detail::TokenKind::Positional);
}

TEST_CASE("classify_token: bare - is Positional", "[parse][phase3]") {
  REQUIRE(parse::detail::classify_token("-") == parse::detail::TokenKind::Positional);
}

TEST_CASE("classify_token: empty string is Positional", "[parse][phase3]") {
  REQUIRE(parse::detail::classify_token("") == parse::detail::TokenKind::Positional);
}

TEST_CASE("split_long_option: --foo returns (foo, nullopt)", "[parse][phase3]") {
  auto [name, value] = parse::detail::split_long_option("--foo");
  REQUIRE(name == "foo");
  REQUIRE_FALSE(value.has_value());
}

TEST_CASE("split_long_option: --foo=bar returns (foo, bar)", "[parse][phase3]") {
  auto [name, value] = parse::detail::split_long_option("--foo=bar");
  REQUIRE(name == "foo");
  REQUIRE(value == "bar");
}

TEST_CASE("split_long_option: --foo= returns (foo, empty)", "[parse][phase3]") {
  auto [name, value] = parse::detail::split_long_option("--foo=");
  REQUIRE(name == "foo");
  REQUIRE(value == "");
}

TEST_CASE("split_long_option: --foo=bar=baz returns (foo, bar=baz)", "[parse][phase3]") {
  auto [name, value] = parse::detail::split_long_option("--foo=bar=baz");
  REQUIRE(name == "foo");
  REQUIRE(value == "bar=baz");
}

// ===========================================================================
// Phase 4: Single Flag Parsing
// ===========================================================================

TEST_CASE("parse: empty args produce empty JSON", "[parse][phase4]") {
  auto root = make_root("tool");
  auto result = parse::parse(root, {}, parse::no_env());
  REQUIRE(std::holds_alternative<parse::ParseOk>(result));
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config == json::object());
  REQUIRE(ok.command_path.empty());
}

TEST_CASE("parse: --verbose sets verbose=true", "[parse][phase4]") {
  auto root = make_root("tool");
  root.args = {arg::ArgSpec{make_flag({"verbose"})}};
  auto result = parse::parse(root, {"--verbose"}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["verbose"] == true);
}

TEST_CASE("parse: absent flag produces false", "[parse][phase4]") {
  auto root = make_root("tool");
  root.args = {arg::ArgSpec{make_flag({"verbose"})}};
  auto result = parse::parse(root, {}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["verbose"] == false);
}

TEST_CASE("parse: -v sets verbose=true (short flag)", "[parse][phase4]") {
  auto root = make_root("tool");
  root.args = {arg::ArgSpec{make_flag({"verbose", "v"})}};
  auto result = parse::parse(root, {"-v"}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["verbose"] == true);
}

TEST_CASE("parse: flag with both names works via long form", "[parse][phase4]") {
  auto root = make_root("tool");
  root.args = {arg::ArgSpec{make_flag({"verbose", "v"})}};
  auto result = parse::parse(root, {"--verbose"}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["verbose"] == true);
}

TEST_CASE("parse: unknown flag throws Error", "[parse][phase4]") {
  auto root = make_root("tool");
  REQUIRE_THROWS_AS(parse::parse(root, {"--unknown"}, parse::no_env()), parse::Error);
}

TEST_CASE("parse: repeated flag (repeated=false) sets true", "[parse][phase4]") {
  auto root = make_root("tool");
  root.args = {arg::ArgSpec{make_flag({"verbose"})}};
  auto result = parse::parse(root, {"--verbose", "--verbose"}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["verbose"] == true);
}

TEST_CASE("parse: repeated flag (repeated=true) counts", "[parse][phase4]") {
  auto root = make_root("tool");
  auto f = make_flag({"verbose", "v"});
  f.repeated = true;
  root.args = {arg::ArgSpec{f}};
  auto result = parse::parse(root, {"--verbose", "--verbose", "-v"}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["verbose"] == 3);
}

// ===========================================================================
// Phase 5: Option Parsing
// ===========================================================================

TEST_CASE("parse: --output=file.txt (= separator)", "[parse][phase5]") {
  auto root = make_root("tool");
  root.args = {arg::ArgSpec{make_option({"output", "o"})}};
  auto result = parse::parse(root, {"--output=file.txt"}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["output"] == "file.txt");
}

TEST_CASE("parse: --output file.txt (space separator)", "[parse][phase5]") {
  auto root = make_root("tool");
  root.args = {arg::ArgSpec{make_option({"output", "o"})}};
  auto result = parse::parse(root, {"--output", "file.txt"}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["output"] == "file.txt");
}

TEST_CASE("parse: -o file.txt (short option)", "[parse][phase5]") {
  auto root = make_root("tool");
  root.args = {arg::ArgSpec{make_option({"output", "o"})}};
  auto result = parse::parse(root, {"-o", "file.txt"}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["output"] == "file.txt");
}

TEST_CASE("parse: --count=42 with int converter", "[parse][phase5]") {
  auto root = make_root("tool");
  root.args = {arg::ArgSpec{make_option({"count"}, model::ScalarType::Int)}};
  auto result = parse::parse(root, {"--count=42"}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["count"] == 42);
}

TEST_CASE("parse: --count abc with int converter throws Error", "[parse][phase5]") {
  auto root = make_root("tool");
  root.args = {arg::ArgSpec{make_option({"count"}, model::ScalarType::Int)}};
  REQUIRE_THROWS_AS(parse::parse(root, {"--count", "abc"}, parse::no_env()), parse::Error);
}

TEST_CASE("parse: missing value at end throws Error", "[parse][phase5]") {
  auto root = make_root("tool");
  root.args = {arg::ArgSpec{make_option({"output"})}};
  REQUIRE_THROWS_AS(parse::parse(root, {"--output"}, parse::no_env()), parse::Error);
}

TEST_CASE("parse: absent option with default", "[parse][phase5]") {
  auto root = make_root("tool");
  auto opt = make_option({"output"});
  opt.default_value = json("stdout");
  root.args = {arg::ArgSpec{opt}};
  auto result = parse::parse(root, {}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["output"] == "stdout");
}

TEST_CASE("parse: absent option without default is absent from config", "[parse][phase5]") {
  auto root = make_root("tool");
  root.args = {arg::ArgSpec{make_option({"output"})}};
  auto result = parse::parse(root, {}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE_FALSE(ok.config.contains("output"));
}

TEST_CASE("parse: repeated option collects array", "[parse][phase5]") {
  auto root = make_root("tool");
  auto opt = make_option({"include", "I"});
  opt.repeated = true;
  root.args = {arg::ArgSpec{opt}};
  auto result = parse::parse(root, {"--include", "a", "-I", "b"}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["include"] == json::array({"a", "b"}));
}

TEST_CASE("parse: non-repeated option last wins", "[parse][phase5]") {
  auto root = make_root("tool");
  root.args = {arg::ArgSpec{make_option({"output"})}};
  auto result = parse::parse(root, {"--output", "a", "--output", "b"}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["output"] == "b");
}

// ===========================================================================
// Phase 6: Short Flag Grouping
// ===========================================================================

TEST_CASE("parse: -vf expands two flags", "[parse][phase6]") {
  auto root = make_root("tool");
  root.args = {
      arg::ArgSpec{make_flag({"verbose", "v"})},
      arg::ArgSpec{make_flag({"force", "f"})},
  };
  auto result = parse::parse(root, {"-vf"}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["verbose"] == true);
  REQUIRE(ok.config["force"] == true);
}

TEST_CASE("parse: -vo file.txt where v=flag, o=option", "[parse][phase6]") {
  auto root = make_root("tool");
  root.args = {
      arg::ArgSpec{make_flag({"verbose", "v"})},
      arg::ArgSpec{make_option({"output", "o"})},
  };
  auto result = parse::parse(root, {"-vo", "file.txt"}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["verbose"] == true);
  REQUIRE(ok.config["output"] == "file.txt");
}

TEST_CASE("parse: -ov file.txt where o=option in non-final position throws", "[parse][phase6]") {
  auto root = make_root("tool");
  root.args = {
      arg::ArgSpec{make_option({"output", "o"})},
      arg::ArgSpec{make_flag({"verbose", "v"})},
  };
  REQUIRE_THROWS_AS(parse::parse(root, {"-ov", "file.txt"}, parse::no_env()), parse::Error);
}

TEST_CASE("parse: -abc sets three flags", "[parse][phase6]") {
  auto root = make_root("tool");
  root.args = {
      arg::ArgSpec{make_flag({"alpha", "a"})},
      arg::ArgSpec{make_flag({"beta", "b"})},
      arg::ArgSpec{make_flag({"charlie", "c"})},
  };
  auto result = parse::parse(root, {"-abc"}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["alpha"] == true);
  REQUIRE(ok.config["beta"] == true);
  REQUIRE(ok.config["charlie"] == true);
}

// ===========================================================================
// Phase 7: Flag Groups
// ===========================================================================

TEST_CASE("parse: flag group selects entry value", "[parse][phase7]") {
  auto root = make_root("tool");
  root.args = {
      arg::ArgSpec{make_flag_group("level", {
                                                {{"quiet", "q"}, json("quiet")},
                                                {{"verbose", "v"}, json("verbose")},
                                            })},
  };
  auto result = parse::parse(root, {"--verbose"}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["level"] == "verbose");
}

TEST_CASE("parse: flag group no match uses default_value", "[parse][phase7]") {
  auto root = make_root("tool");
  root.args = {
      arg::ArgSpec{make_flag_group("level", {
                                                {{"quiet", "q"}, json("quiet")},
                                                {{"verbose", "v"}, json("verbose")},
                                            })},
  };
  auto result = parse::parse(root, {}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["level"] == "default");
}

TEST_CASE("parse: flag group entry via short name", "[parse][phase7]") {
  auto root = make_root("tool");
  root.args = {
      arg::ArgSpec{make_flag_group("level", {
                                                {{"quiet", "q"}, json("quiet")},
                                                {{"verbose", "v"}, json("verbose")},
                                            })},
  };
  auto result = parse::parse(root, {"-q"}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["level"] == "quiet");
}

TEST_CASE("parse: repeated flag group collects array", "[parse][phase7]") {
  auto root = make_root("tool");
  auto fg = make_flag_group("tags", {
                                        {{"alpha", "a"}, json("a")},
                                        {{"beta", "b"}, json("b")},
                                    });
  fg.repeated = true;
  root.args = {arg::ArgSpec{fg}};
  auto result = parse::parse(root, {"--alpha", "--beta", "--alpha"}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["tags"] == json::array({"a", "b", "a"}));
}

TEST_CASE("parse: non-repeated flag group last wins", "[parse][phase7]") {
  auto root = make_root("tool");
  root.args = {
      arg::ArgSpec{make_flag_group("level", {
                                                {{"quiet", "q"}, json("quiet")},
                                                {{"verbose", "v"}, json("verbose")},
                                            })},
  };
  auto result = parse::parse(root, {"--quiet", "--verbose"}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["level"] == "verbose");
}

TEST_CASE("parse: flag group entry in short group", "[parse][phase7]") {
  auto root = make_root("tool");
  root.args = {
      arg::ArgSpec{make_flag({"force", "f"})},
      arg::ArgSpec{make_flag_group("level", {
                                                {{"quiet", "q"}, json("quiet")},
                                                {{"verbose", "v"}, json("verbose")},
                                            })},
  };
  auto result = parse::parse(root, {"-fv"}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["force"] == true);
  REQUIRE(ok.config["level"] == "verbose");
}

// ===========================================================================
// Phase 8: Positional Arguments
// ===========================================================================

TEST_CASE("parse: single positional", "[parse][phase8]") {
  auto root = make_root("tool");
  root.args = {arg::ArgSpec{make_positional("file")}};
  auto result = parse::parse(root, {"hello.txt"}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["file"] == "hello.txt");
}

TEST_CASE("parse: positional with int converter", "[parse][phase8]") {
  auto root = make_root("tool");
  root.args = {arg::ArgSpec{make_positional("count", model::ScalarType::Int)}};
  auto result = parse::parse(root, {"42"}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["count"] == 42);
}

TEST_CASE("parse: two positionals in order", "[parse][phase8]") {
  auto root = make_root("tool");
  root.args = {
      arg::ArgSpec{make_positional("src")},
      arg::ArgSpec{make_positional("dst")},
  };
  auto result = parse::parse(root, {"a.txt", "b.txt"}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["src"] == "a.txt");
  REQUIRE(ok.config["dst"] == "b.txt");
}

TEST_CASE("parse: repeated positional collects array", "[parse][phase8]") {
  auto root = make_root("tool");
  auto pos = make_positional("files");
  pos.repeated = true;
  root.args = {arg::ArgSpec{pos}};
  auto result = parse::parse(root, {"a.txt", "b.txt", "c.txt"}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["files"] == json::array({"a.txt", "b.txt", "c.txt"}));
}

TEST_CASE("parse: -- terminates options", "[parse][phase8]") {
  auto root = make_root("tool");
  root.args = {
      arg::ArgSpec{make_flag({"verbose", "v"})},
      arg::ArgSpec{make_positional("file")},
  };
  auto result = parse::parse(root, {"--", "--verbose"}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["verbose"] == false);
  REQUIRE(ok.config["file"] == "--verbose");
}

TEST_CASE("parse: -- allows dash-prefixed positional", "[parse][phase8]") {
  auto root = make_root("tool");
  root.args = {arg::ArgSpec{make_positional("file")}};
  auto result = parse::parse(root, {"--", "-file"}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["file"] == "-file");
}

TEST_CASE("parse: mixed options and positionals", "[parse][phase8]") {
  auto root = make_root("tool");
  root.args = {
      arg::ArgSpec{make_flag({"verbose", "v"})},
      arg::ArgSpec{make_option({"output", "o"})},
      arg::ArgSpec{make_positional("file")},
  };
  auto result = parse::parse(root, {"--verbose", "input.txt", "-o", "out.txt"}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["verbose"] == true);
  REQUIRE(ok.config["output"] == "out.txt");
  REQUIRE(ok.config["file"] == "input.txt");
}

TEST_CASE("parse: too many positionals throws Error", "[parse][phase8]") {
  auto root = make_root("tool");
  root.args = {arg::ArgSpec{make_positional("file")}};
  REQUIRE_THROWS_AS(parse::parse(root, {"a", "b"}, parse::no_env()), parse::Error);
}

TEST_CASE("parse: absent positional with default", "[parse][phase8]") {
  auto root = make_root("tool");
  auto pos = make_positional("file");
  pos.default_value = json("stdin");
  root.args = {arg::ArgSpec{pos}};
  auto result = parse::parse(root, {}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["file"] == "stdin");
}

// ===========================================================================
// Phase 9: Subcommand Dispatch
// ===========================================================================

TEST_CASE("parse: dispatch selects correct subcommand", "[parse][phase9]") {
  auto root = make_root("tool");
  auto sub = make_command("init");
  sub.args = {arg::ArgSpec{make_positional("dir")}};
  root.commands = {sub};
  auto result = parse::parse(root, {"init", "/tmp"}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["dir"] == "/tmp");
}

TEST_CASE("parse: command_path contains subcommand", "[parse][phase9]") {
  auto root = make_root("tool");
  root.commands = {make_command("init"), make_command("build")};
  auto result = parse::parse(root, {"build"}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.command_path == std::vector<std::string>{"build"});
}

TEST_CASE("parse: parent args before subcommand name", "[parse][phase9]") {
  auto root = make_root("tool");
  root.args = {arg::ArgSpec{make_flag({"verbose", "v"})}};
  auto sub = make_command("init");
  sub.args = {arg::ArgSpec{make_positional("dir")}};
  root.commands = {sub};
  auto result = parse::parse(root, {"--verbose", "init", "/tmp"}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["verbose"] == true);
  REQUIRE(ok.config["dir"] == "/tmp");
  REQUIRE(ok.command_path == std::vector<std::string>{"init"});
}

TEST_CASE("parse: nested subcommands produce multi-element path", "[parse][phase9]") {
  auto root = make_root("tool");
  auto set_cmd = make_command("set");
  set_cmd.args = {
      arg::ArgSpec{make_positional("key")},
      arg::ArgSpec{make_positional("value")},
  };
  auto config_cmd = make_command("config");
  config_cmd.commands = {set_cmd};
  root.commands = {config_cmd};
  auto result = parse::parse(root, {"config", "set", "user.name", "Alice"}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.command_path == std::vector<std::string>{"config", "set"});
  REQUIRE(ok.config["key"] == "user.name");
  REQUIRE(ok.config["value"] == "Alice");
}

TEST_CASE("parse: unknown subcommand throws Error", "[parse][phase9]") {
  auto root = make_root("tool");
  root.commands = {make_command("init")};
  REQUIRE_THROWS_AS(parse::parse(root, {"unknown"}, parse::no_env()), parse::Error);
}

TEST_CASE("parse: parent flags + subcommand args coexist", "[parse][phase9]") {
  auto root = make_root("tool");
  root.args = {arg::ArgSpec{make_flag({"verbose", "v"})}};
  auto sub = make_command("build");
  sub.args = {arg::ArgSpec{make_option({"target", "t"})}};
  root.commands = {sub};
  auto result = parse::parse(root, {"-v", "build", "--target", "release"}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["verbose"] == true);
  REQUIRE(ok.config["target"] == "release");
}

// ===========================================================================
// Phase 10: --help and --version
// ===========================================================================

TEST_CASE("parse: --help returns HelpRequest with empty path", "[parse][phase10]") {
  auto root = make_root("tool");
  auto result = parse::parse(root, {"--help"}, parse::no_env());
  REQUIRE(std::holds_alternative<parse::HelpRequest>(result));
  REQUIRE(std::get<parse::HelpRequest>(result).command_path.empty());
}

TEST_CASE("parse: -h returns HelpRequest with empty path", "[parse][phase10]") {
  auto root = make_root("tool");
  auto result = parse::parse(root, {"-h"}, parse::no_env());
  REQUIRE(std::holds_alternative<parse::HelpRequest>(result));
  REQUIRE(std::get<parse::HelpRequest>(result).command_path.empty());
}

TEST_CASE("parse: -h after subcommand returns HelpRequest with path", "[parse][phase10]") {
  auto root = make_root("tool");
  root.commands = {make_command("build")};
  auto result = parse::parse(root, {"build", "-h"}, parse::no_env());
  REQUIRE(std::holds_alternative<parse::HelpRequest>(result));
  REQUIRE(std::get<parse::HelpRequest>(result).command_path ==
          std::vector<std::string>{"build"});
}

TEST_CASE("parse: -h short-circuits (no validation)", "[parse][phase10]") {
  auto root = make_root("tool");
  auto opt = make_option({"required-opt"});
  opt.validator = validate::required();
  root.args = {arg::ArgSpec{opt}};
  auto result = parse::parse(root, {"-h"}, parse::no_env());
  REQUIRE(std::holds_alternative<parse::HelpRequest>(result));
}

TEST_CASE("parse: --help after subcommand returns HelpRequest with path", "[parse][phase10]") {
  auto root = make_root("tool");
  root.commands = {make_command("build")};
  auto result = parse::parse(root, {"build", "--help"}, parse::no_env());
  REQUIRE(std::holds_alternative<parse::HelpRequest>(result));
  REQUIRE(std::get<parse::HelpRequest>(result).command_path ==
          std::vector<std::string>{"build"});
}

TEST_CASE("parse: --version returns VersionRequest", "[parse][phase10]") {
  auto root = make_root("tool");
  root.version = "1.0.0";
  auto result = parse::parse(root, {"--version"}, parse::no_env());
  REQUIRE(std::holds_alternative<parse::VersionRequest>(result));
}

TEST_CASE("parse: --version with no version defined throws Error", "[parse][phase10]") {
  auto root = make_root("tool");
  REQUIRE_THROWS_AS(parse::parse(root, {"--version"}, parse::no_env()), parse::Error);
}

TEST_CASE("parse: --help short-circuits (no validation)", "[parse][phase10]") {
  auto root = make_root("tool");
  auto opt = make_option({"required-opt"});
  opt.validator = validate::required();
  root.args = {arg::ArgSpec{opt}};
  // Without --help this would fail validation (required option missing)
  auto result = parse::parse(root, {"--help"}, parse::no_env());
  REQUIRE(std::holds_alternative<parse::HelpRequest>(result));
}

TEST_CASE("parse: --help-man returns ManpageRequest with empty path", "[parse][phase10]") {
  auto root = make_root("tool");
  auto result = parse::parse(root, {"--help-man"}, parse::no_env());
  REQUIRE(std::holds_alternative<parse::ManpageRequest>(result));
  REQUIRE(std::get<parse::ManpageRequest>(result).command_path.empty());
}

TEST_CASE("parse: --help-man after subcommand returns ManpageRequest with path",
          "[parse][phase10]") {
  auto root = make_root("tool");
  root.commands = {make_command("build")};
  auto result = parse::parse(root, {"build", "--help-man"}, parse::no_env());
  REQUIRE(std::holds_alternative<parse::ManpageRequest>(result));
  REQUIRE(std::get<parse::ManpageRequest>(result).command_path ==
          std::vector<std::string>{"build"});
}

TEST_CASE("parse: --help-man short-circuits (no validation)", "[parse][phase10]") {
  auto root = make_root("tool");
  auto opt = make_option({"required-opt"});
  opt.validator = validate::required();
  root.args = {arg::ArgSpec{opt}};
  // Without --help-man this would fail validation (required option missing)
  auto result = parse::parse(root, {"--help-man"}, parse::no_env());
  REQUIRE(std::holds_alternative<parse::ManpageRequest>(result));
}

TEST_CASE("parse: --help-man in nested subcommand propagates full path", "[parse][phase10]") {
  auto root = make_root("tool");
  auto set_cmd = make_command("set");
  auto config_cmd = make_command("config");
  config_cmd.commands = {set_cmd};
  root.commands = {config_cmd};
  auto result = parse::parse(root, {"config", "set", "--help-man"}, parse::no_env());
  REQUIRE(std::holds_alternative<parse::ManpageRequest>(result));
  REQUIRE(std::get<parse::ManpageRequest>(result).command_path ==
          std::vector<std::string>{"config", "set"});
}

// ===========================================================================
// Phase 11: Environment Variable Fallback
// ===========================================================================

namespace {

parse::EnvLookup
make_env(std::initializer_list<std::pair<std::string, std::string>> pairs) {
  auto map = std::make_shared<std::unordered_map<std::string, std::string>>();
  for (const auto &p : pairs) {
    map->emplace(p.first, p.second);
  }
  return [map](const std::string &var) -> std::optional<std::string> {
    auto it = map->find(var);
    if (it == map->end()) {
      return std::nullopt;
    }
    return it->second;
  };
}

} // namespace

TEST_CASE("parse: flag env 'true' sets true", "[parse][phase11]") {
  auto root = make_root("tool");
  auto f = make_flag({"verbose"});
  f.env = arg::EnvSpec{"VERBOSE", std::nullopt};
  root.args = {arg::ArgSpec{f}};
  auto result = parse::parse(root, {}, make_env({{"VERBOSE", "true"}}));
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["verbose"] == true);
}

TEST_CASE("parse: flag env 'false' sets false", "[parse][phase11]") {
  auto root = make_root("tool");
  auto f = make_flag({"verbose"});
  f.env = arg::EnvSpec{"VERBOSE", std::nullopt};
  root.args = {arg::ArgSpec{f}};
  auto result = parse::parse(root, {}, make_env({{"VERBOSE", "false"}}));
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["verbose"] == false);
}

TEST_CASE("parse: flag env '1' sets true", "[parse][phase11]") {
  auto root = make_root("tool");
  auto f = make_flag({"verbose"});
  f.env = arg::EnvSpec{"VERBOSE", std::nullopt};
  root.args = {arg::ArgSpec{f}};
  auto result = parse::parse(root, {}, make_env({{"VERBOSE", "1"}}));
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["verbose"] == true);
}

TEST_CASE("parse: flag env '0' sets false", "[parse][phase11]") {
  auto root = make_root("tool");
  auto f = make_flag({"verbose"});
  f.env = arg::EnvSpec{"VERBOSE", std::nullopt};
  root.args = {arg::ArgSpec{f}};
  auto result = parse::parse(root, {}, make_env({{"VERBOSE", "0"}}));
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["verbose"] == false);
}

TEST_CASE("parse: option env through converter", "[parse][phase11]") {
  auto root = make_root("tool");
  auto opt = make_option({"count"}, model::ScalarType::Int);
  opt.env = arg::EnvSpec{"COUNT", std::nullopt};
  root.args = {arg::ArgSpec{opt}};
  auto result = parse::parse(root, {}, make_env({{"COUNT", "42"}}));
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["count"] == 42);
}

TEST_CASE("parse: CLI takes precedence over env", "[parse][phase11]") {
  auto root = make_root("tool");
  auto opt = make_option({"output"});
  opt.env = arg::EnvSpec{"OUTPUT", std::nullopt};
  root.args = {arg::ArgSpec{opt}};
  auto result = parse::parse(root, {"--output", "cli.txt"}, make_env({{"OUTPUT", "env.txt"}}));
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["output"] == "cli.txt");
}

TEST_CASE("parse: unset env leaves value absent", "[parse][phase11]") {
  auto root = make_root("tool");
  auto opt = make_option({"output"});
  opt.env = arg::EnvSpec{"OUTPUT", std::nullopt};
  root.args = {arg::ArgSpec{opt}};
  auto result = parse::parse(root, {}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE_FALSE(ok.config.contains("output"));
}

TEST_CASE("parse: env conversion error throws Error", "[parse][phase11]") {
  auto root = make_root("tool");
  auto opt = make_option({"count"}, model::ScalarType::Int);
  opt.env = arg::EnvSpec{"COUNT", std::nullopt};
  root.args = {arg::ArgSpec{opt}};
  REQUIRE_THROWS_AS(parse::parse(root, {}, make_env({{"COUNT", "abc"}})), parse::Error);
}

// ===========================================================================
// Phase 12: Default Values and Validation
// ===========================================================================

TEST_CASE("parse: default applied when absent", "[parse][phase12]") {
  auto root = make_root("tool");
  auto opt = make_option({"output"});
  opt.default_value = json("stdout");
  root.args = {arg::ArgSpec{opt}};
  auto result = parse::parse(root, {}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["output"] == "stdout");
}

TEST_CASE("parse: default not applied when CLI present", "[parse][phase12]") {
  auto root = make_root("tool");
  auto opt = make_option({"output"});
  opt.default_value = json("stdout");
  root.args = {arg::ArgSpec{opt}};
  auto result = parse::parse(root, {"--output", "file.txt"}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["output"] == "file.txt");
}

TEST_CASE("parse: default not applied when env present", "[parse][phase12]") {
  auto root = make_root("tool");
  auto opt = make_option({"output"});
  opt.default_value = json("stdout");
  opt.env = arg::EnvSpec{"OUTPUT", std::nullopt};
  root.args = {arg::ArgSpec{opt}};
  auto result = parse::parse(root, {}, make_env({{"OUTPUT", "env.txt"}}));
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["output"] == "env.txt");
}

TEST_CASE("parse: required validator throws for absent", "[parse][phase12]") {
  auto root = make_root("tool");
  auto opt = make_option({"output"});
  opt.validator = validate::required();
  root.args = {arg::ArgSpec{opt}};
  REQUIRE_THROWS_AS(parse::parse(root, {}, parse::no_env()), parse::Error);
}

TEST_CASE("parse: required passes for defaulted", "[parse][phase12]") {
  auto root = make_root("tool");
  auto opt = make_option({"output"});
  opt.default_value = json("stdout");
  opt.validator = validate::required();
  root.args = {arg::ArgSpec{opt}};
  auto result = parse::parse(root, {}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["output"] == "stdout");
}

TEST_CASE("parse: validation runs after env and defaults", "[parse][phase12]") {
  auto root = make_root("tool");
  auto opt = make_option({"output"});
  opt.validator = validate::required();
  opt.env = arg::EnvSpec{"OUTPUT", std::nullopt};
  root.args = {arg::ArgSpec{opt}};
  // env satisfies the required constraint
  auto result = parse::parse(root, {}, make_env({{"OUTPUT", "env.txt"}}));
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["output"] == "env.txt");
}

// ===========================================================================
// Phase 13: Integration Tests
// ===========================================================================

TEST_CASE("integration: git-like commit -m 'msg' -a", "[parse][phase13]") {
  // Build a git-like CLI
  auto commit_cmd = make_command("commit");
  auto msg_opt = make_option({"message", "m"});
  auto all_flag = make_flag({"all", "a"});
  commit_cmd.args = {arg::ArgSpec{msg_opt}, arg::ArgSpec{all_flag}};

  auto root = make_root("mygit");
  root.version = "1.0.0";
  root.args = {arg::ArgSpec{make_flag({"verbose", "v"})}};
  root.commands = {commit_cmd};

  auto result = parse::parse(root, {"commit", "-m", "initial", "-a"}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.command_path == std::vector<std::string>{"commit"});
  REQUIRE(ok.config["message"] == "initial");
  REQUIRE(ok.config["all"] == true);
  REQUIRE(ok.config["verbose"] == false);
}

TEST_CASE("integration: nested config set key value with global --verbose", "[parse][phase13]") {
  auto set_cmd = make_command("set");
  set_cmd.args = {
      arg::ArgSpec{make_positional("key")},
      arg::ArgSpec{make_positional("value")},
  };
  auto config_cmd = make_command("config");
  config_cmd.commands = {set_cmd};

  auto root = make_root("tool");
  root.args = {arg::ArgSpec{make_flag({"verbose", "v"})}};
  root.commands = {config_cmd};

  auto result =
      parse::parse(root, {"--verbose", "config", "set", "user.name", "Alice"}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.command_path == std::vector<std::string>{"config", "set"});
  REQUIRE(ok.config["verbose"] == true);
  REQUIRE(ok.config["key"] == "user.name");
  REQUIRE(ok.config["value"] == "Alice");
}

TEST_CASE("integration: empty argv with all-optional args", "[parse][phase13]") {
  auto root = make_root("tool");
  auto opt = make_option({"output"});
  opt.default_value = json("stdout");
  root.args = {
      arg::ArgSpec{make_flag({"verbose"})},
      arg::ArgSpec{opt},
  };
  auto result = parse::parse(root, {}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["verbose"] == false);
  REQUIRE(ok.config["output"] == "stdout");
  REQUIRE(ok.command_path.empty());
}

TEST_CASE("integration: all arg types combined", "[parse][phase13]") {
  auto root = make_root("tool");
  auto fg = make_flag_group("level", {
                                         {{"quiet", "q"}, json("quiet")},
                                         {{"verbose", "v"}, json("verbose")},
                                     });
  auto opt = make_option({"output", "o"});
  auto pos = make_positional("file");
  root.args = {
      arg::ArgSpec{make_flag({"force", "f"})},
      arg::ArgSpec{fg},
      arg::ArgSpec{opt},
      arg::ArgSpec{pos},
  };

  auto result = parse::parse(root, {"-fv", "--output=out.txt", "input.txt"}, parse::no_env());
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["force"] == true);
  REQUIRE(ok.config["level"] == "verbose");
  REQUIRE(ok.config["output"] == "out.txt");
  REQUIRE(ok.config["file"] == "input.txt");
}

TEST_CASE("integration: env fallback with subcommands", "[parse][phase13]") {
  auto sub = make_command("build");
  auto target_opt = make_option({"target"});
  target_opt.env = arg::EnvSpec{"BUILD_TARGET", std::nullopt};
  target_opt.default_value = json("debug");
  sub.args = {arg::ArgSpec{target_opt}};

  auto root = make_root("tool");
  root.commands = {sub};

  auto result = parse::parse(root, {"build"}, make_env({{"BUILD_TARGET", "release"}}));
  auto &ok = std::get<parse::ParseOk>(result);
  REQUIRE(ok.config["target"] == "release");
}

TEST_CASE("integration: --help on subcommand", "[parse][phase13]") {
  auto sub = make_command("build");
  sub.args = {arg::ArgSpec{make_option({"target"})}};
  auto root = make_root("tool");
  root.commands = {sub};

  auto result = parse::parse(root, {"build", "--help"}, parse::no_env());
  REQUIRE(std::holds_alternative<parse::HelpRequest>(result));
  REQUIRE(std::get<parse::HelpRequest>(result).command_path ==
          std::vector<std::string>{"build"});
}

TEST_CASE("integration: --help-man on subcommand", "[parse][phase13]") {
  auto sub = make_command("build");
  sub.args = {arg::ArgSpec{make_option({"target"})}};
  auto root = make_root("tool");
  root.commands = {sub};

  auto result = parse::parse(root, {"build", "--help-man"}, parse::no_env());
  REQUIRE(std::holds_alternative<parse::ManpageRequest>(result));
  REQUIRE(std::get<parse::ManpageRequest>(result).command_path ==
          std::vector<std::string>{"build"});
}

TEST_CASE("integration: idempotence (same args twice give same result)", "[parse][phase13]") {
  auto root = make_root("tool");
  root.args = {
      arg::ArgSpec{make_flag({"verbose", "v"})},
      arg::ArgSpec{make_option({"output", "o"})},
  };

  std::vector<std::string> argv = {"--verbose", "-o", "file.txt"};

  auto result1 = parse::parse(root, argv, parse::no_env());
  auto result2 = parse::parse(root, argv, parse::no_env());

  auto &ok1 = std::get<parse::ParseOk>(result1);
  auto &ok2 = std::get<parse::ParseOk>(result2);
  REQUIRE(ok1.config == ok2.config);
  REQUIRE(ok1.command_path == ok2.command_path);
}
