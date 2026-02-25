#pragma once

#include <json_commander/arg.hpp>
#include <json_commander/model.hpp>
#include <nlohmann/json.hpp>

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace json_commander::config_schema {

  namespace detail {

    inline nlohmann::json
    scalar_type_schema(
      model::ScalarType type,
      const std::optional<std::vector<std::string>>& choices = std::nullopt) {
      nlohmann::json schema;
      switch (type) {
        case model::ScalarType::Int:
          schema["type"] = "integer";
          break;
        case model::ScalarType::Float:
          schema["type"] = "number";
          break;
        case model::ScalarType::Bool:
          schema["type"] = "boolean";
          break;
        default:
          schema["type"] = "string";
          break;
      }
      if (type == model::ScalarType::Enum && choices.has_value()) {
        schema["enum"] = *choices;
      }
      return schema;
    }

    inline nlohmann::json
    type_spec_schema(
      const model::TypeSpec& spec,
      const std::optional<std::vector<std::string>>& choices = std::nullopt) {
      return std::visit(
        [&](const auto& s) -> nlohmann::json {
          using T = std::decay_t<decltype(s)>;
          if constexpr (std::is_same_v<T, model::ScalarType>) {
            return scalar_type_schema(s, choices);
          } else if constexpr (std::is_same_v<T, model::ListType>) {
            return {
              {"type", "array"}, {"items", scalar_type_schema(s.element)}};
          } else if constexpr (std::is_same_v<T, model::PairType>) {
            return {
              {"type", "array"},
              {"prefixItems",
               {scalar_type_schema(s.first), scalar_type_schema(s.second)}},
              {"items", false},
              {"minItems", 2},
              {"maxItems", 2}};
          } else if constexpr (std::is_same_v<T, model::TripleType>) {
            return {
              {"type", "array"},
              {"prefixItems",
               {scalar_type_schema(s.first),
                scalar_type_schema(s.second),
                scalar_type_schema(s.third)}},
              {"items", false},
              {"minItems", 3},
              {"maxItems", 3}};
          }
        },
        spec);
    }

    inline std::pair<std::string, nlohmann::json>
    arg_schema(const model::Argument& argument) {
      return std::visit(
        [](const auto& a) -> std::pair<std::string, nlohmann::json> {
          using T = std::decay_t<decltype(a)>;
          if constexpr (std::is_same_v<T, model::Flag>) {
            std::string dest =
              a.dest.value_or(arg::detail::resolve_dest(a.names));
            bool repeated = a.repeated.value_or(false);
            if (repeated) {
              return {dest, {{"type", "integer"}, {"minimum", 0}}};
            }
            return {dest, {{"type", "boolean"}}};
          } else if constexpr (std::is_same_v<T, model::FlagGroup>) {
            bool repeated = a.repeated.value_or(false);
            if (repeated) { return {a.dest, {{"type", "array"}}}; }
            return {a.dest, nlohmann::json::object()};
          } else if constexpr (std::is_same_v<T, model::Option>) {
            std::string dest =
              a.dest.value_or(arg::detail::resolve_dest(a.names));
            bool repeated = a.repeated.value_or(false);
            auto base = type_spec_schema(a.type, a.choices);
            if (repeated) {
              return {dest, {{"type", "array"}, {"items", base}}};
            }
            return {dest, base};
          } else if constexpr (std::is_same_v<T, model::Positional>) {
            bool repeated = a.repeated.value_or(false);
            auto base = type_spec_schema(a.type);
            if (repeated) {
              return {a.name, {{"type", "array"}, {"items", base}}};
            }
            return {a.name, base};
          }
        },
        argument);
    }

    inline bool
    is_required(const model::Argument& argument) {
      return std::visit(
        [](const auto& a) -> bool {
          using T = std::decay_t<decltype(a)>;
          if constexpr (
            std::is_same_v<T, model::Flag> ||
            std::is_same_v<T, model::FlagGroup>) {
            return true;
          } else if constexpr (std::is_same_v<T, model::Option>) {
            return a.required.value_or(false) || a.default_value.has_value();
          } else if constexpr (std::is_same_v<T, model::Positional>) {
            return a.required.value_or(false) || a.default_value.has_value();
          }
        },
        argument);
    }

    inline nlohmann::json
    generate(
      const std::vector<model::Argument>& args, const std::string& name) {
      nlohmann::json properties = nlohmann::json::object();
      nlohmann::json required = nlohmann::json::array();

      for (const auto& a : args) {
        auto [dest, schema] = arg_schema(a);
        properties[dest] = schema;
        if (is_required(a)) { required.push_back(dest); }
      }

      return {
        {"$schema", "https://json-schema.org/draft/2020-12/schema"},
        {"title", name + " configuration"},
        {"type", "object"},
        {"properties", properties},
        {"required", required},
        {"additionalProperties", false}};
    }

  } // namespace detail

  // -------------------------------------------------------------------------
  // Public API
  // -------------------------------------------------------------------------

  inline nlohmann::json
  to_config_schema(const model::Root& root) {
    auto args = root.args.value_or(std::vector<model::Argument>{});
    return detail::generate(args, root.name);
  }

  inline nlohmann::json
  to_config_schema(
    const model::Root& root, const std::vector<std::string>& command_path) {
    if (command_path.empty()) { return to_config_schema(root); }

    std::vector<model::Argument> all_args;
    if (root.args.has_value()) {
      all_args.insert(all_args.end(), root.args->begin(), root.args->end());
    }

    std::string display_name = root.name;
    const std::vector<model::Command>* commands =
      root.commands.has_value() ? &*root.commands : nullptr;

    for (const auto& segment : command_path) {
      if (!commands) {
        throw std::runtime_error("subcommand not found: " + segment);
      }
      bool found = false;
      for (const auto& cmd : *commands) {
        if (cmd.name == segment) {
          if (cmd.args.has_value()) {
            all_args.insert(all_args.end(), cmd.args->begin(), cmd.args->end());
          }
          commands = cmd.commands.has_value() ? &*cmd.commands : nullptr;
          display_name += "-" + segment;
          found = true;
          break;
        }
      }
      if (!found) {
        throw std::runtime_error("subcommand not found: " + segment);
      }
    }

    return detail::generate(all_args, display_name);
  }

} // namespace json_commander::config_schema
