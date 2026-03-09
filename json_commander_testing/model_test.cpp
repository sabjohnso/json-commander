#include <catch2/catch_test_macros.hpp>
#include <json_commander/model.hpp>
#include <json_commander/model_json.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;
using namespace json_commander::model;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

  template <typename T>
  void
  round_trip(const T& value) {
    json j = value;
    auto recovered = j.get<T>();
    REQUIRE(recovered == value);
  }

  template <typename T>
  void
  round_trip_json(const json& j) {
    auto value = j.get<T>();
    json j2 = value;
    REQUIRE(j2 == j);
  }

} // namespace

// ===========================================================================
// Phase 1: Leaf Types
// ===========================================================================

TEST_CASE("ScalarType round-trip", "[model][leaf]") {
  SECTION("all enum values") {
    round_trip(ScalarType::String);
    round_trip(ScalarType::Int);
    round_trip(ScalarType::Float);
    round_trip(ScalarType::Bool);
    round_trip(ScalarType::Enum);
    round_trip(ScalarType::File);
    round_trip(ScalarType::Dir);
    round_trip(ScalarType::Path);
  }

  SECTION("serializes to expected strings") {
    REQUIRE(json(ScalarType::String) == "string");
    REQUIRE(json(ScalarType::Int) == "int");
    REQUIRE(json(ScalarType::Float) == "float");
    REQUIRE(json(ScalarType::Bool) == "bool");
    REQUIRE(json(ScalarType::Enum) == "enum");
    REQUIRE(json(ScalarType::File) == "file");
    REQUIRE(json(ScalarType::Dir) == "dir");
    REQUIRE(json(ScalarType::Path) == "path");
  }
}

TEST_CASE("DocString round-trip", "[model][leaf]") {
  round_trip(DocString{});
  round_trip(DocString{"Single line"});
  round_trip(DocString{"Line one", "Line two", "", "After break"});
}

TEST_CASE("ListType round-trip", "[model][leaf]") {
  SECTION("with separator") { round_trip(ListType{ScalarType::String, ","}); }

  SECTION("without separator") {
    round_trip(ListType{ScalarType::Int, std::nullopt});
  }

  SECTION("from JSON with separator") {
    json j = {{"list", {{"element", "string"}, {"separator", ","}}}};
    round_trip_json<ListType>(j);
  }

  SECTION("from JSON without separator") {
    json j = {{"list", {{"element", "int"}}}};
    round_trip_json<ListType>(j);
  }
}

TEST_CASE("PairType round-trip", "[model][leaf]") {
  SECTION("with separator") {
    round_trip(PairType{ScalarType::String, ScalarType::Int, "="});
  }

  SECTION("without separator") {
    round_trip(PairType{ScalarType::Float, ScalarType::Bool, std::nullopt});
  }

  SECTION("from JSON") {
    json j = {{"pair", {{"first", "string"}, {"second", "int"}}}};
    round_trip_json<PairType>(j);
  }
}

TEST_CASE("TripleType round-trip", "[model][leaf]") {
  SECTION("with separator") {
    round_trip(
      TripleType{ScalarType::Int, ScalarType::Int, ScalarType::Int, ","});
  }

  SECTION("without separator") {
    round_trip(
      TripleType{
        ScalarType::String, ScalarType::Float, ScalarType::Bool, std::nullopt});
  }

  SECTION("from JSON") {
    json j = {
      {"triple", {{"first", "int"}, {"second", "int"}, {"third", "int"}}}};
    round_trip_json<TripleType>(j);
  }
}

TEST_CASE("TypeSpec round-trip", "[model][leaf]") {
  SECTION("scalar") {
    TypeSpec ts = ScalarType::String;
    round_trip(ts);
  }

  SECTION("list") {
    TypeSpec ts = ListType{ScalarType::String, ","};
    round_trip(ts);
  }

  SECTION("pair") {
    TypeSpec ts = PairType{ScalarType::String, ScalarType::Int, "="};
    round_trip(ts);
  }

  SECTION("triple") {
    TypeSpec ts =
      TripleType{ScalarType::Int, ScalarType::Int, ScalarType::Int, ","};
    round_trip(ts);
  }

  SECTION("from scalar JSON") { round_trip_json<TypeSpec>(json("string")); }

  SECTION("from list JSON") {
    json j = {{"list", {{"element", "string"}, {"separator", ","}}}};
    round_trip_json<TypeSpec>(j);
  }

  SECTION("from pair JSON") {
    json j = {{"pair", {{"first", "string"}, {"second", "int"}}}};
    round_trip_json<TypeSpec>(j);
  }

  SECTION("from triple JSON") {
    json j = {
      {"triple", {{"first", "int"}, {"second", "int"}, {"third", "int"}}}};
    round_trip_json<TypeSpec>(j);
  }
}

