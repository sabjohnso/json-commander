#pragma once

#include <json_commander/model.hpp>

#include <string>
#include <vector>

namespace json_commander::completion {

  // -------------------------------------------------------------------------
  // Detail: helpers for extracting completion data from the model
  // -------------------------------------------------------------------------

  namespace detail {

    inline std::string
    first_doc_line(const model::DocString& doc) {
      return doc.empty() ? "" : doc[0];
    }

    inline std::string
    cli_name(const std::string& name) {
      return (name.size() == 1) ? ("-" + name) : ("--" + name);
    }

    struct OptionInfo {
      std::string long_name;
      std::string short_name;
      std::string description;
      bool takes_value;
      bool is_file;
      bool is_dir;
      bool is_path;
      std::vector<std::string> choices;
    };

    inline bool
    is_file_type(const model::TypeSpec& type) {
      if (auto* s = std::get_if<model::ScalarType>(&type)) {
        return *s == model::ScalarType::File;
      }
      return false;
    }

    inline bool
    is_dir_type(const model::TypeSpec& type) {
      if (auto* s = std::get_if<model::ScalarType>(&type)) {
        return *s == model::ScalarType::Dir;
      }
      return false;
    }

    inline bool
    is_path_type(const model::TypeSpec& type) {
      if (auto* s = std::get_if<model::ScalarType>(&type)) {
        return *s == model::ScalarType::Path;
      }
      return false;
    }

    struct CompletionData {
      std::vector<OptionInfo> options;
      std::vector<std::string> subcommand_names;
      std::vector<std::string> subcommand_docs;
      bool has_file_positional = false;
      bool has_dir_positional = false;
    };

    inline void
    collect_args(
      CompletionData& data, const std::vector<model::Argument>& args) {
      for (const auto& arg : args) {
        std::visit(
          [&](const auto& a) {
            using T = std::decay_t<decltype(a)>;
            if constexpr (std::is_same_v<T, model::Flag>) {
              OptionInfo info;
              info.takes_value = false;
              info.is_file = false;
              info.is_dir = false;
              info.is_path = false;
              info.description = first_doc_line(a.doc);
              for (const auto& name : a.names) {
                if (name.size() == 1) {
                  info.short_name = name;
                } else {
                  info.long_name = name;
                }
              }
              data.options.push_back(info);
            } else if constexpr (std::is_same_v<T, model::Option>) {
              OptionInfo info;
              info.takes_value = true;
              info.is_file = is_file_type(a.type);
              info.is_dir = is_dir_type(a.type);
              info.is_path = is_path_type(a.type);
              info.description = first_doc_line(a.doc);
              if (a.choices.has_value()) { info.choices = *a.choices; }
              for (const auto& name : a.names) {
                if (name.size() == 1) {
                  info.short_name = name;
                } else {
                  info.long_name = name;
                }
              }
              data.options.push_back(info);
            } else if constexpr (std::is_same_v<T, model::FlagGroup>) {
              for (const auto& entry : a.flags) {
                OptionInfo info;
                info.takes_value = false;
                info.is_file = false;
                info.is_dir = false;
                info.is_path = false;
                info.description = first_doc_line(entry.doc);
                for (const auto& name : entry.names) {
                  if (name.size() == 1) {
                    info.short_name = name;
                  } else {
                    info.long_name = name;
                  }
                }
                data.options.push_back(info);
              }
            } else if constexpr (std::is_same_v<T, model::Positional>) {
              if (is_file_type(a.type) || is_path_type(a.type)) {
                data.has_file_positional = true;
              }
              if (is_dir_type(a.type)) { data.has_dir_positional = true; }
            }
          },
          arg);
      }
    }

    inline CompletionData
    collect(const model::Root& root) {
      CompletionData data;
      if (root.args.has_value()) { collect_args(data, *root.args); }
      if (root.commands.has_value()) {
        for (const auto& cmd : *root.commands) {
          data.subcommand_names.push_back(cmd.name);
          data.subcommand_docs.push_back(first_doc_line(cmd.doc));
        }
      }
      return data;
    }

