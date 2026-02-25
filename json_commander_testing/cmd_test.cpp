#include <catch2/catch_test_macros.hpp>
#include <json_commander/cmd.hpp>

using namespace json_commander;
using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Helpers for building model types without triggering -Wmissing-field
// ---------------------------------------------------------------------------

namespace {

  model::Command
  make_command(std::string name) {
    model::Command c{};
    c.name = std::move(name);
    c.doc = {"doc"};
    return c;
  }

  model::Root
  make_root(std::string name) {
    model::Root r{};
    r.name = std::move(name);
    r.doc = {"doc"};
    return r;
  }

  model::Flag
  make_flag(model::ArgNames names) {
    model::Flag f{};
    f.names = std::move(names);
    f.doc = {"doc"};
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

  model::Positional
  make_positional(std::string name, model::TypeSpec type) {
    model::Positional pos{};
    pos.name = std::move(name);
    pos.doc = {"doc"};
    pos.type = std::move(type);
    return pos;
  }

  model::FlagGroup
  make_flag_group(std::string dest) {
    model::FlagGroup g{};
    g.dest = std::move(dest);
    g.doc = {"doc"};
    g.default_value = json("default");
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

} // namespace

// ---------------------------------------------------------------------------
// Phase 1: Leaf CommandSpec (no args, no subcommands)
// ---------------------------------------------------------------------------

TEST_CASE(
  "make(Command) with no args and no subcommands produces empty vectors",
  "[cmd]") {
  auto c = make_command("sub");
  auto spec = cmd::make(c);
  REQUIRE(spec.name == "sub");
  REQUIRE(spec.args.empty());
  REQUIRE(spec.commands.empty());
}

TEST_CASE("make(Command) preserves name", "[cmd]") {
  auto c = make_command("my-tool");
  auto spec = cmd::make(c);
  REQUIRE(spec.name == "my-tool");
}

TEST_CASE("make(Command) preserves multi-line doc", "[cmd]") {
  auto c = make_command("sub");
  c.doc = {"Line 1", "", "Line 2"};
  auto spec = cmd::make(c);
  REQUIRE(spec.doc == model::DocString{"Line 1", "", "Line 2"});
}

// ---------------------------------------------------------------------------
// Phase 2: CommandSpec with arguments
// ---------------------------------------------------------------------------

TEST_CASE(
  "make(Command) with one flag produces one FlagSpec in args", "[cmd]") {
  auto c = make_command("sub");
  c.args = std::vector<model::Argument>{make_flag({"verbose"})};
  auto spec = cmd::make(c);
  REQUIRE(spec.args.size() == 1);
  REQUIRE(std::holds_alternative<arg::FlagSpec>(spec.args[0]));
}

TEST_CASE("make(Command) with mixed args preserves order", "[cmd]") {
  auto c = make_command("sub");
  c.args = std::vector<model::Argument>{
    make_flag({"verbose"}),
    make_option({"output"}, model::ScalarType::String),
    make_positional("FILE", model::ScalarType::String),
  };
  auto spec = cmd::make(c);
  REQUIRE(spec.args.size() == 3);
  REQUIRE(std::holds_alternative<arg::FlagSpec>(spec.args[0]));
  REQUIRE(std::holds_alternative<arg::OptionSpec>(spec.args[1]));
  REQUIRE(std::holds_alternative<arg::PositionalSpec>(spec.args[2]));
}

TEST_CASE("make(Command) compiles option with correct converter", "[cmd]") {
  auto c = make_command("sub");
  c.args = std::vector<model::Argument>{
    make_option({"count"}, model::ScalarType::Int)};
  auto spec = cmd::make(c);
  auto& opt = std::get<arg::OptionSpec>(spec.args[0]);
  REQUIRE(opt.converter.parse("42") == json(42));
}

// ---------------------------------------------------------------------------
// Phase 3: CommandSpec with subcommands (one level)
// ---------------------------------------------------------------------------

TEST_CASE(
  "make(Command) with one subcommand produces one CommandSpec", "[cmd]") {
  auto child = make_command("sub1");
  auto parent = make_command("parent");
  parent.commands = std::vector<model::Command>{child};
  auto spec = cmd::make(parent);
  REQUIRE(spec.commands.size() == 1);
  REQUIRE(spec.commands[0].name == "sub1");
}

TEST_CASE("make(Command) with multiple subcommands preserves order", "[cmd]") {
  auto parent = make_command("parent");
  parent.commands = std::vector<model::Command>{
    make_command("alpha"),
    make_command("beta"),
    make_command("gamma"),
  };
  auto spec = cmd::make(parent);
  REQUIRE(spec.commands.size() == 3);
  REQUIRE(spec.commands[0].name == "alpha");
  REQUIRE(spec.commands[1].name == "beta");
  REQUIRE(spec.commands[2].name == "gamma");
}

// ---------------------------------------------------------------------------
// Phase 4: Nested subcommands (recursive)
// ---------------------------------------------------------------------------

TEST_CASE("make(Command) compiles two-level nested subcommands", "[cmd]") {
  auto subsub = make_command("subsub");
  auto sub = make_command("sub");
  sub.commands = std::vector<model::Command>{subsub};
  auto parent = make_command("parent");
  parent.commands = std::vector<model::Command>{sub};

  auto spec = cmd::make(parent);
  REQUIRE(spec.commands.size() == 1);
  REQUIRE(spec.commands[0].name == "sub");
  REQUIRE(spec.commands[0].commands.size() == 1);
  REQUIRE(spec.commands[0].commands[0].name == "subsub");
}

TEST_CASE("make(Command) compiles three-level nesting", "[cmd]") {
  auto leaf = make_command("leaf");
  auto mid = make_command("mid");
  mid.commands = std::vector<model::Command>{leaf};
  auto top_sub = make_command("top-sub");
  top_sub.commands = std::vector<model::Command>{mid};
  auto root = make_command("root");
  root.commands = std::vector<model::Command>{top_sub};

  auto spec = cmd::make(root);
  REQUIRE(spec.commands[0].commands[0].commands[0].name == "leaf");
}

// ---------------------------------------------------------------------------
// Phase 5: Subcommands with their own arguments
// ---------------------------------------------------------------------------

TEST_CASE("Subcommand args are compiled independently from parent", "[cmd]") {
  auto child = make_command("child");
  child.args = std::vector<model::Argument>{
    make_option({"count"}, model::ScalarType::Int)};

  auto parent = make_command("parent");
  parent.args = std::vector<model::Argument>{make_flag({"verbose"})};
  parent.commands = std::vector<model::Command>{child};

  auto spec = cmd::make(parent);
  REQUIRE(spec.args.size() == 1);
  REQUIRE(std::holds_alternative<arg::FlagSpec>(spec.args[0]));
  REQUIRE(spec.commands[0].args.size() == 1);
  REQUIRE(std::holds_alternative<arg::OptionSpec>(spec.commands[0].args[0]));
}

TEST_CASE("Subcommand converter works correctly", "[cmd]") {
  auto child = make_command("child");
  child.args = std::vector<model::Argument>{
    make_option({"count"}, model::ScalarType::Int)};

  auto parent = make_command("parent");
  parent.commands = std::vector<model::Command>{child};

  auto spec = cmd::make(parent);
  auto& opt = std::get<arg::OptionSpec>(spec.commands[0].args[0]);
  REQUIRE(opt.converter.parse("99") == json(99));
}

// ---------------------------------------------------------------------------
// Phase 6: RootSpec — basic fields
// ---------------------------------------------------------------------------

TEST_CASE("make(Root) with no version and no config", "[cmd]") {
  auto root = make_root("tool");
  auto spec = cmd::make(root);
  REQUIRE(spec.name == "tool");
  REQUIRE_FALSE(spec.version.has_value());
  REQUIRE_FALSE(spec.config.has_value());
}

TEST_CASE("make(Root) with version", "[cmd]") {
  auto root = make_root("tool");
  root.version = "1.2.3";
  auto spec = cmd::make(root);
  REQUIRE(spec.version.has_value());
  REQUIRE(*spec.version == "1.2.3");
}

TEST_CASE("make(Root) with config", "[cmd]") {
  auto root = make_root("tool");
  model::Config cfg{};
  cfg.format = "json";
  root.config = cfg;
  auto spec = cmd::make(root);
  REQUIRE(spec.config.has_value());
  REQUIRE(spec.config->format == "json");
}

TEST_CASE("make(Root) preserves name and doc", "[cmd]") {
  auto root = make_root("mytool");
  root.doc = {"A great tool", "", "With multiple paragraphs"};
  auto spec = cmd::make(root);
  REQUIRE(spec.name == "mytool");
  REQUIRE(
    spec.doc ==
    model::DocString{"A great tool", "", "With multiple paragraphs"});
}

// ---------------------------------------------------------------------------
// Phase 7: RootSpec — args and subcommands
// ---------------------------------------------------------------------------

TEST_CASE("make(Root) compiles args via arg::make_all", "[cmd]") {
  auto root = make_root("tool");
  root.args = std::vector<model::Argument>{make_flag({"verbose"})};
  auto spec = cmd::make(root);
  REQUIRE(spec.args.size() == 1);
  REQUIRE(std::holds_alternative<arg::FlagSpec>(spec.args[0]));
}

TEST_CASE("make(Root) compiles subcommands recursively", "[cmd]") {
  auto root = make_root("tool");
  root.commands = std::vector<model::Command>{
    make_command("init"),
    make_command("build"),
  };
  auto spec = cmd::make(root);
  REQUIRE(spec.commands.size() == 2);
  REQUIRE(spec.commands[0].name == "init");
  REQUIRE(spec.commands[1].name == "build");
}

TEST_CASE("make(Root) with nested subcommands that have args", "[cmd]") {
  auto sub_sub = make_command("set");
  sub_sub.args = std::vector<model::Argument>{
    make_positional("key", model::ScalarType::String),
  };

  auto sub = make_command("config");
  sub.commands = std::vector<model::Command>{sub_sub};

  auto root = make_root("tool");
  root.commands = std::vector<model::Command>{sub};

  auto spec = cmd::make(root);
  REQUIRE(spec.commands[0].name == "config");
  REQUIRE(spec.commands[0].commands[0].name == "set");
  REQUIRE(spec.commands[0].commands[0].args.size() == 1);
  REQUIRE(
    std::holds_alternative<arg::PositionalSpec>(
      spec.commands[0].commands[0].args[0]));
}

// ---------------------------------------------------------------------------
// Phase 8: Integration tests
// ---------------------------------------------------------------------------

TEST_CASE("Full realistic schema compiles correctly", "[cmd]") {
  // Build a git-like CLI: tool with version, global flags, and subcommands
  auto init_cmd = make_command("init");
  init_cmd.args = std::vector<model::Argument>{
    make_positional("directory", model::ScalarType::String),
  };

  auto commit_cmd = make_command("commit");
  commit_cmd.args = std::vector<model::Argument>{
    make_option({"message", "m"}, model::ScalarType::String),
    make_flag({"all", "a"}),
  };

  auto show_cmd = make_command("show");
  auto set_cmd = make_command("set");
  set_cmd.args = std::vector<model::Argument>{
    make_positional("key", model::ScalarType::String),
    make_positional("value", model::ScalarType::String),
  };
  auto config_cmd = make_command("config");
  config_cmd.commands = std::vector<model::Command>{show_cmd, set_cmd};

  auto root = make_root("mygit");
  root.version = "1.0.0";
  auto verbosity = make_flag_group("verbosity");
  verbosity.flags = {
    make_flag_group_entry({"quiet", "q"}, json("quiet")),
    make_flag_group_entry({"verbose", "v"}, json("verbose")),
  };
  root.args = std::vector<model::Argument>{verbosity};
  root.commands = std::vector<model::Command>{init_cmd, commit_cmd, config_cmd};

  auto spec = cmd::make(root);

  // Root level
  REQUIRE(spec.name == "mygit");
  REQUIRE(spec.version == "1.0.0");
  REQUIRE(spec.args.size() == 1);
  REQUIRE(std::holds_alternative<arg::FlagGroupSpec>(spec.args[0]));

  // Subcommands
  REQUIRE(spec.commands.size() == 3);
  REQUIRE(spec.commands[0].name == "init");
  REQUIRE(spec.commands[0].args.size() == 1);
  REQUIRE(spec.commands[1].name == "commit");
  REQUIRE(spec.commands[1].args.size() == 2);
  REQUIRE(spec.commands[2].name == "config");
  REQUIRE(spec.commands[2].args.empty());

  // Nested subcommands
  REQUIRE(spec.commands[2].commands.size() == 2);
  REQUIRE(spec.commands[2].commands[0].name == "show");
  REQUIRE(spec.commands[2].commands[0].args.empty());
  REQUIRE(spec.commands[2].commands[1].name == "set");
  REQUIRE(spec.commands[2].commands[1].args.size() == 2);

  // Verify converter works deep in tree
  auto& key_pos =
    std::get<arg::PositionalSpec>(spec.commands[2].commands[1].args[0]);
  REQUIRE(key_pos.converter.parse("my.key") == json("my.key"));
}

TEST_CASE("Empty root compiles cleanly", "[cmd]") {
  auto root = make_root("empty");
  auto spec = cmd::make(root);
  REQUIRE(spec.name == "empty");
  REQUIRE(spec.args.empty());
  REQUIRE(spec.commands.empty());
  REQUIRE_FALSE(spec.version.has_value());
  REQUIRE_FALSE(spec.config.has_value());
}

TEST_CASE("make(Command) excludes man envs exits by construction", "[cmd]") {
  auto c = make_command("sub");
  c.man = model::Man{};
  c.envs = std::vector<model::EnvInfo>{};
  c.exits = std::vector<model::ExitInfo>{};
  // CommandSpec has no man/envs/exits fields — this compiles and the metadata
  // is dropped
  auto spec = cmd::make(c);
  REQUIRE(spec.name == "sub");
  REQUIRE(spec.args.empty());
  REQUIRE(spec.commands.empty());
}