// ===========================================================================
// Phase 2: Environment & Metadata Types
// ===========================================================================

TEST_CASE("EnvBinding round-trip", "[model][env]") {
  SECTION("string form") {
    EnvBinding eb = std::string("MYAPP_VERBOSE");
    round_trip(eb);
  }

  SECTION("object form without doc") {
    EnvBinding eb = EnvBindingObj{"MYAPP_VERBOSE", std::nullopt};
    round_trip(eb);
  }

  SECTION("object form with doc") {
    EnvBinding eb = EnvBindingObj{"MYAPP_VERBOSE", DocString{"Enable verbose"}};
    round_trip(eb);
  }

  SECTION("from string JSON") {
    round_trip_json<EnvBinding>(json("MYAPP_VERBOSE"));
  }

  SECTION("from object JSON") {
    json j = {{"var", "MYAPP_VERBOSE"}, {"doc", {"Enable verbose"}}};
    round_trip_json<EnvBinding>(j);
  }
}

TEST_CASE("EnvInfo round-trip", "[model][env]") {
  SECTION("with doc") {
    round_trip(EnvInfo{"MYAPP_CONFIG", DocString{"Path to config"}});
  }

  SECTION("without doc") { round_trip(EnvInfo{"MYAPP_DEBUG", std::nullopt}); }

  SECTION("from JSON with doc") {
    json j = {{"var", "MYAPP_CONFIG"}, {"doc", {"Path to config"}}};
    round_trip_json<EnvInfo>(j);
  }

  SECTION("from JSON without doc") {
    json j = {{"var", "MYAPP_DEBUG"}};
    round_trip_json<EnvInfo>(j);
  }
}

TEST_CASE("ExitInfo round-trip", "[model][env]") {
  SECTION("simple exit code") {
    round_trip(ExitInfo{0, std::nullopt, {"Success"}});
  }

  SECTION("exit code range") {
    round_trip(ExitInfo{1, 99, {"Application error"}});
  }

  SECTION("from JSON without max") {
    json j = {{"code", 0}, {"doc", {"Success"}}};
    round_trip_json<ExitInfo>(j);
  }

  SECTION("from JSON with max") {
    json j = {{"code", 1}, {"max", 99}, {"doc", {"Application error"}}};
    round_trip_json<ExitInfo>(j);
  }
}

// ===========================================================================
// Phase 3: Argument Types
// ===========================================================================

TEST_CASE("Flag round-trip", "[model][args]") {
  SECTION("minimal flag") {
    Flag f;
    f.names = {"verbose"};
    f.doc = {"Be verbose"};
    round_trip(f);
  }

  SECTION("flag with all optional fields") {
    Flag f;
    f.names = {"verbose", "v"};
    f.doc = {"Be verbose"};
    f.dest = "verbose";
    f.env = EnvBindingObj{"MYAPP_VERBOSE", DocString{"Enable verbose"}};
    f.repeated = true;
    f.deprecated = "Use --log-level instead";
    f.docs = "COMMON OPTIONS";
    round_trip(f);
  }

  SECTION("from minimal JSON") {
    json j = {
      {"kind", "flag"}, {"names", {"verbose"}}, {"doc", {"Be verbose"}}};
    round_trip_json<Flag>(j);
  }

  SECTION("from full JSON") {
    json j = {
      {"kind", "flag"},
      {"names", {"verbose", "v"}},
      {"doc", {"Be verbose"}},
      {"dest", "verbose"},
      {"env", "MYAPP_VERBOSE"},
      {"repeated", true},
      {"deprecated", "Use --log-level instead"},
      {"docs", "COMMON OPTIONS"}};
    round_trip_json<Flag>(j);
  }
}

