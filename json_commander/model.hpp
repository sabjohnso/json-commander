#pragma once

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace json_commander::model {

  // ---------------------------------------------------------------------------
  // Leaf types
  // ---------------------------------------------------------------------------

  using DocString = std::vector<std::string>;

  using ArgNames = std::vector<std::string>;

  enum class ScalarType { String, Int, Float, Bool, Enum, File, Dir, Path };

  struct ListType {
    ScalarType element;
    std::optional<std::string> separator;
    bool
    operator==(const ListType&) const = default;
  };

  struct PairType {
    ScalarType first;
    ScalarType second;
    std::optional<std::string> separator;
    bool
    operator==(const PairType&) const = default;
  };

  struct TripleType {
    ScalarType first;
    ScalarType second;
    ScalarType third;
    std::optional<std::string> separator;
    bool
    operator==(const TripleType&) const = default;
  };

  using TypeSpec = std::variant<ScalarType, ListType, PairType, TripleType>;

  // ---------------------------------------------------------------------------
  // Environment & metadata types
  // ---------------------------------------------------------------------------

  struct EnvBindingObj {
    std::string var;
    std::optional<DocString> doc;
    bool
    operator==(const EnvBindingObj&) const = default;
  };

  using EnvBinding = std::variant<std::string, EnvBindingObj>;

  struct EnvInfo {
    std::string var;
    std::optional<DocString> doc;
    bool
    operator==(const EnvInfo&) const = default;
  };

  struct ExitInfo {
    int code;
    std::optional<int> max;
    DocString doc;
    bool
    operator==(const ExitInfo&) const = default;
  };

  // ---------------------------------------------------------------------------
  // Argument types
  // ---------------------------------------------------------------------------

} // namespace json_commander::model

#include <nlohmann/json.hpp>

namespace json_commander::model {

  struct Flag {
    ArgNames names;
    DocString doc;
    std::optional<std::string> dest;
    std::optional<EnvBinding> env;
    std::optional<bool> repeated;
    std::optional<std::string> deprecated;
    std::optional<std::string> docs;
    bool
    operator==(const Flag&) const = default;
  };

  struct FlagGroupEntry {
    ArgNames names;
    DocString doc;
    nlohmann::json value;
    bool
    operator==(const FlagGroupEntry&) const = default;
  };

  struct FlagGroup {
    std::string dest;
    DocString doc;
    nlohmann::json default_value;
    std::vector<FlagGroupEntry> flags;
    std::optional<bool> repeated;
    std::optional<std::string> docs;
    bool
    operator==(const FlagGroup&) const = default;
  };

  struct Option {
    ArgNames names;
    DocString doc;
    TypeSpec type;
    std::optional<std::string> docv;
    std::optional<nlohmann::json> default_value;
    std::optional<bool> required;
    std::optional<bool> repeated;
    std::optional<std::vector<std::string>> choices;
    std::optional<bool> must_exist;
    std::optional<std::string> dest;
    std::optional<EnvBinding> env;
    std::optional<std::string> docs;
    bool
    operator==(const Option&) const = default;
  };

  struct Positional {
    std::string name;
    DocString doc;
    TypeSpec type;
    std::optional<std::string> docv;
    std::optional<nlohmann::json> default_value;
    std::optional<bool> required;
    std::optional<bool> repeated;
    std::optional<bool> must_exist;
    std::optional<std::string> docs;
    bool
    operator==(const Positional&) const = default;
  };

  using Argument = std::variant<Flag, FlagGroup, Option, Positional>;

  // ---------------------------------------------------------------------------
  // Man page types
  // ---------------------------------------------------------------------------

  struct ParagraphBlock {
    DocString paragraph;
    bool
    operator==(const ParagraphBlock&) const = default;
  };

  struct PreBlock {
    DocString pre;
    bool
    operator==(const PreBlock&) const = default;
  };

  struct LabelTextBlock {
    std::string label;
    DocString text;
    bool
    operator==(const LabelTextBlock&) const = default;
  };

  struct NoBlankBlock {
    bool
    operator==(const NoBlankBlock&) const = default;
  };

  using ManBlock =
    std::variant<ParagraphBlock, PreBlock, LabelTextBlock, NoBlankBlock>;

  struct ManSection {
    std::string name;
    std::vector<ManBlock> blocks;
    std::optional<std::string> after;
    bool
    operator==(const ManSection&) const = default;
  };

  struct ManXref {
    std::string name;
    int section;
    bool
    operator==(const ManXref&) const = default;
  };

  struct Man {
    std::optional<int> section;
    std::optional<std::vector<ManSection>> sections;
    std::optional<std::vector<ManXref>> xrefs;
    bool
    operator==(const Man&) const = default;
  };

  // ---------------------------------------------------------------------------
  // Config types
  // ---------------------------------------------------------------------------

  struct ConfigPaths {
    std::optional<std::string> system;
    std::optional<std::string> user;
    std::optional<std::string> local;
    bool
    operator==(const ConfigPaths&) const = default;
  };

  struct Config {
    std::string format;
    std::optional<ConfigPaths> paths;
    bool
    operator==(const Config&) const = default;
  };

  // ---------------------------------------------------------------------------
  // Command types
  // ---------------------------------------------------------------------------

  struct Command {
    std::string name;
    DocString doc;
    std::optional<std::vector<Argument>> args;
    std::optional<std::vector<Command>> commands;
    std::optional<Man> man;
    std::optional<std::vector<EnvInfo>> envs;
    std::optional<std::vector<ExitInfo>> exits;
    bool
    operator==(const Command&) const = default;
  };

  struct Root {
    std::string name;
    DocString doc;
    std::optional<std::vector<Argument>> args;
    std::optional<std::vector<Command>> commands;
    std::optional<Man> man;
    std::optional<std::vector<EnvInfo>> envs;
    std::optional<std::vector<ExitInfo>> exits;
    std::optional<std::string> version;
    std::optional<Config> config;
    bool
    operator==(const Root&) const = default;
  };

} // namespace json_commander::model
