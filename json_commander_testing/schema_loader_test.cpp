#include <catch2/catch_test_macros.hpp>
#include <json_commander/schema_loader.hpp>

#include <filesystem>

using namespace json_commander::schema;

// ---------------------------------------------------------------------------
// Phase 1: Error type and Loader construction
// ---------------------------------------------------------------------------

TEST_CASE("Error carries its message", "[schema_loader]") {
  Error e("something went wrong");
  REQUIRE(std::string(e.what()) == "something went wrong");
}

TEST_CASE("Loader construction does not throw", "[schema_loader]") {
  REQUIRE_NOTHROW(Loader());
}

// ---------------------------------------------------------------------------
// Phase 2: Validation failure tests
// ---------------------------------------------------------------------------

TEST_CASE("load(json) with empty object throws Error", "[schema_loader]") {
  Loader loader;
  REQUIRE_THROWS_AS(loader.load(nlohmann::json::object()), Error);
}

TEST_CASE(
  "load(json) with invalid name format throws Error", "[schema_loader]") {
  Loader loader;
  nlohmann::json bad = {{"name", "1app"}, {"doc", {"A description"}}};
  REQUIRE_THROWS_AS(loader.load(bad), Error);
}

TEST_CASE(
  "load(json) with unknown top-level property throws Error",
  "[schema_loader]") {
  Loader loader;
  nlohmann::json bad = {
    {"name", "myapp"}, {"doc", {"A description"}}, {"unknown_field", "value"}};
  REQUIRE_THROWS_AS(loader.load(bad), Error);
}

TEST_CASE("load(path) with nonexistent file throws Error", "[schema_loader]") {
  Loader loader;
  REQUIRE_THROWS_AS(
    loader.load(std::string("/tmp/nonexistent_commander_schema_12345.json")),
    Error);
}

TEST_CASE(
  "load(path) with invalid JSON content throws Error", "[schema_loader]") {
  Loader loader;
  std::string path = "/tmp/commander_bad_schema_test.json";
  {
    std::ofstream out(path);
    out << "{ not valid json }";
  }
  REQUIRE_THROWS_AS(loader.load(path), Error);
  std::remove(path.c_str());
}

// ---------------------------------------------------------------------------
// Phase 3: Successful load tests
// ---------------------------------------------------------------------------

TEST_CASE("load(json) with minimal schema returns Root", "[schema_loader]") {
  Loader loader;
  nlohmann::json j = {{"name", "app"}, {"doc", {"A description"}}};
  auto root = loader.load(j);
  REQUIRE(root.name == "app");
  REQUIRE(root.doc == json_commander::model::DocString{"A description"});
  REQUIRE_FALSE(root.version.has_value());
  REQUIRE_FALSE(root.config.has_value());
  REQUIRE_FALSE(root.args.has_value());
  REQUIRE_FALSE(root.commands.has_value());
}

TEST_CASE("load(json) with realistic schema round-trips", "[schema_loader]") {
  using json = nlohmann::json;

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

  Loader loader;
  auto root = loader.load(realistic);
  nlohmann::json output = root;
  REQUIRE(output == realistic);
}

TEST_CASE("load(path) with valid file returns Root", "[schema_loader]") {
  nlohmann::json j = {
    {"name", "app"}, {"doc", {"A test app"}}, {"version", "1.0.0"}};
  std::string path = "/tmp/commander_valid_schema_test.json";
  {
    std::ofstream out(path);
    out << j.dump(2);
  }
  Loader loader;
  auto root = loader.load(path);
  REQUIRE(root.name == "app");
  REQUIRE(root.version == "1.0.0");
  std::remove(path.c_str());
}

// ---------------------------------------------------------------------------
// Phase 4: Integration — metaschema file is not a valid CLI schema
// ---------------------------------------------------------------------------

TEST_CASE(
  "load(path) rejects metaschema file as invalid CLI schema",
  "[schema_loader]") {
  Loader loader;
  std::string metaschema_path =
    std::string(METASCHEMA_DIR) + "/json_commander.schema.json";
  REQUIRE_THROWS_AS(loader.load(metaschema_path), Error);
}

// ---------------------------------------------------------------------------
// Phase 5: External file references for subcommands
// ---------------------------------------------------------------------------

namespace {

  // Helper to write JSON to a file and ensure cleanup
  struct TempFile {
    std::string path;
    TempFile(const std::string& p, const nlohmann::json& j)
        : path(p) {
      std::filesystem::create_directories(
        std::filesystem::path(p).parent_path());
      std::ofstream out(p);
      out << j.dump(2);
    }
    ~TempFile() { std::remove(path.c_str()); }
  };

  struct TempDir {
    std::string path;
    explicit TempDir(const std::string& p)
        : path(p) {
      std::filesystem::create_directories(p);
    }
    ~TempDir() { std::filesystem::remove_all(path); }
  };

} // namespace

TEST_CASE(
  "load(path) resolves string entries in commands as external files",
  "[schema_loader][external]") {
  TempDir dir("/tmp/jcmd_test_external");

  nlohmann::json deploy_cmd = {
    {"name", "deploy"},
    {"doc", {"Deploy the app."}},
    {"args",
     {{{"kind", "option"},
       {"names", {"target"}},
       {"doc", {"Deploy target."}},
       {"type", "string"}}}}};

  TempFile deploy_file("/tmp/jcmd_test_external/deploy.json", deploy_cmd);

  nlohmann::json root_json = {
    {"name", "mytool"},
    {"doc", {"A tool."}},
    {"commands",
     {{{"name", "build"}, {"doc", {"Build stuff."}}}, "deploy.json"}}};

  TempFile root_file("/tmp/jcmd_test_external/mytool.json", root_json);

  Loader loader;
  auto root = loader.load(std::string("/tmp/jcmd_test_external/mytool.json"));

  REQUIRE(root.commands.has_value());
  REQUIRE(root.commands->size() == 2);
  REQUIRE((*root.commands)[0].name == "build");
  REQUIRE((*root.commands)[1].name == "deploy");
  REQUIRE((*root.commands)[1].args.has_value());
}

