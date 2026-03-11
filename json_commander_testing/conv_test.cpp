#include <catch2/catch_test_macros.hpp>
#include <json_commander/conv.hpp>

using namespace json_commander::conv;
using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Phase 1: Converter type and string converter
// ---------------------------------------------------------------------------

TEST_CASE("Error carries its message", "[conv]") {
  Error e("something went wrong");
  REQUIRE(std::string(e.what()) == "something went wrong");
}

TEST_CASE("string_conv parses string to JSON string", "[conv]") {
  REQUIRE(string_conv().parse("hello") == json("hello"));
}

TEST_CASE("string_conv parses empty string", "[conv]") {
  REQUIRE(string_conv().parse("") == json(""));
}

TEST_CASE("string_conv formats JSON string", "[conv]") {
  REQUIRE(string_conv().format(json("hello")) == "hello");
}

TEST_CASE("string_conv has docv STRING", "[conv]") {
  REQUIRE(string_conv().docv == "STRING");
}

// ---------------------------------------------------------------------------
// Phase 2: Numeric converters (int, float)
// ---------------------------------------------------------------------------

TEST_CASE("int_conv parses positive integer", "[conv]") {
  REQUIRE(int_conv().parse("42") == json(42));
}

TEST_CASE("int_conv parses negative integer", "[conv]") {
  REQUIRE(int_conv().parse("-1") == json(-1));
}

TEST_CASE("int_conv parses zero", "[conv]") {
  REQUIRE(int_conv().parse("0") == json(0));
}

TEST_CASE("int_conv rejects non-numeric string", "[conv]") {
  REQUIRE_THROWS_AS(int_conv().parse("abc"), Error);
}

TEST_CASE("int_conv rejects empty string", "[conv]") {
  REQUIRE_THROWS_AS(int_conv().parse(""), Error);
}

TEST_CASE("int_conv rejects float string", "[conv]") {
  REQUIRE_THROWS_AS(int_conv().parse("1.5"), Error);
}

TEST_CASE("int_conv formats integer", "[conv]") {
  REQUIRE(int_conv().format(json(42)) == "42");
}

TEST_CASE("int_conv has docv INT", "[conv]") {
  REQUIRE(int_conv().docv == "INT");
}

TEST_CASE("float_conv parses decimal", "[conv]") {
  REQUIRE(float_conv().parse("3.14") == json(3.14));
}

TEST_CASE("float_conv parses negative decimal", "[conv]") {
  REQUIRE(float_conv().parse("-0.5") == json(-0.5));
}

TEST_CASE("float_conv parses integer as float", "[conv]") {
  auto result = float_conv().parse("1");
  REQUIRE(result.is_number());
  REQUIRE(result.get<double>() == 1.0);
}

TEST_CASE("float_conv rejects non-numeric string", "[conv]") {
  REQUIRE_THROWS_AS(float_conv().parse("abc"), Error);
}

TEST_CASE("float_conv formats float", "[conv]") {
  REQUIRE(float_conv().format(json(3.14)) == "3.14");
}

TEST_CASE("float_conv has docv FLOAT", "[conv]") {
  REQUIRE(float_conv().docv == "FLOAT");
}

// ---------------------------------------------------------------------------
// Phase 3: Bool converter
// ---------------------------------------------------------------------------

TEST_CASE("bool_conv parses true", "[conv]") {
  REQUIRE(bool_conv().parse("true") == json(true));
}

TEST_CASE("bool_conv parses false", "[conv]") {
  REQUIRE(bool_conv().parse("false") == json(false));
}

TEST_CASE("bool_conv parses TRUE (case-insensitive)", "[conv]") {
  REQUIRE(bool_conv().parse("TRUE") == json(true));
}

TEST_CASE("bool_conv parses False (case-insensitive)", "[conv]") {
  REQUIRE(bool_conv().parse("False") == json(false));
}

TEST_CASE("bool_conv rejects yes", "[conv]") {
  REQUIRE_THROWS_AS(bool_conv().parse("yes"), Error);
}

TEST_CASE("bool_conv rejects 1", "[conv]") {
  REQUIRE_THROWS_AS(bool_conv().parse("1"), Error);
}

TEST_CASE("bool_conv rejects empty string", "[conv]") {
  REQUIRE_THROWS_AS(bool_conv().parse(""), Error);
}

TEST_CASE("bool_conv formats true", "[conv]") {
  REQUIRE(bool_conv().format(json(true)) == "true");
}

TEST_CASE("bool_conv formats false", "[conv]") {
  REQUIRE(bool_conv().format(json(false)) == "false");
}

TEST_CASE("bool_conv has docv BOOL", "[conv]") {
  REQUIRE(bool_conv().docv == "BOOL");
}

// ---------------------------------------------------------------------------
// Phase 4: Enum converter
// ---------------------------------------------------------------------------

TEST_CASE("enum_conv parses valid choice", "[conv]") {
  auto c = enum_conv({"json", "yaml", "toml"});
  REQUIRE(c.parse("json") == json("json"));
}