TEST_CASE("FlagGroupEntry round-trip", "[model][args]") {
  SECTION("from struct") {
    FlagGroupEntry e;
    e.names = {"quiet", "q"};
    e.doc = {"Quiet mode"};
    e.value = "quiet";
    round_trip(e);
  }

  SECTION("from JSON") {
    json j = {
      {"names", {"quiet", "q"}}, {"doc", {"Quiet mode"}}, {"value", "quiet"}};
    round_trip_json<FlagGroupEntry>(j);
  }
}

TEST_CASE("FlagGroup round-trip", "[model][args]") {
  SECTION("minimal flag group") {
    FlagGroup fg;
    fg.dest = "log_level";
    fg.doc = {"Logging level"};
    fg.default_value = "normal";
    FlagGroupEntry e1;
    e1.names = {"quiet"};
    e1.doc = {"Quiet"};
    e1.value = "quiet";
    FlagGroupEntry e2;
    e2.names = {"verbose"};
    e2.doc = {"Verbose"};
    e2.value = "verbose";
    fg.flags = {e1, e2};
    round_trip(fg);
  }

  SECTION("from JSON") {
    json j = {
      {"kind", "flag_group"},
      {"dest", "log_level"},
      {"doc", {"Logging level"}},
      {"default", "normal"},
      {"flags",
       {{{"names", {"quiet"}}, {"doc", {"Quiet"}}, {"value", "quiet"}},
        {{"names", {"verbose"}}, {"doc", {"Verbose"}}, {"value", "verbose"}}}}};
    round_trip_json<FlagGroup>(j);
  }

  SECTION("from JSON with repeated") {
    json j = {
      {"kind", "flag_group"},
      {"dest", "log_level"},
      {"doc", {"Logging level"}},
      {"default", "normal"},
      {"repeated", true},
      {"flags",
       {{{"names", {"quiet"}}, {"doc", {"Quiet"}}, {"value", "quiet"}},
        {{"names", {"verbose"}}, {"doc", {"Verbose"}}, {"value", "verbose"}}}}};
    round_trip_json<FlagGroup>(j);
  }
}

TEST_CASE("Option round-trip", "[model][args]") {
  SECTION("minimal option") {
    Option o;
    o.names = {"output", "o"};
    o.doc = {"Output file"};
    o.type = ScalarType::String;
    round_trip(o);
  }

  SECTION("option with all fields") {
    Option o;
    o.names = {"output", "o"};
    o.doc = {"Output file"};
    o.docv = "FILE";
    o.type = ScalarType::File;
    o.default_value = "-";
    o.required = false;
    o.repeated = false;
    o.must_exist = true;
    o.dest = "output";
    o.env = std::string("MYAPP_OUTPUT");
    o.docs = "OPTIONS";
    round_trip(o);
  }

  SECTION("option with enum type and choices") {
    Option o;
    o.names = {"format"};
    o.doc = {"Output format"};
    o.type = ScalarType::Enum;
    o.choices = std::vector<std::string>{"json", "yaml", "toml"};
    round_trip(o);
  }

  SECTION("option with list type") {
    Option o;
    o.names = {"includes"};
    o.doc = {"Include paths"};
    o.type = ListType{ScalarType::String, ","};
    round_trip(o);
  }

  SECTION("from minimal JSON") {
    json j = {
      {"kind", "option"},
      {"names", {"output", "o"}},
      {"doc", {"Output file"}},
      {"type", "string"}};
    round_trip_json<Option>(j);
  }

  SECTION("from full JSON") {
    json j = {
      {"kind", "option"},
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
      {"docs", "OPTIONS"}};
    round_trip_json<Option>(j);
  }

  SECTION("from JSON with choices") {
    json j = {
      {"kind", "option"},
      {"names", {"format"}},
      {"doc", {"Output format"}},
      {"type", "enum"},
      {"choices", {"json", "yaml", "toml"}}};
    round_trip_json<Option>(j);
  }

  SECTION("from JSON with list type") {
    json j = {
      {"kind", "option"},
      {"names", {"includes"}},
      {"doc", {"Include paths"}},
      {"type", {{"list", {{"element", "string"}, {"separator", ","}}}}}};
    round_trip_json<Option>(j);
  }
}