    inline CompletionData
    collect_command(const model::Command& cmd) {
      CompletionData data;
      if (cmd.args.has_value()) { collect_args(data, *cmd.args); }
      if (cmd.commands.has_value()) {
        for (const auto& sub : *cmd.commands) {
          data.subcommand_names.push_back(sub.name);
          data.subcommand_docs.push_back(first_doc_line(sub.doc));
        }
      }
      return data;
    }

    inline std::string
    sanitize_function_name(const std::string& name) {
      std::string result;
      for (char c : name) {
        result += (c == '-') ? '_' : c;
      }
      return result;
    }

    // Builtin flags that every command gets
    inline std::vector<OptionInfo>
    builtin_options(bool is_root) {
      std::vector<OptionInfo> builtins;
      builtins.push_back(
        {"help", "h", "Show help information", false, false, false, false, {}});
      builtins.push_back(
        {"help-man", "", "Show man page", false, false, false, false, {}});
      builtins.push_back(
        {"help-completion",
         "",
         "Generate shell completions",
         true,
         false,
         false,
         false,
         {"bash", "zsh", "fish"}});
      if (is_root) {
        builtins.push_back(
          {"version", "", "Show version", false, false, false, false, {}});
      }
      return builtins;
    }

  } // namespace detail

  // -------------------------------------------------------------------------
  // Bash completion
  // -------------------------------------------------------------------------

  namespace detail {

    inline void
    bash_emit_level(
      std::string& out,
      const std::string& func_name,
      const CompletionData& data,
      bool is_root) {
      out += func_name + "() {\n";
      out += "  local cur prev opts\n";
      out += "  cur=\"${COMP_WORDS[COMP_CWORD]}\"\n";
      out += "  prev=\"${COMP_WORDS[COMP_CWORD-1]}\"\n";
      out += "\n";

      // Collect all option strings
      std::vector<std::string> all_opts;
      auto builtins = builtin_options(is_root);
      for (const auto& b : builtins) {
        all_opts.push_back("--" + b.long_name);
        if (!b.short_name.empty()) { all_opts.push_back("-" + b.short_name); }
      }
      for (const auto& opt : data.options) {
        if (!opt.long_name.empty()) {
          all_opts.push_back("--" + opt.long_name);
        }
        if (!opt.short_name.empty()) {
          all_opts.push_back("-" + opt.short_name);
        }
      }
      for (const auto& sub : data.subcommand_names) {
        all_opts.push_back(sub);
      }

      // Handle prev-word completions for options that take values
      bool has_value_opts = false;
      for (const auto& opt : data.options) {
        if (!opt.takes_value) { continue; }
        has_value_opts = true;
      }
      // Also check builtins
      for (const auto& b : builtins) {
        if (b.takes_value) { has_value_opts = true; }
      }

      if (has_value_opts) {
        out += "  case \"$prev\" in\n";
        auto emit_value_case = [&](const OptionInfo& opt) {
          std::string names;
          if (!opt.long_name.empty()) { names += "--" + opt.long_name; }
          if (!opt.short_name.empty()) {
            if (!names.empty()) { names += "|"; }
            names += "-" + opt.short_name;
          }
          out += "    " + names + ")\n";
          if (!opt.choices.empty()) {
            std::string choices;
            for (const auto& c : opt.choices) {
              if (!choices.empty()) { choices += " "; }
              choices += c;
            }
            out += "      COMPREPLY=($(compgen -W \"" + choices +
                   "\" -- \"$cur\"))\n";
          } else if (opt.is_file || opt.is_path) {
            out += "      compopt -o filenames\n";
            out += "      COMPREPLY=($(compgen -f -- \"$cur\"))\n";
          } else if (opt.is_dir) {
            out += "      compopt -o filenames\n";
            out += "      COMPREPLY=($(compgen -d -- \"$cur\"))\n";
          } else {
            out += "      return\n";
          }
          out += "      return\n";
          out += "      ;;\n";
        };

        for (const auto& b : builtins) {
          if (b.takes_value) { emit_value_case(b); }
        }
        for (const auto& opt : data.options) {
          if (opt.takes_value) { emit_value_case(opt); }
        }
        out += "  esac\n";
        out += "\n";
      }

      // Handle subcommand dispatch
      if (!data.subcommand_names.empty()) {
        out += "  # Check if a subcommand has been given\n";
        out += "  local subcmd=\"\"\n";
        out += "  for ((i=1; i < COMP_CWORD; i++)); do\n";
        out += "    case \"${COMP_WORDS[i]}\" in\n";
        for (const auto& sub : data.subcommand_names) {
          out += "      " + sub + ") subcmd=\"" + sub + "\"; break ;;\n";
        }
        out += "    esac\n";
        out += "  done\n";
        out += "\n";
        out += "  if [[ -n \"$subcmd\" ]]; then\n";
        out += "    case \"$subcmd\" in\n";
        for (const auto& sub : data.subcommand_names) {
          out += "      " + sub + ") " + func_name + "__" +
                 sanitize_function_name(sub) + " ;;\n";
        }
        out += "    esac\n";
        out += "    return\n";
        out += "  fi\n";
        out += "\n";
      }

      // Default: complete options and subcommands
      std::string opts_str;
      for (const auto& o : all_opts) {
        if (!opts_str.empty()) { opts_str += " "; }
        opts_str += o;
      }
      out += "  opts=\"" + opts_str + "\"\n";

      if (data.has_file_positional || data.has_dir_positional) {
        out += "  compopt -o filenames\n";
        if (data.has_dir_positional && !data.has_file_positional) {
          out += "  COMPREPLY=($(compgen -W \"$opts\" -d -- \"$cur\"))\n";
        } else {
          out += "  COMPREPLY=($(compgen -W \"$opts\" -f -- \"$cur\"))\n";
        }
      } else {
        out += "  COMPREPLY=($(compgen -W \"$opts\" -- \"$cur\"))\n";
      }

      out += "}\n";
    }