TEST_CASE("enum_conv rejects invalid choice", "[conv]") {
  auto c = enum_conv({"json", "yaml", "toml"});
  REQUIRE_THROWS_AS(c.parse("xml"), Error);
}

TEST_CASE(
  "enum_conv error mentions invalid value and valid choices", "[conv]") {
  auto c = enum_conv({"json", "yaml", "toml"});
  try {
    c.parse("xml");
    FAIL("expected Error to be thrown");
  } catch (const Error& e) {
    std::string msg = e.what();
    CHECK(msg.find("xml") != std::string::npos);
    CHECK(msg.find("json") != std::string::npos);
    CHECK(msg.find("yaml") != std::string::npos);
    CHECK(msg.find("toml") != std::string::npos);
  }
}

TEST_CASE("enum_conv formats valid choice", "[conv]") {
  auto c = enum_conv({"json", "yaml", "toml"});
  REQUIRE(c.format(json("yaml")) == "yaml");
}

TEST_CASE("enum_conv has docv ENUM", "[conv]") {
  REQUIRE(enum_conv({"json", "yaml"}).docv == "ENUM");
}

// ---------------------------------------------------------------------------
// Phase 5: File system converters (file, dir, path)
// ---------------------------------------------------------------------------

TEST_CASE("file_conv parses path as pass-through", "[conv]") {
  REQUIRE(file_conv().parse("/tmp/f") == json("/tmp/f"));
}

TEST_CASE("file_conv formats path", "[conv]") {
  REQUIRE(file_conv().format(json("/tmp/f")) == "/tmp/f");
}

TEST_CASE("file_conv has docv FILE", "[conv]") {
  REQUIRE(file_conv().docv == "FILE");
}

TEST_CASE("dir_conv has docv DIR", "[conv]") {
  REQUIRE(dir_conv().docv == "DIR");
}

TEST_CASE("path_conv has docv PATH", "[conv]") {
  REQUIRE(path_conv().docv == "PATH");
}

// ---------------------------------------------------------------------------
// Phase 6: Compound converters (list, pair, triple)
// ---------------------------------------------------------------------------

TEST_CASE("list_conv parses comma-separated integers", "[conv]") {
  auto c = list_conv(int_conv(), ",");
  REQUIRE(c.parse("1,2,3") == json({1, 2, 3}));
}

TEST_CASE("list_conv parses comma-separated strings", "[conv]") {
  auto c = list_conv(string_conv(), ",");
  REQUIRE(c.parse("a,b,c") == json({"a", "b", "c"}));
}

TEST_CASE("list_conv parses single element", "[conv]") {
  auto c = list_conv(int_conv(), ",");
  REQUIRE(c.parse("1") == json({1}));
}

TEST_CASE("list_conv parses empty string to empty array", "[conv]") {
  auto c = list_conv(int_conv(), ",");
  REQUIRE(c.parse("") == json::array());
}

TEST_CASE("list_conv propagates element parse error", "[conv]") {
  auto c = list_conv(int_conv(), ",");
  REQUIRE_THROWS_AS(c.parse("1,abc"), Error);
}

TEST_CASE("list_conv enforces max element count", "[conv]") {
  auto c = list_conv(int_conv(), ",", 3);
  REQUIRE_NOTHROW(c.parse("1,2,3"));
  REQUIRE_THROWS_AS(c.parse("1,2,3,4"), Error);
}

TEST_CASE("list_conv uses default max element limit", "[conv]") {
  auto c = list_conv(int_conv(), ",");
  // Default limit should be large enough for normal use
  std::string input;
  for (int i = 0; i < 100; ++i) {
    if (i > 0) input += ",";
    input += std::to_string(i);
  }
  REQUIRE_NOTHROW(c.parse(input));
}

TEST_CASE("list_conv formats array", "[conv]") {
  auto c = list_conv(int_conv(), ",");
  REQUIRE(c.format(json({1, 2, 3})) == "1,2,3");
}

TEST_CASE("list_conv docv shows element type with separator", "[conv]") {
  auto c = list_conv(int_conv(), ",");
  REQUIRE(c.docv == "INT,...");
}

TEST_CASE("pair_conv parses separated pair", "[conv]") {
  auto c = pair_conv(string_conv(), int_conv(), "=");
  REQUIRE(c.parse("key=42") == json({"key", 42}));
}

TEST_CASE("pair_conv rejects missing separator", "[conv]") {
  auto c = pair_conv(string_conv(), int_conv(), "=");
  REQUIRE_THROWS_AS(c.parse("no_sep"), Error);
}

TEST_CASE("pair_conv formats pair", "[conv]") {
  auto c = pair_conv(string_conv(), int_conv(), "=");
  REQUIRE(c.format(json({"key", 42})) == "key=42");
}

TEST_CASE("pair_conv docv shows both types", "[conv]") {
  auto c = pair_conv(string_conv(), int_conv(), "=");
  REQUIRE(c.docv == "STRING=INT");
}

TEST_CASE("triple_conv parses separated triple", "[conv]") {
  auto c = triple_conv(int_conv(), int_conv(), int_conv(), ",");
  REQUIRE(c.parse("255,128,0") == json({255, 128, 0}));
}