TEST_CASE("Positional round-trip", "[model][args]") {
  SECTION("minimal positional") {
    Positional p;
    p.name = "input";
    p.doc = {"Input file"};
    p.type = ScalarType::String;
    round_trip(p);
  }

  SECTION("positional with all fields") {
    Positional p;
    p.name = "input";
    p.doc = {"Input files"};
    p.docv = "FILE";
    p.type = ScalarType::File;
    p.default_value = nullptr;
    p.required = true;
    p.repeated = true;
    p.must_exist = true;
    p.docs = "ARGUMENTS";
    round_trip(p);
  }

  SECTION("from minimal JSON") {
    json j = {
      {"kind", "positional"},
      {"name", "input"},
      {"doc", {"Input file"}},
      {"type", "string"}};
    round_trip_json<Positional>(j);
  }

  SECTION("from full JSON") {
    json j = {
      {"kind", "positional"},
      {"name", "input"},
      {"doc", {"Input files"}},
      {"docv", "FILE"},
      {"type", "file"},
      {"default", nullptr},
      {"required", true},
      {"repeated", true},
      {"must_exist", true},
      {"docs", "ARGUMENTS"}};
    round_trip_json<Positional>(j);
  }
}

TEST_CASE("Argument variant round-trip", "[model][args]") {
  SECTION("flag") {
    Flag f;
    f.names = {"verbose"};
    f.doc = {"Be verbose"};
    Argument a = f;
    round_trip(a);
  }

  SECTION("flag_group") {
    FlagGroup fg;
    fg.dest = "level";
    fg.doc = {"Level"};
    fg.default_value = "normal";
    FlagGroupEntry e;
    e.names = {"quiet"};
    e.doc = {"Quiet"};
    e.value = "quiet";
    fg.flags = {e};
    Argument a = fg;
    round_trip(a);
  }

  SECTION("option") {
    Option o;
    o.names = {"output"};
    o.doc = {"Output"};
    o.type = ScalarType::String;
    Argument a = o;
    round_trip(a);
  }

  SECTION("positional") {
    Positional p;
    p.name = "input";
    p.doc = {"Input"};
    p.type = ScalarType::String;
    Argument a = p;
    round_trip(a);
  }

  SECTION("from flag JSON") {
    json j = {
      {"kind", "flag"}, {"names", {"verbose"}}, {"doc", {"Be verbose"}}};
    round_trip_json<Argument>(j);
  }

  SECTION("from option JSON") {
    json j = {
      {"kind", "option"},
      {"names", {"output"}},
      {"doc", {"Output"}},
      {"type", "string"}};
    round_trip_json<Argument>(j);
  }

  SECTION("from positional JSON") {
    json j = {
      {"kind", "positional"},
      {"name", "input"},
      {"doc", {"Input"}},
      {"type", "string"}};
    round_trip_json<Argument>(j);
  }

  SECTION("from flag_group JSON") {
    json j = {
      {"kind", "flag_group"},
      {"dest", "level"},
      {"doc", {"Level"}},
      {"default", "normal"},
      {"flags",
       {{{"names", {"quiet"}}, {"doc", {"Quiet"}}, {"value", "quiet"}}}}};
    round_trip_json<Argument>(j);
  }
}

// ===========================================================================
// Phase 4: Man Page Types
// ===========================================================================

TEST_CASE("ParagraphBlock round-trip", "[model][man]") {
  round_trip(ParagraphBlock{{"A useful tool."}});

  json j = {{"paragraph", {"A useful tool."}}};
  round_trip_json<ParagraphBlock>(j);
}

TEST_CASE("PreBlock round-trip", "[model][man]") {
  round_trip(PreBlock{{"$ myapp --verbose input.txt"}});

  json j = {{"pre", {"$ myapp --verbose input.txt"}}};
  round_trip_json<PreBlock>(j);
}

TEST_CASE("LabelTextBlock round-trip", "[model][man]") {
  round_trip(LabelTextBlock{"--verbose", {"Enable verbose output"}});

  json j = {{"label", "--verbose"}, {"text", {"Enable verbose output"}}};
  round_trip_json<LabelTextBlock>(j);
}

TEST_CASE("NoBlankBlock round-trip", "[model][man]") {
  round_trip(NoBlankBlock{});

  json j = {{"noblank", true}};
  round_trip_json<NoBlankBlock>(j);
}

