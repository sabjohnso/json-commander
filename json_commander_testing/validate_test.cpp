#include <catch2/catch_test_macros.hpp>
#include <json_commander/validate.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>

using namespace json_commander::validate;
using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Phase 1: Error type and Validator struct
// ---------------------------------------------------------------------------

TEST_CASE("Error carries its message", "[validate]") {
  Error e("something went wrong");
  REQUIRE(std::string(e.what()) == "something went wrong");
}

TEST_CASE("Error is catchable as std::runtime_error", "[validate]") {
  try {
    throw Error("test");
  } catch (const std::runtime_error& e) {
    REQUIRE(std::string(e.what()) == "test");
  }
}

TEST_CASE("Validator with no-op check does not throw", "[validate]") {
  Validator v{[](const std::string&, const std::optional<json>&) {}, "no-op"};
  REQUIRE_NOTHROW(v.check("--arg", json("hello")));
}

TEST_CASE("Validator with throwing check throws Error", "[validate]") {
  Validator v{
    [](const std::string& name, const std::optional<json>&) {
      throw Error(name + " failed");
    },
    "always-fail",
  };
  REQUIRE_THROWS_AS(v.check("--arg", json("hello")), Error);
}

// ---------------------------------------------------------------------------
// Phase 2: required() validator
// ---------------------------------------------------------------------------

TEST_CASE("required passes for present string value", "[validate]") {
  REQUIRE_NOTHROW(required().check("--input", json("hello")));
}

TEST_CASE("required passes for present null value", "[validate]") {
  REQUIRE_NOTHROW(required().check("--input", json(nullptr)));
}

TEST_CASE("required passes for present integer value", "[validate]") {
  REQUIRE_NOTHROW(required().check("--count", json(42)));
}

TEST_CASE("required passes for present array value", "[validate]") {
  REQUIRE_NOTHROW(required().check("--files", json::array({"a", "b"})));
}

TEST_CASE("required throws for absent value", "[validate]") {
  REQUIRE_THROWS_AS(required().check("--input", std::nullopt), Error);
}

TEST_CASE("required error message includes argument name", "[validate]") {
  try {
    required().check("--input", std::nullopt);
    FAIL("expected Error to be thrown");
  } catch (const Error& e) {
    std::string msg = e.what();
    CHECK(msg.find("--input") != std::string::npos);
    CHECK(msg.find("required") != std::string::npos);
  }
}

TEST_CASE("required has description 'required'", "[validate]") {
  REQUIRE(required().description == "required");
}

// ---------------------------------------------------------------------------
// Phase 3: must_exist validators
// ---------------------------------------------------------------------------

namespace {

  struct TempFile {
    std::string path;

    TempFile()
        : path("/tmp/commander_validate_test_file") {
      std::ofstream ofs(path);
      ofs << "test";
    }

    ~TempFile() { std::remove(path.c_str()); }
  };

  const std::string absent_path = "/tmp/commander_nonexistent_xyz_abc_123";
  const std::string known_dir = "/tmp";

} // namespace

// --- must_exist_file ---

TEST_CASE("must_exist_file passes for existing regular file", "[validate]") {
  TempFile tf;
  REQUIRE_NOTHROW(must_exist_file().check("--input", json(tf.path)));
}

TEST_CASE("must_exist_file throws for non-existent path", "[validate]") {
  REQUIRE_THROWS_AS(
    must_exist_file().check("--input", json(absent_path)), Error);
}

TEST_CASE("must_exist_file throws for directory", "[validate]") {
  REQUIRE_THROWS_AS(must_exist_file().check("--input", json(known_dir)), Error);
}

TEST_CASE("must_exist_file passes for absent value (nullopt)", "[validate]") {
  REQUIRE_NOTHROW(must_exist_file().check("--input", std::nullopt));
}

TEST_CASE(
  "must_exist_file error includes argument name and path", "[validate]") {
  try {
    must_exist_file().check("--input", json(absent_path));
    FAIL("expected Error to be thrown");
  } catch (const Error& e) {
    std::string msg = e.what();
    CHECK(msg.find("--input") != std::string::npos);
    CHECK(msg.find(absent_path) != std::string::npos);
  }
}

TEST_CASE("must_exist_file has description 'must_exist(file)'", "[validate]") {
  REQUIRE(must_exist_file().description == "must_exist(file)");
}