TEST_CASE("triple_conv rejects insufficient parts", "[conv]") {
  auto c = triple_conv(int_conv(), int_conv(), int_conv(), ",");
  REQUIRE_THROWS_AS(c.parse("1,2"), Error);
}

TEST_CASE("triple_conv formats triple", "[conv]") {
  auto c = triple_conv(int_conv(), int_conv(), int_conv(), ",");
  REQUIRE(c.format(json({255, 128, 0})) == "255,128,0");
}

TEST_CASE("triple_conv docv shows all three types", "[conv]") {
  auto c = triple_conv(int_conv(), int_conv(), int_conv(), ",");
  REQUIRE(c.docv == "INT,INT,INT");
}

// ---------------------------------------------------------------------------
// Phase 7: Factory functions (make from model types)
// ---------------------------------------------------------------------------

TEST_CASE("make(ScalarType::String) has docv STRING", "[conv]") {
  REQUIRE(make(json_commander::model::ScalarType::String).docv == "STRING");
}

TEST_CASE("make(ScalarType::Int) parses integer", "[conv]") {
  REQUIRE(make(json_commander::model::ScalarType::Int).parse("42") == json(42));
}

TEST_CASE(
  "make(ScalarType::Enum) without choices parses any string", "[conv]") {
  auto c = make(json_commander::model::ScalarType::Enum);
  REQUIRE_NOTHROW(c.parse("anything"));
  REQUIRE(c.parse("anything") == json("anything"));
}

TEST_CASE("make(TypeSpec) with ListType parses list", "[conv]") {
  using namespace json_commander::model;
  TypeSpec spec = ListType{ScalarType::String, ","};
  auto c = make(spec);
  REQUIRE(c.parse("a,b") == json({"a", "b"}));
}

TEST_CASE("make(TypeSpec) with PairType parses pair", "[conv]") {
  using namespace json_commander::model;
  TypeSpec spec = PairType{ScalarType::String, ScalarType::Int, "="};
  auto c = make(spec);
  REQUIRE(c.parse("k=1") == json({"k", 1}));
}

TEST_CASE("make(TypeSpec) with TripleType parses triple", "[conv]") {
  using namespace json_commander::model;
  TypeSpec spec =
    TripleType{ScalarType::Int, ScalarType::Int, ScalarType::Int, ","};
  auto c = make(spec);
  REQUIRE(c.parse("1,2,3") == json({1, 2, 3}));
}

TEST_CASE(
  "make(TypeSpec) with enum and choices validates against choices", "[conv]") {
  using namespace json_commander::model;
  TypeSpec spec = ScalarType::Enum;
  auto c = make(spec, std::vector<std::string>{"json", "yaml"});
  REQUIRE(c.parse("json") == json("json"));
  REQUIRE_THROWS_AS(c.parse("xml"), Error);
}

TEST_CASE(
  "make(TypeSpec) with list uses default separator when omitted", "[conv]") {
  using namespace json_commander::model;
  TypeSpec spec = ListType{ScalarType::Int, std::nullopt};
  auto c = make(spec);
  REQUIRE(c.parse("1,2,3") == json({1, 2, 3}));
}

// ---------------------------------------------------------------------------
// Phase 7: Round-trip property tests
// ---------------------------------------------------------------------------

TEST_CASE("round-trip: string", "[conv]") {
  auto c = string_conv();
  REQUIRE(c.format(c.parse("hello")) == "hello");
}

TEST_CASE("round-trip: int", "[conv]") {
  auto c = int_conv();
  REQUIRE(c.format(c.parse("42")) == "42");
  REQUIRE(c.format(c.parse("-1")) == "-1");
  REQUIRE(c.format(c.parse("0")) == "0");
}

TEST_CASE("round-trip: float", "[conv]") {
  auto c = float_conv();
  REQUIRE(c.format(c.parse("3.14")) == "3.14");
  REQUIRE(c.format(c.parse("-0.5")) == "-0.5");
}

TEST_CASE("round-trip: bool", "[conv]") {
  auto c = bool_conv();
  REQUIRE(c.format(c.parse("true")) == "true");
  REQUIRE(c.format(c.parse("false")) == "false");
}

TEST_CASE("round-trip: enum", "[conv]") {
  auto c = enum_conv({"json", "yaml"});
  REQUIRE(c.format(c.parse("json")) == "json");
  REQUIRE(c.format(c.parse("yaml")) == "yaml");
}

TEST_CASE("round-trip: list", "[conv]") {
  auto c = list_conv(int_conv(), ",");
  REQUIRE(c.format(c.parse("1,2,3")) == "1,2,3");
}

TEST_CASE("round-trip: pair", "[conv]") {
  auto c = pair_conv(string_conv(), int_conv(), "=");
  REQUIRE(c.format(c.parse("key=42")) == "key=42");
}

TEST_CASE("round-trip: triple", "[conv]") {
  auto c = triple_conv(int_conv(), int_conv(), int_conv(), ",");
  REQUIRE(c.format(c.parse("255,128,0")) == "255,128,0");
}