TEST_CASE("ManBlock variant round-trip", "[model][man]") {
  SECTION("paragraph") {
    ManBlock mb = ParagraphBlock{{"text"}};
    round_trip(mb);
  }

  SECTION("pre") {
    ManBlock mb = PreBlock{{"$ cmd"}};
    round_trip(mb);
  }

  SECTION("label+text") {
    ManBlock mb = LabelTextBlock{"--flag", {"Description"}};
    round_trip(mb);
  }

  SECTION("noblank") {
    ManBlock mb = NoBlankBlock{};
    round_trip(mb);
  }

  SECTION("from paragraph JSON") {
    json j = {{"paragraph", {"text"}}};
    round_trip_json<ManBlock>(j);
  }

  SECTION("from pre JSON") {
    json j = {{"pre", {"$ cmd"}}};
    round_trip_json<ManBlock>(j);
  }

  SECTION("from label JSON") {
    json j = {{"label", "--flag"}, {"text", {"Description"}}};
    round_trip_json<ManBlock>(j);
  }

  SECTION("from noblank JSON") {
    json j = {{"noblank", true}};
    round_trip_json<ManBlock>(j);
  }
}

TEST_CASE("ManSection round-trip", "[model][man]") {
  ManSection s;
  s.name = "DESCRIPTION";
  s.blocks = {ParagraphBlock{{"A useful tool."}}, PreBlock{{"$ myapp --help"}}};
  round_trip(s);

  json j = {
    {"name", "DESCRIPTION"},
    {"blocks",
     {{{"paragraph", {"A useful tool."}}}, {{"pre", {"$ myapp --help"}}}}}};
  round_trip_json<ManSection>(j);
}

TEST_CASE("ManXref round-trip", "[model][man]") {
  round_trip(ManXref{"git", 1});
  round_trip(ManXref{"zlib", 3});

  json j = {{"name", "git"}, {"section", 1}};
  round_trip_json<ManXref>(j);
}

TEST_CASE("Man round-trip", "[model][man]") {
  SECTION("section only") {
    Man m;
    m.section = 1;
    round_trip(m);
  }

  SECTION("full man") {
    Man m;
    m.section = 1;
    m.sections = std::vector<ManSection>{{
      ManSection{"DESCRIPTION", {ParagraphBlock{{"A tool."}}}, {}},
    }};
    m.xrefs = std::vector<ManXref>{{"git", 1}};
    round_trip(m);
  }

  SECTION("from JSON section only") {
    json j = {{"section", 1}};
    round_trip_json<Man>(j);
  }

  SECTION("from full JSON") {
    json j = {
      {"section", 1},
      {"sections",
       {{{"name", "DESCRIPTION"}, {"blocks", {{{"paragraph", {"A tool."}}}}}}}},
      {"xrefs", {{{"name", "git"}, {"section", 1}}}}};
    round_trip_json<Man>(j);
  }
}

// ===========================================================================
// Phase 5: Config & Command Types
// ===========================================================================

TEST_CASE("ConfigPaths round-trip", "[model][config]") {
  SECTION("all paths") {
    round_trip(
      ConfigPaths{
        "/etc/myapp/config.json",
        "~/.config/myapp/config.json",
        ".myapp.json"});
  }

  SECTION("partial paths") {
    round_trip(ConfigPaths{std::nullopt, std::nullopt, ".myapp.toml"});
  }

  SECTION("from full JSON") {
    json j = {
      {"system", "/etc/myapp/config.json"},
      {"user", "~/.config/myapp/config.json"},
      {"local", ".myapp.json"}};
    round_trip_json<ConfigPaths>(j);
  }

  SECTION("from partial JSON") {
    json j = {{"local", ".myapp.toml"}};
    round_trip_json<ConfigPaths>(j);
  }
}

TEST_CASE("Config round-trip", "[model][config]") {
  SECTION("format only") {
    Config c;
    c.format = "json";
    round_trip(c);
  }

  SECTION("with paths") {
    Config c;
    c.format = "json";
    c.paths = ConfigPaths{
      "/etc/myapp/config.json", "~/.config/myapp/config.json", ".myapp.json"};
    round_trip(c);
  }

  SECTION("from JSON format only") {
    json j = {{"format", "json"}};
    round_trip_json<Config>(j);
  }

  SECTION("from full JSON") {
    json j = {
      {"format", "json"},
      {"paths",
       {{"system", "/etc/myapp/config.json"},
        {"user", "~/.config/myapp/config.json"},
        {"local", ".myapp.json"}}}};
    round_trip_json<Config>(j);
  }
}