// --- must_exist_dir ---

TEST_CASE("must_exist_dir passes for existing directory", "[validate]") {
  REQUIRE_NOTHROW(must_exist_dir().check("--output", json(known_dir)));
}

TEST_CASE("must_exist_dir throws for regular file", "[validate]") {
  TempFile tf;
  REQUIRE_THROWS_AS(must_exist_dir().check("--output", json(tf.path)), Error);
}

TEST_CASE("must_exist_dir throws for non-existent path", "[validate]") {
  REQUIRE_THROWS_AS(
    must_exist_dir().check("--output", json(absent_path)), Error);
}

TEST_CASE("must_exist_dir passes for absent value (nullopt)", "[validate]") {
  REQUIRE_NOTHROW(must_exist_dir().check("--output", std::nullopt));
}

TEST_CASE("must_exist_dir has description 'must_exist(dir)'", "[validate]") {
  REQUIRE(must_exist_dir().description == "must_exist(dir)");
}

// --- must_exist_path ---

TEST_CASE("must_exist_path passes for existing file", "[validate]") {
  TempFile tf;
  REQUIRE_NOTHROW(must_exist_path().check("--path", json(tf.path)));
}

TEST_CASE("must_exist_path passes for existing directory", "[validate]") {
  REQUIRE_NOTHROW(must_exist_path().check("--path", json(known_dir)));
}

TEST_CASE("must_exist_path throws for non-existent path", "[validate]") {
  REQUIRE_THROWS_AS(
    must_exist_path().check("--path", json(absent_path)), Error);
}

TEST_CASE("must_exist_path passes for absent value (nullopt)", "[validate]") {
  REQUIRE_NOTHROW(must_exist_path().check("--path", std::nullopt));
}

TEST_CASE("must_exist_path has description 'must_exist(path)'", "[validate]") {
  REQUIRE(must_exist_path().description == "must_exist(path)");
}

// ---------------------------------------------------------------------------
// Phase 4: all_of composition
// ---------------------------------------------------------------------------

TEST_CASE("all_of empty is no-op for any input", "[validate]") {
  auto v = all_of({});
  REQUIRE_NOTHROW(v.check("--arg", json("hello")));
  REQUIRE_NOTHROW(v.check("--arg", std::nullopt));
}

TEST_CASE("all_of empty has description 'none'", "[validate]") {
  REQUIRE(all_of({}).description == "none");
}

TEST_CASE("all_of with single required throws for nullopt", "[validate]") {
  auto v = all_of({required()});
  REQUIRE_THROWS_AS(v.check("--arg", std::nullopt), Error);
}

TEST_CASE("all_of with single required passes for value", "[validate]") {
  auto v = all_of({required()});
  REQUIRE_NOTHROW(v.check("--arg", json("hello")));
}

TEST_CASE(
  "all_of with single required has description 'required'", "[validate]") {
  REQUIRE(all_of({required()}).description == "required");
}

TEST_CASE(
  "all_of short-circuits: required before must_exist_file", "[validate]") {
  auto v = all_of({required(), must_exist_file()});
  try {
    v.check("--input", std::nullopt);
    FAIL("expected Error to be thrown");
  } catch (const Error& e) {
    std::string msg = e.what();
    CHECK(msg.find("required") != std::string::npos);
  }
}

TEST_CASE(
  "all_of: required + must_exist_file throws for non-existent file",
  "[validate]") {
  auto v = all_of({required(), must_exist_file()});
  REQUIRE_THROWS_AS(v.check("--input", json(absent_path)), Error);
}

TEST_CASE("all_of: required + must_exist_dir passes for /tmp", "[validate]") {
  auto v = all_of({required(), must_exist_dir()});
  REQUIRE_NOTHROW(v.check("--output", json(known_dir)));
}

TEST_CASE("all_of joins descriptions with ' + '", "[validate]") {
  auto v = all_of({required(), must_exist_file()});
  REQUIRE(v.description == "required + must_exist(file)");
}

// ---------------------------------------------------------------------------
// Phase 5: Factory functions (from_option, from_positional)
// ---------------------------------------------------------------------------

using namespace json_commander::model;

// --- helpers for building model types without triggering -Wmissing-field ---

Option
make_option(ArgNames names, TypeSpec type) {
  Option opt{};
  opt.names = std::move(names);
  opt.doc = {"doc"};
  opt.type = std::move(type);
  return opt;
}

