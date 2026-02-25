#pragma once

#include <json_commander/conv.hpp>
#include <json_commander/model.hpp>
#include <json_commander/validate.hpp>
#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace json_commander::arg {

  // -------------------------------------------------------------------------
  // Spec types
  // -------------------------------------------------------------------------

  struct EnvSpec {
    std::string var;
    std::optional<model::DocString> doc;
    bool
    operator==(const EnvSpec&) const = default;
  };

  struct FlagSpec {
    model::ArgNames names;
    std::string dest;
    bool repeated;
    std::optional<EnvSpec> env;
    std::optional<std::string> deprecated;
  };

  struct FlagGroupEntrySpec {
    model::ArgNames names;
    nlohmann::json value;
  };

  struct FlagGroupSpec {
    std::string dest;
    nlohmann::json default_value;
    std::vector<FlagGroupEntrySpec> entries;
    bool repeated;
  };

  struct OptionSpec {
    model::ArgNames names;
    std::string dest;
    conv::Converter converter;
    validate::Validator validator;
    std::optional<nlohmann::json> default_value;
    bool repeated;
    std::optional<EnvSpec> env;
  };

  struct PositionalSpec {
    std::string name;
    std::string dest;
    conv::Converter converter;
    validate::Validator validator;
    std::optional<nlohmann::json> default_value;
    bool repeated;
  };

  using ArgSpec =
    std::variant<FlagSpec, FlagGroupSpec, OptionSpec, PositionalSpec>;

  // -------------------------------------------------------------------------
  // Detail: resolution helpers
  // -------------------------------------------------------------------------

  namespace detail {

    inline std::string
    resolve_dest(const model::ArgNames& names) {
      for (const auto& name : names) {
        if (name.size() > 1) { return name; }
      }
      return names.front();
    }

    inline EnvSpec
    resolve_env(const model::EnvBinding& binding) {
      return std::visit(
        [](const auto& b) -> EnvSpec {
          using T = std::decay_t<decltype(b)>;
          if constexpr (std::is_same_v<T, std::string>) {
            return {b, std::nullopt};
          } else {
            return {b.var, b.doc};
          }
        },
        binding);
    }

    inline std::optional<EnvSpec>
    resolve_env_opt(const std::optional<model::EnvBinding>& binding) {
      if (!binding.has_value()) { return std::nullopt; }
      return resolve_env(*binding);
    }

  } // namespace detail

  // -------------------------------------------------------------------------
  // Factory functions
  // -------------------------------------------------------------------------

  inline FlagSpec
  make(const model::Flag& flag) {
    return {
      flag.names,
      flag.dest.value_or(detail::resolve_dest(flag.names)),
      flag.repeated.value_or(false),
      detail::resolve_env_opt(flag.env),
      flag.deprecated,
    };
  }

  inline FlagGroupSpec
  make(const model::FlagGroup& group) {
    std::vector<FlagGroupEntrySpec> entries;
    entries.reserve(group.flags.size());
    for (const auto& entry : group.flags) {
      entries.push_back({entry.names, entry.value});
    }
    return {
      group.dest,
      group.default_value,
      std::move(entries),
      group.repeated.value_or(false),
    };
  }

  inline OptionSpec
  make(const model::Option& opt) {
    return {
      opt.names,
      opt.dest.value_or(detail::resolve_dest(opt.names)),
      conv::make(opt.type, opt.choices),
      validate::from_option(opt),
      opt.default_value,
      opt.repeated.value_or(false),
      detail::resolve_env_opt(opt.env),
    };
  }

  inline PositionalSpec
  make(const model::Positional& pos) {
    return {
      pos.name,
      pos.name,
      conv::make(pos.type),
      validate::from_positional(pos),
      pos.default_value,
      pos.repeated.value_or(false),
    };
  }

  inline ArgSpec
  make(const model::Argument& argument) {
    return std::visit(
      [](const auto& a) -> ArgSpec { return make(a); }, argument);
  }

  inline std::vector<ArgSpec>
  make_all(const std::vector<model::Argument>& arguments) {
    std::vector<ArgSpec> specs;
    specs.reserve(arguments.size());
    for (const auto& a : arguments) {
      specs.push_back(make(a));
    }
    return specs;
  }

} // namespace json_commander::arg