    inline void
    bash_emit_subcommands(
      std::string& out,
      const std::string& parent_func,
      const std::vector<model::Command>& commands) {
      for (const auto& cmd : commands) {
        auto sub_data = collect_command(cmd);
        std::string sub_func =
          parent_func + "__" + sanitize_function_name(cmd.name);
        bash_emit_level(out, sub_func, sub_data, false);
        out += "\n";
        if (cmd.commands.has_value() && !cmd.commands->empty()) {
          bash_emit_subcommands(out, sub_func, *cmd.commands);
        }
      }
    }

  } // namespace detail

  inline std::string
  to_bash(const model::Root& root) {
    std::string out;
    std::string func_name = "_" + detail::sanitize_function_name(root.name);

    // Emit subcommand functions first (bottom-up for bash)
    if (root.commands.has_value() && !root.commands->empty()) {
      detail::bash_emit_subcommands(out, func_name, *root.commands);
    }

    auto data = detail::collect(root);
    detail::bash_emit_level(out, func_name, data, true);

    out += "\ncomplete -F " + func_name + " " + root.name + "\n";
    return out;
  }

  // -------------------------------------------------------------------------
  // Zsh completion
  // -------------------------------------------------------------------------

  namespace detail {

    inline std::string
    zsh_escape(const std::string& s) {
      std::string result;
      for (char c : s) {
        if (c == '\'' || c == '[' || c == ']' || c == ':') { result += '\\'; }
        result += c;
      }
      return result;
    }

    inline std::string
    zsh_action_for(const OptionInfo& opt) {
      if (!opt.choices.empty()) {
        std::string choices;
        for (const auto& c : opt.choices) {
          if (!choices.empty()) { choices += " "; }
          choices += c;
        }
        return ":value:(" + choices + ")";
      }
      if (opt.is_file || opt.is_path) { return ":file:_files"; }
      if (opt.is_dir) { return ":directory:_directories"; }
      return ":value:";
    }

