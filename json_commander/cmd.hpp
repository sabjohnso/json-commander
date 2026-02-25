#pragma once

#include <json_commander/arg.hpp>

#include <optional>
#include <string>
#include <vector>

namespace json_commander::cmd {

  // -------------------------------------------------------------------------
  // Spec types
  // -------------------------------------------------------------------------

  struct CommandSpec {
    std::string name;
    model::DocString doc;
    std::vector<arg::ArgSpec> args;
    std::vector<CommandSpec> commands;
  };

  struct RootSpec {
    std::string name;
    model::DocString doc;
    std::vector<arg::ArgSpec> args;
    std::vector<CommandSpec> commands;
    std::optional<std::string> version;
    std::optional<model::Config> config;
  };

  // -------------------------------------------------------------------------
  // Forward declarations
  // -------------------------------------------------------------------------

  inline std::vector<CommandSpec>
  make_all(const std::vector<model::Command>& commands);

  // -------------------------------------------------------------------------
  // Factory functions
  // -------------------------------------------------------------------------

  inline CommandSpec
  make(const model::Command& cmd) {
    return {
      cmd.name,
      cmd.doc,
      cmd.args.has_value() ? arg::make_all(*cmd.args)
                           : std::vector<arg::ArgSpec>{},
      cmd.commands.has_value() ? make_all(*cmd.commands)
                               : std::vector<CommandSpec>{},
    };
  }

  inline std::vector<CommandSpec>
  make_all(const std::vector<model::Command>& commands) {
    std::vector<CommandSpec> specs;
    specs.reserve(commands.size());
    for (const auto& c : commands) {
      specs.push_back(make(c));
    }
    return specs;
  }

  inline RootSpec
  make(const model::Root& root) {
    return {
      root.name,
      root.doc,
      root.args.has_value() ? arg::make_all(*root.args)
                            : std::vector<arg::ArgSpec>{},
      root.commands.has_value() ? make_all(*root.commands)
                                : std::vector<CommandSpec>{},
      root.version,
      root.config,
    };
  }

} // namespace json_commander::cmd