Positional
make_positional(std::string name, TypeSpec type) {
  Positional pos{};
  pos.name = std::move(name);
  pos.doc = {"doc"};
  pos.type = std::move(type);
  return pos;
}

// --- from_option ---

TEST_CASE(
  "from_option: no constraints produces no-op validator", "[validate]") {
  auto opt = make_option({"output"}, ScalarType::String);
  auto v = from_option(opt);
  REQUIRE_NOTHROW(v.check("--output", std::nullopt));
  REQUIRE_NOTHROW(v.check("--output", json("hello")));
}

TEST_CASE("from_option: required=true checks required", "[validate]") {
  auto opt = make_option({"input"}, ScalarType::String);
  opt.required = true;
  auto v = from_option(opt);
  REQUIRE_THROWS_AS(v.check("--input", std::nullopt), Error);
  REQUIRE_NOTHROW(v.check("--input", json("hello")));
}

TEST_CASE("from_option: required=false does not check required", "[validate]") {
  auto opt = make_option({"input"}, ScalarType::String);
  opt.required = false;
  auto v = from_option(opt);
  REQUIRE_NOTHROW(v.check("--input", std::nullopt));
}

TEST_CASE(
  "from_option: type=File + must_exist checks file existence", "[validate]") {
  TempFile tf;
  auto opt = make_option({"input"}, ScalarType::File);
  opt.must_exist = true;
  auto v = from_option(opt);
  REQUIRE_NOTHROW(v.check("--input", json(tf.path)));
  REQUIRE_THROWS_AS(v.check("--input", json(absent_path)), Error);
}

TEST_CASE(
  "from_option: type=Dir + must_exist checks directory existence",
  "[validate]") {
  auto opt = make_option({"output"}, ScalarType::Dir);
  opt.must_exist = true;
  auto v = from_option(opt);
  REQUIRE_NOTHROW(v.check("--output", json(known_dir)));
  REQUIRE_THROWS_AS(v.check("--output", json(absent_path)), Error);
}

TEST_CASE(
  "from_option: type=Path + must_exist checks path existence", "[validate]") {
  TempFile tf;
  auto opt = make_option({"target"}, ScalarType::Path);
  opt.must_exist = true;
  auto v = from_option(opt);
  REQUIRE_NOTHROW(v.check("--target", json(tf.path)));
  REQUIRE_NOTHROW(v.check("--target", json(known_dir)));
  REQUIRE_THROWS_AS(v.check("--target", json(absent_path)), Error);
}

TEST_CASE("from_option: type=String + must_exist is no-op", "[validate]") {
  auto opt = make_option({"name"}, ScalarType::String);
  opt.must_exist = true;
  auto v = from_option(opt);
  REQUIRE_NOTHROW(v.check("--name", json("anything")));
}

TEST_CASE("from_option: required + must_exist composes both", "[validate]") {
  auto opt = make_option({"input"}, ScalarType::File);
  opt.required = true;
  opt.must_exist = true;
  auto v = from_option(opt);
  REQUIRE_THROWS_AS(v.check("--input", std::nullopt), Error);
  REQUIRE_THROWS_AS(v.check("--input", json(absent_path)), Error);
  TempFile tf;
  REQUIRE_NOTHROW(v.check("--input", json(tf.path)));
}

TEST_CASE(
  "from_option: description reflects composed validators", "[validate]") {
  auto opt = make_option({"input"}, ScalarType::File);
  opt.required = true;
  opt.must_exist = true;
  auto v = from_option(opt);
  CHECK(v.description.find("required") != std::string::npos);
  CHECK(v.description.find("must_exist(file)") != std::string::npos);
}

// --- from_positional ---

TEST_CASE(
  "from_positional: no constraints produces no-op validator", "[validate]") {
  auto pos = make_positional("filename", ScalarType::String);
  auto v = from_positional(pos);
  REQUIRE_NOTHROW(v.check("filename", std::nullopt));
  REQUIRE_NOTHROW(v.check("filename", json("hello")));
}

TEST_CASE("from_positional: required=true checks required", "[validate]") {
  auto pos = make_positional("filename", ScalarType::String);
  pos.required = true;
  auto v = from_positional(pos);
  REQUIRE_THROWS_AS(v.check("filename", std::nullopt), Error);
  REQUIRE_NOTHROW(v.check("filename", json("hello")));
}