    inline void
    zsh_emit_level(
      std::string& out,
      const std::string& func_name,
      const CompletionData& data,
      bool is_root,
      const std::string& indent) {
      out += indent + func_name + "() {\n";

      if (!data.subcommand_names.empty()) {
        out += indent + "  local -a subcmds\n";
        out += indent + "  subcmds=(\n";
        for (std::size_t i = 0; i < data.subcommand_names.size(); ++i) {
          out += indent + "    '" + data.subcommand_names[i] + ":" +
                 zsh_escape(data.subcommand_docs[i]) + "'\n";
        }
        out += indent + "  )\n\n";
      }

      out += indent + "  _arguments -s \\\n";

      auto builtins = builtin_options(is_root);
      for (const auto& b : builtins) {
        std::string spec;
        if (!b.short_name.empty()) {
          spec = indent + "    '(-" + b.short_name + " --" + b.long_name +
                 ")'{-" + b.short_name + ",--" + b.long_name + "}'[" +
                 zsh_escape(b.description) + "]";
        } else {
          spec = indent + "    '--" + b.long_name + "[" +
                 zsh_escape(b.description) + "]";
        }
        if (b.takes_value) { spec += zsh_action_for(b); }
        spec += "' \\\n";
        out += spec;
      }

      for (const auto& opt : data.options) {
        std::string spec;
        if (!opt.short_name.empty() && !opt.long_name.empty()) {
          spec = indent + "    '(-" + opt.short_name + " --" + opt.long_name +
                 ")'{-" + opt.short_name + ",--" + opt.long_name + "}'[" +
                 zsh_escape(opt.description) + "]";
        } else if (!opt.long_name.empty()) {
          spec = indent + "    '--" + opt.long_name + "[" +
                 zsh_escape(opt.description) + "]";
        } else if (!opt.short_name.empty()) {
          spec = indent + "    '-" + opt.short_name + "[" +
                 zsh_escape(opt.description) + "]";
        }
        if (opt.takes_value) { spec += zsh_action_for(opt); }
        spec += "' \\\n";
        out += spec;
      }

      if (!data.subcommand_names.empty()) {
        out += indent + "    '1:command:->subcmd' \\\n";
        out += indent + "    '*::arg:->args'\n\n";
        out += indent + "  case $state in\n";
        out += indent + "    subcmd)\n";
        out += indent + "      _describe 'command' subcmds\n";
        out += indent + "      ;;\n";
        out += indent + "    args)\n";
        out += indent + "      case $words[1] in\n";
        for (const auto& sub : data.subcommand_names) {
          out += indent + "        " + sub + ") " + func_name + "__" +
                 sanitize_function_name(sub) + " ;;\n";
        }
        out += indent + "      esac\n";
        out += indent + "      ;;\n";
        out += indent + "  esac\n";
      } else {
        // Remove trailing backslash-newline and close
        if (out.size() >= 3 && out.substr(out.size() - 3) == " \\\n") {
          out.resize(out.size() - 3);
          out += "\n";
        }
      }

      out += indent + "}\n";
    }

    inline void
    zsh_emit_subcommands(
      std::string& out,
      const std::string& parent_func,
      const std::vector<model::Command>& commands) {
      for (const auto& cmd : commands) {
        auto sub_data = collect_command(cmd);
        std::string sub_func =
          parent_func + "__" + sanitize_function_name(cmd.name);
        zsh_emit_level(out, sub_func, sub_data, false, "");
        out += "\n";
        if (cmd.commands.has_value() && !cmd.commands->empty()) {
          zsh_emit_subcommands(out, sub_func, *cmd.commands);
        }
      }
    }

  } // namespace detail