TEST_CASE("Command round-trip", "[model][command]") {
  SECTION("minimal command") {
    Command cmd;
    cmd.name = "build";
    cmd.doc = {"Build the project"};
    round_trip(cmd);
  }

  SECTION("command with args") {
    Command cmd;
    cmd.name = "build";
    cmd.doc = {"Build the project"};
    Flag f;
    f.names = {"release"};
    f.doc = {"Release build"};
    cmd.args = std::vector<Argument>{f};
    round_trip(cmd);
  }

  SECTION("command with subcommands") {
    Command add_cmd;
    add_cmd.name = "add";
    add_cmd.doc = {"Add a remote"};

    Command remove_cmd;
    remove_cmd.name = "remove";
    remove_cmd.doc = {"Remove a remote"};

    Command remote_cmd;
    remote_cmd.name = "remote";
    remote_cmd.doc = {"Manage remotes"};
    remote_cmd.commands = std::vector<Command>{add_cmd, remove_cmd};
    round_trip(remote_cmd);
  }

  SECTION("from minimal JSON") {
    json j = {{"name", "build"}, {"doc", {"Build the project"}}};
    round_trip_json<Command>(j);
  }

  SECTION("from JSON with subcommands") {
    json j = {
      {"name", "remote"},
      {"doc", {"Manage remotes"}},
      {"commands",
       {{{"name", "add"}, {"doc", {"Add a remote"}}},
        {{"name", "remove"}, {"doc", {"Remove a remote"}}}}}};
    round_trip_json<Command>(j);
  }
}

TEST_CASE("Root round-trip", "[model][command]") {
  SECTION("minimal root") {
    Root r;
    r.name = "myapp";
    r.doc = {"A test application"};
    round_trip(r);
  }

  SECTION("root with version") {
    Root r;
    r.name = "myapp";
    r.doc = {"A test application"};
    r.version = "1.2.3";
    round_trip(r);
  }

  SECTION("root with config") {
    Root r;
    r.name = "myapp";
    r.doc = {"A test application"};
    r.version = "1.0";
    Config c;
    c.format = "json";
    r.config = c;
    round_trip(r);
  }

  SECTION("root with everything") {
    Root r;
    r.name = "myapp";
    r.doc = {"A test application"};
    r.version = "2.1.0";

    Config c;
    c.format = "json";
    c.paths = ConfigPaths{
      "/etc/myapp/config.json", "~/.config/myapp/config.json", ".myapp.json"};
    r.config = c;

    Flag f;
    f.names = {"verbose", "v"};
    f.doc = {"Be verbose"};
    r.args = std::vector<Argument>{f};

    Command sub;
    sub.name = "build";
    sub.doc = {"Build"};
    r.commands = std::vector<Command>{sub};

    Man m;
    m.section = 1;
    r.man = m;

    r.envs =
      std::vector<EnvInfo>{{"MYAPP_CONFIG", DocString{"Path to config"}}};
    r.exits = std::vector<ExitInfo>{{0, std::nullopt, {"Success"}}};

    round_trip(r);
  }

  SECTION("from minimal JSON") {
    json j = {{"name", "myapp"}, {"doc", {"A test application"}}};
    round_trip_json<Root>(j);
  }

  SECTION("from JSON with version and config") {
    json j = {
      {"name", "myapp"},
      {"doc", {"A test application"}},
      {"version", "1.2.3"},
      {"config", {{"format", "json"}}}};
    round_trip_json<Root>(j);
  }
}

// ===========================================================================
// Phase 6: Integration — realistic schema round-trip
// ===========================================================================

TEST_CASE(
  "Realistic CLI schema round-trips through data model",
  "[model][integration]") {
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
          {{{"paragraph", {"myapp processes files according to rules.", "It supports multiple input formats.", "", "Output can be written to a file or stdout."}}}}}},
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
        {{{"names", {"quiet", "q"}},
          {"doc", {"Quiet mode"}},
          {"value", "quiet"}},
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

  Root root;
  from_json(realistic, root);
  json output;
  to_json(output, root);

  REQUIRE(output == realistic);
}