TEST_CASE("from_positional: type=File + must_exist checks file", "[validate]") {
  TempFile tf;
  auto pos = make_positional("filename", ScalarType::File);
  pos.must_exist = true;
  auto v = from_positional(pos);
  REQUIRE_NOTHROW(v.check("filename", json(tf.path)));
  REQUIRE_THROWS_AS(v.check("filename", json(absent_path)), Error);
}

TEST_CASE(
  "from_positional: required + must_exist composes both", "[validate]") {
  auto pos = make_positional("filename", ScalarType::File);
  pos.required = true;
  pos.must_exist = true;
  auto v = from_positional(pos);
  REQUIRE_THROWS_AS(v.check("filename", std::nullopt), Error);
  REQUIRE_THROWS_AS(v.check("filename", json(absent_path)), Error);
  TempFile tf;
  REQUIRE_NOTHROW(v.check("filename", json(tf.path)));
}

// ---------------------------------------------------------------------------
// Phase 6: Compound type must_exist
// ---------------------------------------------------------------------------

// --- list ---

TEST_CASE("list(file) + must_exist: fails on first bad path", "[validate]") {
  TempFile tf;
  auto opt = make_option({"files"}, ListType{ScalarType::File, std::nullopt});
  opt.must_exist = true;
  auto v = from_option(opt);
  REQUIRE_THROWS_AS(
    v.check("--files", json::array({tf.path, absent_path})), Error);
}

TEST_CASE(
  "list(file) + must_exist: passes when all elements exist", "[validate]") {
  TempFile tf;
  auto opt = make_option({"files"}, ListType{ScalarType::File, std::nullopt});
  opt.must_exist = true;
  auto v = from_option(opt);
  REQUIRE_NOTHROW(v.check("--files", json::array({tf.path})));
}

TEST_CASE("list(file) + must_exist: passes for empty array", "[validate]") {
  auto opt = make_option({"files"}, ListType{ScalarType::File, std::nullopt});
  opt.must_exist = true;
  auto v = from_option(opt);
  REQUIRE_NOTHROW(v.check("--files", json::array()));
}

TEST_CASE("list(string) + must_exist: no-op", "[validate]") {
  auto opt = make_option({"names"}, ListType{ScalarType::String, std::nullopt});
  opt.must_exist = true;
  auto v = from_option(opt);
  REQUIRE_NOTHROW(v.check("--names", json::array({"anything", "else"})));
}

TEST_CASE("list(dir) + must_exist: checks directories", "[validate]") {
  auto opt = make_option({"dirs"}, ListType{ScalarType::Dir, std::nullopt});
  opt.must_exist = true;
  auto v = from_option(opt);
  REQUIRE_NOTHROW(v.check("--dirs", json::array({known_dir})));
  REQUIRE_THROWS_AS(v.check("--dirs", json::array({absent_path})), Error);
}

// --- pair ---

TEST_CASE(
  "pair(string, file) + must_exist: checks only index 1", "[validate]") {
  TempFile tf;
  auto opt = make_option(
    {"kv"}, PairType{ScalarType::String, ScalarType::File, std::nullopt});
  opt.must_exist = true;
  auto v = from_option(opt);
  REQUIRE_NOTHROW(v.check("--kv", json::array({"key", tf.path})));
  REQUIRE_THROWS_AS(v.check("--kv", json::array({"key", absent_path})), Error);
}

TEST_CASE("pair(file, int) + must_exist: checks only index 0", "[validate]") {
  TempFile tf;
  auto opt = make_option(
    {"fi"}, PairType{ScalarType::File, ScalarType::Int, std::nullopt});
  opt.must_exist = true;
  auto v = from_option(opt);
  REQUIRE_NOTHROW(v.check("--fi", json::array({tf.path, 42})));
  REQUIRE_THROWS_AS(v.check("--fi", json::array({absent_path, 42})), Error);
}

TEST_CASE("pair(file, file) + must_exist: checks both", "[validate]") {
  TempFile tf;
  auto opt = make_option(
    {"ff"}, PairType{ScalarType::File, ScalarType::File, std::nullopt});
  opt.must_exist = true;
  auto v = from_option(opt);
  REQUIRE_NOTHROW(v.check("--ff", json::array({tf.path, tf.path})));
  REQUIRE_THROWS_AS(
    v.check("--ff", json::array({tf.path, absent_path})), Error);
  REQUIRE_THROWS_AS(
    v.check("--ff", json::array({absent_path, tf.path})), Error);
}

