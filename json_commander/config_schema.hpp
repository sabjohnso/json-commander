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
              {"items",
               {scalar_type_schema(s.first), scalar_type_schema(s.second)}},
              {"additionalItems", false},
              {"minItems", 2},
              {"maxItems", 2}};
          } else if constexpr (std::is_same_v<T, model::TripleType>) {
            return {
              {"type", "array"},
              {"items",
               {scalar_type_schema(s.first),
                scalar_type_schema(s.second),
                scalar_type_schema(s.third)}},
              {"additionalItems", false},
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
        {"$schema", "http://json-schema.org/draft-07/schema#"},
        {"title", name + " configuration"},
        {"type", "object"},
        {"properties", properties},
        {"required", required},
        {"additionalProperties", false}};
    }

    inline void
    collect_arg_props(
      const std::vector<model::Argument>& args,
      nlohmann::json& properties,
      nlohmann::json& required) {
      for (const auto& a : args) {
        auto [dest, schema] = arg_schema(a);
        properties[dest] = schema;
        if (is_required(a)) { required.push_back(dest); }
      }
    }

    // Generates a command-level schema, registering subcommand definitions
    // in the shared top-level `defs` map under qualified names.
    // Returns either a simple object schema (no subcommands) or a
    // discriminated union (oneOf) referencing definitions entries.
    inline nlohmann::json
    generate_command_schema(
      const std::vector<model::Argument>& args,
      const std::vector<model::Command>& commands,
      nlohmann::json& defs,
      const std::string& prefix) {
      nlohmann::json arg_properties = nlohmann::json::object();
      nlohmann::json arg_required = nlohmann::json::array();
      collect_arg_props(args, arg_properties, arg_required);

      if (commands.empty()) {
        return {
          {"type", "object"},
          {"properties", arg_properties},
          {"required", arg_required},
          {"additionalProperties", false}};
      }

      for (const auto& cmd : commands) {
        auto cmd_args = cmd.args.value_or(std::vector<model::Argument>{});
        auto cmd_commands =
          cmd.commands.value_or(std::vector<model::Command>{});
        std::string def_name =
          prefix.empty() ? cmd.name : prefix + "." + cmd.name;
        defs[def_name] =
          generate_command_schema(cmd_args, cmd_commands, defs, def_name);
      }

      nlohmann::json variants = nlohmann::json::array();
      for (const auto& cmd : commands) {
        nlohmann::json properties = arg_properties;
        nlohmann::json required = arg_required;
        std::string def_name =
          prefix.empty() ? cmd.name : prefix + "." + cmd.name;
        properties["command"] = {{"const", cmd.name}};
        required.push_back("command");
        properties[cmd.name] = {{"$ref", "#/definitions/" + def_name}};
        required.push_back(cmd.name);
        variants.push_back(
          {{"properties", properties},
           {"required", required},
           {"additionalProperties", false}});
      }

      return {{"type", "object"}, {"oneOf", variants}};
    }

  } // namespace detail

  // -------------------------------------------------------------------------
  // Public API
  // -------------------------------------------------------------------------

  inline nlohmann::json
  to_config_schema(const model::Root& root) {
    auto args = root.args.value_or(std::vector<model::Argument>{});
    auto commands = root.commands.value_or(std::vector<model::Command>{});
    if (commands.empty()) { return detail::generate(args, root.name); }

    nlohmann::json defs = nlohmann::json::object();
    auto schema = detail::generate_command_schema(args, commands, defs, "");
    schema["$schema"] = "http://json-schema.org/draft-07/schema#";
    schema["title"] = root.name + " configuration";
    if (!defs.empty()) { schema["definitions"] = defs; }
    return schema;
  }

  inline nlohmann::json
  to_config_schema(
    const model::Root& root, const std::vector<std::string>& command_path) {
    if (command_path.empty()) { return to_config_schema(root); }

    std::vector<model::Argument> root_args;
    if (root.args.has_value()) { root_args = *root.args; }

    std::string display_name = root.name;
    const std::vector<model::Command>* commands_ptr =
      root.commands.has_value() ? &*root.commands : nullptr;

    struct PathEntry {
      const model::Command* cmd;
      std::string def_name;
    };
    std::vector<PathEntry> path_entries;
    std::string prefix;

    for (const auto& segment : command_path) {
      if (!commands_ptr) {
        throw std::runtime_error("subcommand not found: " + segment);
      }
      bool found = false;
      for (const auto& cmd : *commands_ptr) {
        if (cmd.name == segment) {
          prefix = prefix.empty() ? segment : prefix + "." + segment;
          path_entries.push_back({&cmd, prefix});
          commands_ptr = cmd.commands.has_value() ? &*cmd.commands : nullptr;
          display_name += "-" + segment;
          found = true;
          break;
        }
      }
      if (!found) {
        throw std::runtime_error("subcommand not found: " + segment);
      }
    }

    nlohmann::json defs = nlohmann::json::object();

    // Generate the deepest command's schema (handles its own subcommands)
    auto& deepest_entry = path_entries.back();
    auto deepest_args =
      deepest_entry.cmd->args.value_or(std::vector<model::Argument>{});
    auto deepest_commands =
      deepest_entry.cmd->commands.value_or(std::vector<model::Command>{});
    defs[deepest_entry.def_name] = detail::generate_command_schema(
      deepest_args, deepest_commands, defs, deepest_entry.def_name);

    // Wrap from second-to-last back to first command in path
    for (int i = static_cast<int>(path_entries.size()) - 2; i >= 0; --i) {
      auto& entry = path_entries[i];
      auto& next_entry = path_entries[i + 1];
      auto cmd_args = entry.cmd->args.value_or(std::vector<model::Argument>{});

      nlohmann::json properties = nlohmann::json::object();
      nlohmann::json required = nlohmann::json::array();
      detail::collect_arg_props(cmd_args, properties, required);
      properties["command"] = {{"const", next_entry.cmd->name}};
      required.push_back("command");
      properties[next_entry.cmd->name] = {
        {"$ref", "#/definitions/" + next_entry.def_name}};
      required.push_back(std::string(next_entry.cmd->name));

      defs[entry.def_name] = {
        {"type", "object"},
        {"properties", properties},
        {"required", required},
        {"additionalProperties", false}};
    }

    // Root-level schema
    nlohmann::json properties = nlohmann::json::object();
    nlohmann::json required = nlohmann::json::array();
    detail::collect_arg_props(root_args, properties, required);
    auto& first = path_entries[0];
    properties["command"] = {{"const", first.cmd->name}};
    required.push_back("command");
    properties[first.cmd->name] = {{"$ref", "#/definitions/" + first.def_name}};
    required.push_back(std::string(first.cmd->name));

    nlohmann::json schema = {
      {"$schema", "http://json-schema.org/draft-07/schema#"},
      {"title", display_name + " configuration"},
      {"type", "object"},
      {"properties", properties},
      {"required", required},
      {"additionalProperties", false}};
    if (!defs.empty()) { schema["definitions"] = defs; }
    return schema;
  }

} // namespace json_commander::config_schema
