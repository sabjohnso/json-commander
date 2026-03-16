#pragma once

#include <json_commander/cmd.hpp>
#include <json_commander/completion.hpp>
#include <json_commander/config_schema.hpp>
#include <json_commander/manpage.hpp>
#include <json_commander/parse.hpp>
#include <json_commander/schema_loader.hpp>

#include <nlohmann/json-schema.hpp>

#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#define JCMD_ISATTY(fd) _isatty(fd)
#define JCMD_STDOUT_FD _fileno(stdout)
#define JCMD_STDERR_FD _fileno(stderr)
#else
#include <sys/ioctl.h>
#include <unistd.h>
#define JCMD_ISATTY(fd) isatty(fd)
#define JCMD_STDOUT_FD STDOUT_FILENO
#define JCMD_STDERR_FD STDERR_FILENO
#endif

namespace json_commander {

  inline int
  terminal_width(int fd) {
#ifdef _WIN32
    HANDLE h =
      GetStdHandle(fd == JCMD_STDOUT_FD ? STD_OUTPUT_HANDLE : STD_ERROR_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (GetConsoleScreenBufferInfo(h, &info)) {
      return info.srWindow.Right - info.srWindow.Left + 1;
    }
    return 80;
#else
    struct winsize ws{};
    if (ioctl(fd, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) { return ws.ws_col; }
    return 80;
#endif
  }

  using MainFn = std::function<int(const nlohmann::json& config)>;

  // -------------------------------------------------------------------------
  // Core overload: model::Root → run
  // -------------------------------------------------------------------------

  inline int
  run(const model::Root& root, int argc, char* argv[], MainFn main_fn) {
    std::string name =
      (argc > 0 && argv && argv[0] && argv[0][0] != '\0') ? argv[0] : "error";

    auto spec = cmd::make(root);

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
      args.emplace_back(argv[i]);
    }

    parse::ParseResult result;
    try {
      result = parse::parse(spec, args);
    } catch (const parse::Error& e) {
      std::cerr << name << ": " << e.what() << "\n";
      if (JCMD_ISATTY(JCMD_STDERR_FD)) {
        int width = terminal_width(JCMD_STDERR_FD);
        std::cerr << manpage::to_ansi_text(root, {}, width);
      } else {
        std::cerr << manpage::to_plain_text(root, {});
      }
      return 1;
    }

    return std::visit(
      [&](const auto& r) -> int {
        using T = std::decay_t<decltype(r)>;

        if constexpr (std::is_same_v<T, parse::ParseOk>) {
          // Check for missing subcommand: walk into the nested config
          // to find the deepest level that should have a "command" but doesn't.
          {
            const nlohmann::json* cfg = &r.config;
            const model::Root* cur_root = &root;
            std::vector<std::string> help_path = r.command_path;
            bool has_commands =
              cur_root->commands.has_value() && !cur_root->commands->empty();

            // Walk through selected commands to find the leaf
            const std::vector<model::Command>* cmds =
              has_commands ? &*cur_root->commands : nullptr;
            for (const auto& seg : r.command_path) {
              if (!cmds) break;
              for (const auto& cmd : *cmds) {
                if (cmd.name == seg) {
                  cfg = &(*cfg)[seg];
                  cmds = (cmd.commands.has_value() && !cmd.commands->empty())
                           ? &*cmd.commands
                           : nullptr;
                  has_commands = cmds != nullptr;
                  break;
                }
              }
            }

            if (has_commands && !cfg->contains("command")) {
              std::cerr << name << ": missing subcommand\n";
              if (JCMD_ISATTY(JCMD_STDERR_FD)) {
                int width = terminal_width(JCMD_STDERR_FD);
                std::cerr << manpage::to_ansi_text(root, r.command_path, width);
              } else {
                std::cerr << manpage::to_plain_text(root, r.command_path);
              }
              return 1;
            }
          }

          try {
            auto schema = config_schema::to_config_schema(root, r.command_path);
            nlohmann::json_schema::json_validator validator;
            validator.set_root_schema(schema);
            validator.validate(r.config);
          } catch (const std::exception& e) {
            std::cerr << name
                      << ": internal error: config failed schema validation: "
                      << e.what() << "\n";
            return 1;
          }
          return main_fn(r.config);
        } else if constexpr (std::is_same_v<T, parse::HelpRequest>) {
          if (JCMD_ISATTY(JCMD_STDOUT_FD)) {
            int width = terminal_width(JCMD_STDOUT_FD);
            std::cout << manpage::to_ansi_text(root, r.command_path, width);
          } else {
            std::cout << manpage::to_plain_text(root, r.command_path);
          }
          return 0;
        } else if constexpr (std::is_same_v<T, parse::VersionRequest>) {
          std::cout << name << " version";
          if (root.version) { std::cout << " " << *root.version; }
          std::cout << "\n";
          return 0;
        } else if constexpr (std::is_same_v<T, parse::ManpageRequest>) {
          std::cout << manpage::to_groff(root, r.command_path);
          return 0;
        } else if constexpr (std::is_same_v<T, parse::CompletionRequest>) {
          if (r.shell == "bash") {
            std::cout << completion::to_bash(root);
          } else if (r.shell == "zsh") {
            std::cout << completion::to_zsh(root);
          } else if (r.shell == "fish") {
            std::cout << completion::to_fish(root);
          }
          return 0;
        }
      },
      result);
  }

  // -------------------------------------------------------------------------
  // JSON string overload: parse JSON → load schema → delegate to Root overload
  // -------------------------------------------------------------------------

  inline int
  run(const std::string& cli_json, int argc, char* argv[], MainFn main_fn) {
    std::string name =
      (argc > 0 && argv && argv[0] && argv[0][0] != '\0') ? argv[0] : "error";

    model::Root root;
    try {
      schema::Loader loader;
      auto j = nlohmann::json::parse(cli_json);
      root = loader.load(j);
    } catch (const nlohmann::json::exception& e) {
      std::cerr << name
                << ": invalid CLI definition. Use json-commander validate to "
                   "check your schema.\n";
      throw;
    } catch (const schema::Error& e) {
      std::cerr << name
                << ": invalid CLI definition. Use json-commander validate to "
                   "check your schema.\n";
      throw;
    }

    return run(root, argc, argv, std::move(main_fn));
  }

  // -------------------------------------------------------------------------
  // File overload: load schema from path → delegate to Root overload
  // -------------------------------------------------------------------------

  inline int
  run_file(
    const std::filesystem::path& schema_path,
    int argc,
    char* argv[],
    MainFn main_fn) {
    std::string name =
      (argc > 0 && argv && argv[0] && argv[0][0] != '\0') ? argv[0] : "error";

    model::Root root;
    try {
      schema::Loader loader;
      root = loader.load(schema_path.string());
    } catch (const nlohmann::json::exception& e) {
      std::cerr << name
                << ": invalid CLI definition. Use json-commander validate to "
                   "check your schema.\n";
      throw;
    } catch (const schema::Error& e) {
      std::cerr << name
                << ": invalid CLI definition. Use json-commander validate to "
                   "check your schema.\n";
      throw;
    }

    return run(root, argc, argv, std::move(main_fn));
  }

} // namespace json_commander