TEST_CASE("pair(string, int) + must_exist: no-op", "[validate]") {
  auto opt = make_option(
    {"si"}, PairType{ScalarType::String, ScalarType::Int, std::nullopt});
  opt.must_exist = true;
  auto v = from_option(opt);
  REQUIRE_NOTHROW(v.check("--si", json::array({"hello", 42})));
}

// --- triple ---

TEST_CASE(
  "triple(file, int, dir) + must_exist: checks indices 0 and 2", "[validate]") {
  TempFile tf;
  auto opt = make_option(
    {"fid"},
    TripleType{
      ScalarType::File, ScalarType::Int, ScalarType::Dir, std::nullopt});
  opt.must_exist = true;
  auto v = from_option(opt);
  REQUIRE_NOTHROW(v.check("--fid", json::array({tf.path, 42, known_dir})));
  REQUIRE_THROWS_AS(
    v.check("--fid", json::array({absent_path, 42, known_dir})), Error);
  REQUIRE_THROWS_AS(
    v.check("--fid", json::array({tf.path, 42, absent_path})), Error);
}

TEST_CASE("triple(string, int, float) + must_exist: no-op", "[validate]") {
  auto opt = make_option(
    {"sif"},
    TripleType{
      ScalarType::String, ScalarType::Int, ScalarType::Float, std::nullopt});
  opt.must_exist = true;
  auto v = from_option(opt);
  REQUIRE_NOTHROW(v.check("--sif", json::array({"hello", 42, 3.14})));
}

// ---------------------------------------------------------------------------
// Phase 7: Integration tests
// ---------------------------------------------------------------------------

TEST_CASE(
  "integration: --output file with no constraints is no-op", "[validate]") {
  auto opt = make_option({"output", "o"}, ScalarType::File);
  auto v = from_option(opt);
  REQUIRE_NOTHROW(v.check("--output", json("/any/path")));
  REQUIRE_NOTHROW(v.check("--output", std::nullopt));
}

TEST_CASE("integration: required file with must_exist", "[validate]") {
  auto opt = make_option({"input"}, ScalarType::File);
  opt.required = true;
  opt.must_exist = true;
  auto v = from_option(opt);

  // absent → required error
  REQUIRE_THROWS_AS(v.check("--input", std::nullopt), Error);

  // non-existent file → must_exist error
  REQUIRE_THROWS_AS(v.check("--input", json(absent_path)), Error);

  // existing file → pass
  TempFile tf;
  REQUIRE_NOTHROW(v.check("--input", json(tf.path)));
}

TEST_CASE(
  "integration: list of files with must_exist validates each element",
  "[validate]") {
  TempFile tf;
  auto opt = make_option({"files"}, ListType{ScalarType::File, std::nullopt});
  opt.must_exist = true;
  auto v = from_option(opt);

  REQUIRE_NOTHROW(v.check("--files", json::array({tf.path})));
  REQUIRE_THROWS_AS(
    v.check("--files", json::array({tf.path, absent_path})), Error);
}

TEST_CASE(
  "integration: pair(string, file) with must_exist checks only file",
  "[validate]") {
  TempFile tf;
  auto opt = make_option(
    {"kv"}, PairType{ScalarType::String, ScalarType::File, std::nullopt});
  opt.must_exist = true;
  auto v = from_option(opt);

  // string element "nonexistent" is fine, file element is checked
  REQUIRE_NOTHROW(v.check("--kv", json::array({"nonexistent", tf.path})));
  REQUIRE_THROWS_AS(
    v.check("--kv", json::array({"nonexistent", absent_path})), Error);
}

TEST_CASE(
  "integration: idempotence — running validator twice produces same result",
  "[validate]") {
  TempFile tf;
  auto opt = make_option({"input"}, ScalarType::File);
  opt.required = true;
  opt.must_exist = true;
  auto v = from_option(opt);

  auto good = std::optional<json>(json(tf.path));
  REQUIRE_NOTHROW(v.check("--input", good));
  REQUIRE_NOTHROW(v.check("--input", good));

  REQUIRE_THROWS_AS(v.check("--input", std::nullopt), Error);
  REQUIRE_THROWS_AS(v.check("--input", std::nullopt), Error);
}