TEST_CASE(
  "load(path) resolves nested external references recursively",
  "[schema_loader][external]") {
  TempDir dir("/tmp/jcmd_test_nested");

  nlohmann::json leaf_cmd = {{"name", "leaf"}, {"doc", {"A leaf command."}}};

  TempFile leaf_file("/tmp/jcmd_test_nested/leaf.json", leaf_cmd);

  nlohmann::json mid_cmd = {
    {"name", "mid"},
    {"doc", {"A middle command."}},
    {"commands", {"leaf.json"}}};

  TempFile mid_file("/tmp/jcmd_test_nested/mid.json", mid_cmd);

  nlohmann::json root_json = {
    {"name", "mytool"}, {"doc", {"A tool."}}, {"commands", {"mid.json"}}};

  TempFile root_file("/tmp/jcmd_test_nested/mytool.json", root_json);

  Loader loader;
  auto root = loader.load(std::string("/tmp/jcmd_test_nested/mytool.json"));

  REQUIRE(root.commands.has_value());
  REQUIRE(root.commands->size() == 1);
  REQUIRE((*root.commands)[0].name == "mid");
  REQUIRE((*root.commands)[0].commands.has_value());
  REQUIRE((*root.commands)[0].commands->size() == 1);
  REQUIRE((*root.commands)[0].commands->at(0).name == "leaf");
}

TEST_CASE(
  "load(path) resolves external files in subdirectories",
  "[schema_loader][external]") {
  TempDir dir("/tmp/jcmd_test_subdir");

  nlohmann::json deploy_cmd = {
    {"name", "deploy"}, {"doc", {"Deploy the app."}}};

  TempFile deploy_file(
    "/tmp/jcmd_test_subdir/commands/deploy.json", deploy_cmd);

  nlohmann::json root_json = {
    {"name", "mytool"},
    {"doc", {"A tool."}},
    {"commands", {"commands/deploy.json"}}};

  TempFile root_file("/tmp/jcmd_test_subdir/mytool.json", root_json);

  Loader loader;
  auto root = loader.load(std::string("/tmp/jcmd_test_subdir/mytool.json"));

  REQUIRE(root.commands.has_value());
  REQUIRE(root.commands->size() == 1);
  REQUIRE((*root.commands)[0].name == "deploy");
}

TEST_CASE(
  "load(path) throws Error for missing external file",
  "[schema_loader][external]") {
  TempDir dir("/tmp/jcmd_test_missing");

  nlohmann::json root_json = {
    {"name", "mytool"},
    {"doc", {"A tool."}},
    {"commands", {"nonexistent.json"}}};

  TempFile root_file("/tmp/jcmd_test_missing/mytool.json", root_json);

  Loader loader;
  REQUIRE_THROWS_AS(
    loader.load(std::string("/tmp/jcmd_test_missing/mytool.json")), Error);
}

TEST_CASE(
  "load(path) with inline-only commands still works",
  "[schema_loader][external]") {
  TempDir dir("/tmp/jcmd_test_inline");

  nlohmann::json root_json = {
    {"name", "mytool"},
    {"doc", {"A tool."}},
    {"commands", {{{"name", "build"}, {"doc", {"Build stuff."}}}}}};

  TempFile root_file("/tmp/jcmd_test_inline/mytool.json", root_json);

  Loader loader;
  auto root = loader.load(std::string("/tmp/jcmd_test_inline/mytool.json"));

  REQUIRE(root.commands.has_value());
  REQUIRE(root.commands->size() == 1);
  REQUIRE((*root.commands)[0].name == "build");
}

TEST_CASE(
  "load(path) mixes inline and external commands",
  "[schema_loader][external]") {
  TempDir dir("/tmp/jcmd_test_mixed");

  nlohmann::json deploy_cmd = {
    {"name", "deploy"}, {"doc", {"Deploy the app."}}};

  TempFile deploy_file("/tmp/jcmd_test_mixed/deploy.json", deploy_cmd);

  nlohmann::json root_json = {
    {"name", "mytool"},
    {"doc", {"A tool."}},
    {"commands",
     {{{"name", "build"}, {"doc", {"Build stuff."}}},
      "deploy.json",
      {{"name", "test"}, {"doc", {"Run tests."}}}}}};

  TempFile root_file("/tmp/jcmd_test_mixed/mytool.json", root_json);

  Loader loader;
  auto root = loader.load(std::string("/tmp/jcmd_test_mixed/mytool.json"));

  REQUIRE(root.commands.has_value());
  REQUIRE(root.commands->size() == 3);
  REQUIRE((*root.commands)[0].name == "build");
  REQUIRE((*root.commands)[1].name == "deploy");
  REQUIRE((*root.commands)[2].name == "test");
}

TEST_CASE(
  "load(json) without file path ignores string command entries",
  "[schema_loader][external]") {
  Loader loader;
  nlohmann::json root_json = {
    {"name", "mytool"}, {"doc", {"A tool."}}, {"commands", {"deploy.json"}}};

  // load(json) has no base path so it cannot resolve external refs;
  // the string entry fails schema validation as expected
  REQUIRE_THROWS_AS(loader.load(root_json), Error);
}
