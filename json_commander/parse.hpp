#pragma once

#include <json_commander/cmd.hpp>
#include <nlohmann/json.hpp>

#include <cstdlib>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace json_commander::parse {

  // -------------------------------------------------------------------------
  // Error type
  // -------------------------------------------------------------------------

  class Error : public std::runtime_error {
  public:
    explicit Error(const std::string& message)
        : std::runtime_error(message) {}
  };

  // -------------------------------------------------------------------------
  // Result types
  // -------------------------------------------------------------------------

  struct ParseOk {
    nlohmann::json config;
    std::vector<std::string> command_path;
  };

  struct HelpRequest {
    std::vector<std::string> command_path;
  };

  struct VersionRequest {};

  struct ManpageRequest {
    std::vector<std::string> command_path;
  };

  using ParseResult =
    std::variant<ParseOk, HelpRequest, VersionRequest, ManpageRequest>;

  // -------------------------------------------------------------------------
  // Environment lookup
  // -------------------------------------------------------------------------

  using EnvLookup =
    std::function<std::optional<std::string>(const std::string&)>;

  inline EnvLookup
  default_env_lookup() {
    return [](const std::string& var) -> std::optional<std::string> {
      const char* val = std::getenv(var.c_str());
      if (val == nullptr) { return std::nullopt; }
      return std::string(val);
    };
  }

  inline EnvLookup
  no_env() {
    return [](const std::string&) -> std::optional<std::string> {
      return std::nullopt;
    };
  }

  // -------------------------------------------------------------------------
  // Detail: name index
  // -------------------------------------------------------------------------

  namespace detail {

    enum class MatchKind { Flag, Option, FlagGroup };

    struct MatchResult {
      std::size_t arg_index;
      MatchKind kind;
      std::size_t entry_index;
    };

    class NameIndex {
      std::unordered_map<std::string, MatchResult> entries_;

    public:
      void
      insert(const std::string& cli_name, MatchResult result) {
        entries_.emplace(cli_name, result);
      }

      std::optional<MatchResult>
      lookup(const std::string& cli_name) const {
        auto it = entries_.find(cli_name);
        if (it == entries_.end()) { return std::nullopt; }
        return it->second;
      }
    };

    inline std::string
    cli_name(const std::string& name) {
      if (name.size() == 1) { return "-" + name; }
      return "--" + name;
    }

    inline NameIndex
    build_index(const std::vector<arg::ArgSpec>& args) {
      NameIndex index;
      for (std::size_t i = 0; i < args.size(); ++i) {
        std::visit(
          [&](const auto& spec) {
            using T = std::decay_t<decltype(spec)>;
            if constexpr (std::is_same_v<T, arg::FlagSpec>) {
              for (const auto& name : spec.names) {
                index.insert(cli_name(name), {i, MatchKind::Flag, 0});
              }
            } else if constexpr (std::is_same_v<T, arg::OptionSpec>) {
              for (const auto& name : spec.names) {
                index.insert(cli_name(name), {i, MatchKind::Option, 0});
              }
            } else if constexpr (std::is_same_v<T, arg::FlagGroupSpec>) {
              for (std::size_t e = 0; e < spec.entries.size(); ++e) {
                for (const auto& name : spec.entries[e].names) {
                  index.insert(cli_name(name), {i, MatchKind::FlagGroup, e});
                }
              }
            }
            // PositionalSpec: not indexed
          },
          args[i]);
      }
      return index;
    }

    // -----------------------------------------------------------------------
    // Token classification
    // -----------------------------------------------------------------------

    enum class TokenKind { LongOption, ShortGroup, DoubleDash, Positional };

    inline TokenKind
    classify_token(const std::string& token) {
      if (token == "--") { return TokenKind::DoubleDash; }
      if (token.size() >= 3 && token[0] == '-' && token[1] == '-') {
        return TokenKind::LongOption;
      }
      if (token.size() >= 2 && token[0] == '-' && token[1] != '-') {
        return TokenKind::ShortGroup;
      }
      return TokenKind::Positional;
    }

    struct SplitResult {
      std::string name;
      std::optional<std::string> value;
    };

    inline SplitResult
    split_long_option(const std::string& token) {
      auto stripped = token.substr(2); // remove leading --
      auto eq = stripped.find('=');
      if (eq == std::string::npos) { return {stripped, std::nullopt}; }
      return {stripped.substr(0, eq), stripped.substr(eq + 1)};
    }

    // -----------------------------------------------------------------------
    // Level parsing state
    // -----------------------------------------------------------------------

    struct LevelOk {
      nlohmann::json config;
      std::vector<std::string> command_path;
      std::size_t next_pos;
    };

    using LevelResult =
      std::variant<LevelOk, HelpRequest, VersionRequest, ManpageRequest>;

    inline LevelResult
    parse_level(
      const std::vector<arg::ArgSpec>& args,
      const std::vector<cmd::CommandSpec>& commands,
      const std::vector<std::string>& tokens,
      std::size_t start,
      bool is_root,
      const std::optional<std::string>& version) {
      auto index = build_index(args);
      nlohmann::json config = nlohmann::json::object();
      std::vector<std::string> command_path;

      // Track flag counts for repeated flags
      std::vector<int> flag_counts(args.size(), 0);

      // Track positional argument cursor
      std::size_t pos_cursor = 0;
      std::vector<std::size_t> positional_indices;
      for (std::size_t i = 0; i < args.size(); ++i) {
        if (std::holds_alternative<arg::PositionalSpec>(args[i])) {
          positional_indices.push_back(i);
        }
      }

      bool options_terminated = false;
      std::size_t i = start;

      while (i < tokens.size()) {
        const auto& token = tokens[i];

        if (!options_terminated) {
          auto kind = classify_token(token);

          if (kind == TokenKind::DoubleDash) {
            options_terminated = true;
            ++i;
            continue;
          }

          // Check for --help / -h
          if (token == "--help" || token == "-h") {
            return HelpRequest{command_path};
          }

          // Check for --help-man
          if (token == "--help-man") { return ManpageRequest{command_path}; }

          // Check for --version at root
          if (is_root && token == "--version") {
            if (!version.has_value()) {
              throw Error("--version: no version defined");
            }
            return VersionRequest{};
          }

          if (kind == TokenKind::LongOption) {
            auto [name, eq_value] = split_long_option(token);
            auto match = index.lookup("--" + name);
            if (!match.has_value()) {
              throw Error("unknown option: --" + name);
            }

            if (match->kind == MatchKind::Flag) {
              auto& flag = std::get<arg::FlagSpec>(args[match->arg_index]);
              flag_counts[match->arg_index]++;
              if (flag.repeated) {
                config[flag.dest] = flag_counts[match->arg_index];
              } else {
                config[flag.dest] = true;
              }
              ++i;
              continue;
            }

            if (match->kind == MatchKind::Option) {
              auto& opt = std::get<arg::OptionSpec>(args[match->arg_index]);
              std::string raw_value;
              if (eq_value.has_value()) {
                raw_value = *eq_value;
              } else {
                ++i;
                if (i >= tokens.size()) {
                  throw Error("option --" + name + " requires a value");
                }
                raw_value = tokens[i];
              }
              nlohmann::json converted;
              try {
                converted = opt.converter.parse(raw_value);
              } catch (const conv::Error& e) {
                throw Error("option --" + name + ": " + e.what());
              }
              if (opt.repeated) {
                if (!config.contains(opt.dest)) {
                  config[opt.dest] = nlohmann::json::array();
                }
                config[opt.dest].push_back(converted);
              } else {
                config[opt.dest] = converted;
              }
              ++i;
              continue;
            }

            if (match->kind == MatchKind::FlagGroup) {
              auto& group =
                std::get<arg::FlagGroupSpec>(args[match->arg_index]);
              auto& entry = group.entries[match->entry_index];
              flag_counts[match->arg_index]++;
              if (group.repeated) {
                if (!config.contains(group.dest)) {
                  config[group.dest] = nlohmann::json::array();
                }
                config[group.dest].push_back(entry.value);
              } else {
                config[group.dest] = entry.value;
              }
              ++i;
              continue;
            }
          }

          if (kind == TokenKind::ShortGroup) {
            // Process each character in the short group
            for (std::size_t c = 1; c < token.size(); ++c) {
              std::string short_name = std::string("-") + token[c];
              auto match = index.lookup(short_name);
              if (!match.has_value()) {
                throw Error("unknown option: " + short_name);
              }

              if (match->kind == MatchKind::Flag) {
                auto& flag = std::get<arg::FlagSpec>(args[match->arg_index]);
                flag_counts[match->arg_index]++;
                if (flag.repeated) {
                  config[flag.dest] = flag_counts[match->arg_index];
                } else {
                  config[flag.dest] = true;
                }
                continue;
              }

              if (match->kind == MatchKind::Option) {
                auto& opt = std::get<arg::OptionSpec>(args[match->arg_index]);
                // If not the last character in the group, error
                if (c != token.size() - 1) {
                  throw Error(
                    "option " + short_name +
                    " requires a value and must be last in a short group");
                }
                ++i;
                if (i >= tokens.size()) {
                  throw Error("option " + short_name + " requires a value");
                }
                std::string raw_value = tokens[i];
                nlohmann::json converted;
                try {
                  converted = opt.converter.parse(raw_value);
                } catch (const conv::Error& e) {
                  throw Error("option " + short_name + ": " + e.what());
                }
                if (opt.repeated) {
                  if (!config.contains(opt.dest)) {
                    config[opt.dest] = nlohmann::json::array();
                  }
                  config[opt.dest].push_back(converted);
                } else {
                  config[opt.dest] = converted;
                }
                continue;
              }

              if (match->kind == MatchKind::FlagGroup) {
                auto& group =
                  std::get<arg::FlagGroupSpec>(args[match->arg_index]);
                auto& entry = group.entries[match->entry_index];
                flag_counts[match->arg_index]++;
                if (group.repeated) {
                  if (!config.contains(group.dest)) {
                    config[group.dest] = nlohmann::json::array();
                  }
                  config[group.dest].push_back(entry.value);
                } else {
                  config[group.dest] = entry.value;
                }
                continue;
              }
            }
            ++i;
            continue;
          }
        }

        // Positional or subcommand
        // Check for subcommand match (only when options not terminated)
        if (!options_terminated) {
          bool found_command = false;
          for (const auto& cmd : commands) {
            if (cmd.name == tokens[i]) {
              command_path.push_back(cmd.name);
              auto sub_result = parse_level(
                cmd.args, cmd.commands, tokens, i + 1, false, std::nullopt);

              // Propagate help/version from sub-level
              if (auto* help = std::get_if<HelpRequest>(&sub_result)) {
                // Prepend our accumulated command_path
                std::vector<std::string> full_path = command_path;
                for (auto& p : help->command_path) {
                  full_path.push_back(std::move(p));
                }
                return HelpRequest{std::move(full_path)};
              }
              if (auto* manpage = std::get_if<ManpageRequest>(&sub_result)) {
                std::vector<std::string> full_path = command_path;
                for (auto& p : manpage->command_path) {
                  full_path.push_back(std::move(p));
                }
                return ManpageRequest{std::move(full_path)};
              }
              if (std::holds_alternative<VersionRequest>(sub_result)) {
                return VersionRequest{};
              }

              auto& sub_ok = std::get<LevelOk>(sub_result);
              for (auto& [key, val] : sub_ok.config.items()) {
                config[key] = val;
              }
              for (auto& p : sub_ok.command_path) {
                command_path.push_back(std::move(p));
              }
              i = sub_ok.next_pos;
              found_command = true;
              break;
            }
          }
          if (found_command) { continue; }
        }

        // Treat as positional
        if (pos_cursor >= positional_indices.size()) {
          throw Error("unexpected positional argument: " + tokens[i]);
        }
        auto pos_idx = positional_indices[pos_cursor];
        auto& pos = std::get<arg::PositionalSpec>(args[pos_idx]);
        nlohmann::json converted;
        try {
          converted = pos.converter.parse(tokens[i]);
        } catch (const conv::Error& e) {
          throw Error("positional " + pos.name + ": " + e.what());
        }
        if (pos.repeated) {
          if (!config.contains(pos.dest)) {
            config[pos.dest] = nlohmann::json::array();
          }
          config[pos.dest].push_back(converted);
        } else {
          config[pos.dest] = converted;
          ++pos_cursor;
        }
        ++i;
      }

      return LevelOk{config, command_path, i};
    }

    // -----------------------------------------------------------------------
    // Post-processing: env fallback
    // -----------------------------------------------------------------------

    inline void
    apply_env(
      nlohmann::json& config,
      const std::vector<arg::ArgSpec>& args,
      const EnvLookup& env) {
      for (const auto& a : args) {
        std::visit(
          [&](const auto& spec) {
            using T = std::decay_t<decltype(spec)>;
            if constexpr (std::is_same_v<T, arg::FlagSpec>) {
              if (config.contains(spec.dest) && config[spec.dest] != false) {
                return; // already set by CLI
              }
              if (!spec.env.has_value()) { return; }
              auto val = env(spec.env->var);
              if (!val.has_value()) { return; }
              auto lower = *val;
              std::transform(
                lower.begin(),
                lower.end(),
                lower.begin(),
                [](unsigned char ch) { return std::tolower(ch); });
              if (lower == "true" || lower == "1") {
                config[spec.dest] = true;
              } else if (lower == "false" || lower == "0") {
                config[spec.dest] = false;
              } else {
                throw Error(
                  "env " + spec.env->var + ": expected boolean value, got '" +
                  *val + "'");
              }
            } else if constexpr (std::is_same_v<T, arg::OptionSpec>) {
              if (config.contains(spec.dest)) {
                return; // already set by CLI
              }
              if (!spec.env.has_value()) { return; }
              auto val = env(spec.env->var);
              if (!val.has_value()) { return; }
              try {
                config[spec.dest] = spec.converter.parse(*val);
              } catch (const conv::Error& e) {
                throw Error("env " + spec.env->var + ": " + e.what());
              }
            }
            // FlagGroupSpec and PositionalSpec have no env
          },
          a);
      }
    }

    // -----------------------------------------------------------------------
    // Post-processing: defaults
    // -----------------------------------------------------------------------

    inline void
    apply_defaults(
      nlohmann::json& config, const std::vector<arg::ArgSpec>& args) {
      for (const auto& a : args) {
        std::visit(
          [&](const auto& spec) {
            using T = std::decay_t<decltype(spec)>;
            if constexpr (std::is_same_v<T, arg::FlagSpec>) {
              if (!config.contains(spec.dest)) { config[spec.dest] = false; }
            } else if constexpr (std::is_same_v<T, arg::OptionSpec>) {
              if (
                !config.contains(spec.dest) && spec.default_value.has_value()) {
                config[spec.dest] = *spec.default_value;
              }
            } else if constexpr (std::is_same_v<T, arg::PositionalSpec>) {
              if (
                !config.contains(spec.dest) && spec.default_value.has_value()) {
                config[spec.dest] = *spec.default_value;
              }
            } else if constexpr (std::is_same_v<T, arg::FlagGroupSpec>) {
              if (!config.contains(spec.dest)) {
                config[spec.dest] = spec.default_value;
              }
            }
          },
          a);
      }
    }

    // -----------------------------------------------------------------------
    // Post-processing: validation
    // -----------------------------------------------------------------------

    inline void
    run_validators(
      const nlohmann::json& config, const std::vector<arg::ArgSpec>& args) {
      for (const auto& a : args) {
        std::visit(
          [&](const auto& spec) {
            using T = std::decay_t<decltype(spec)>;
            if constexpr (std::is_same_v<T, arg::OptionSpec>) {
              std::optional<nlohmann::json> val;
              if (config.contains(spec.dest)) { val = config[spec.dest]; }
              try {
                spec.validator.check(spec.dest, val);
              } catch (const validate::Error& e) {
                throw Error(std::string(e.what()));
              }
            } else if constexpr (std::is_same_v<T, arg::PositionalSpec>) {
              std::optional<nlohmann::json> val;
              if (config.contains(spec.dest)) { val = config[spec.dest]; }
              try {
                spec.validator.check(spec.dest, val);
              } catch (const validate::Error& e) {
                throw Error(std::string(e.what()));
              }
            }
          },
          a);
      }
    }

    // -----------------------------------------------------------------------
    // Recursive post-processing across command levels
    // -----------------------------------------------------------------------

    inline void
    post_process(
      nlohmann::json& config,
      const std::vector<arg::ArgSpec>& args,
      const std::vector<cmd::CommandSpec>& commands,
      const std::vector<std::string>& command_path,
      std::size_t path_index,
      const EnvLookup& env) {
      apply_env(config, args, env);
      apply_defaults(config, args);
      run_validators(config, args);

      if (path_index < command_path.size()) {
        for (const auto& cmd : commands) {
          if (cmd.name == command_path[path_index]) {
            post_process(
              config,
              cmd.args,
              cmd.commands,
              command_path,
              path_index + 1,
              env);
            break;
          }
        }
      }
    }

  } // namespace detail

  // -------------------------------------------------------------------------
  // Top-level parse function
  // -------------------------------------------------------------------------

  inline ParseResult
  parse(
    const cmd::RootSpec& root,
    const std::vector<std::string>& args,
    EnvLookup env = default_env_lookup()) {
    auto level_result = detail::parse_level(
      root.args, root.commands, args, 0, true, root.version);

    if (auto* help = std::get_if<HelpRequest>(&level_result)) {
      return std::move(*help);
    }
    if (auto* manpage = std::get_if<ManpageRequest>(&level_result)) {
      return std::move(*manpage);
    }
    if (std::holds_alternative<VersionRequest>(level_result)) {
      return VersionRequest{};
    }

    auto& ok = std::get<detail::LevelOk>(level_result);
    detail::post_process(
      ok.config, root.args, root.commands, ok.command_path, 0, env);

    return ParseOk{std::move(ok.config), std::move(ok.command_path)};
  }

} // namespace json_commander::parse