  inline std::string
  to_zsh(const model::Root& root) {
    std::string out;
    out += "#compdef " + root.name + "\n\n";

    std::string func_name = "_" + detail::sanitize_function_name(root.name);

    // Emit subcommand functions first
    if (root.commands.has_value() && !root.commands->empty()) {
      detail::zsh_emit_subcommands(out, func_name, *root.commands);
    }

    auto data = detail::collect(root);
    detail::zsh_emit_level(out, func_name, data, true, "");

    out += "\ncompdef " + func_name + " " + root.name + "\n";
    return out;
  }

  // -------------------------------------------------------------------------
  // Fish completion
  // -------------------------------------------------------------------------

  namespace detail {

    inline std::string
    fish_escape(const std::string& s) {
      std::string result;
      for (char c : s) {
        if (c == '\'') {
          result += "\\'";
        } else {
          result += c;
        }
      }
      return result;
    }

    inline void
    fish_emit_option(
      std::string& out,
      const std::string& cmd_name,
      const OptionInfo& opt,
      const std::string& condition) {
      out += "complete -c " + cmd_name;
      if (!condition.empty()) { out += " -n '" + condition + "'"; }
      if (!opt.long_name.empty()) { out += " -l " + opt.long_name; }
      if (!opt.short_name.empty()) { out += " -s " + opt.short_name; }
      if (opt.takes_value) {
        if (opt.is_file || opt.is_path) {
          out += " -r -F";
        } else if (opt.is_dir) {
          out += " -r -xa '(__fish_complete_directories)'";
        } else if (!opt.choices.empty()) {
          std::string choices;
          for (const auto& c : opt.choices) {
            if (!choices.empty()) { choices += " "; }
            choices += c;
          }
          out += " -r -xa '" + choices + "'";
        } else {
          out += " -r";
        }
      }
      if (!opt.description.empty()) {
        out += " -d '" + fish_escape(opt.description) + "'";
      }
      out += "\n";
    }

    inline void
    fish_emit_level(
      std::string& out,
      const std::string& cmd_name,
      const CompletionData& data,
      bool is_root,
      const std::string& condition) {
      auto builtins = builtin_options(is_root);
      for (const auto& b : builtins) {
        fish_emit_option(out, cmd_name, b, condition);
      }
      for (const auto& opt : data.options) {
        fish_emit_option(out, cmd_name, opt, condition);
      }

      // Subcommands
      for (std::size_t i = 0; i < data.subcommand_names.size(); ++i) {
        out += "complete -c " + cmd_name;
        if (!condition.empty()) { out += " -n '" + condition + "'"; }
        out += " -f -a '" + data.subcommand_names[i] + "'";
        if (
          i < data.subcommand_docs.size() && !data.subcommand_docs[i].empty()) {
          out += " -d '" + fish_escape(data.subcommand_docs[i]) + "'";
        }
        out += "\n";
      }
    }

    inline void
    fish_emit_subcommands(
      std::string& out,
      const std::string& cmd_name,
      const std::vector<model::Command>& commands,
      const std::string& parent_condition) {
      for (const auto& cmd : commands) {
        auto sub_data = collect_command(cmd);
        std::string condition = "__fish_seen_subcommand_from " + cmd.name;
        if (!parent_condition.empty()) {
          condition = parent_condition + "; and " + condition;
        }
        out += "\n# " + cmd.name + " subcommand\n";
        fish_emit_level(out, cmd_name, sub_data, false, condition);

        if (cmd.commands.has_value() && !cmd.commands->empty()) {
          fish_emit_subcommands(out, cmd_name, *cmd.commands, condition);
        }
      }
    }

  } // namespace detail

  inline std::string
  to_fish(const model::Root& root) {
    std::string out;
    out += "# Fish completions for " + root.name + "\n\n";

    auto data = detail::collect(root);
    bool has_subcommands = !data.subcommand_names.empty();

    std::string condition;
    if (has_subcommands) { condition = "__fish_use_subcommand"; }

    detail::fish_emit_level(out, root.name, data, true, condition);

    if (root.commands.has_value() && !root.commands->empty()) {
      detail::fish_emit_subcommands(out, root.name, *root.commands, "");
    }

    return out;
  }

} // namespace json_commander::completion
