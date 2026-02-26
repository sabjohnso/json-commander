#pragma once

#include <json_commander/cmd.hpp>
#include <json_commander/completion.hpp>
#include <json_commander/manpage.hpp>
#include <json_commander/parse.hpp>
#include <json_commander/schema_loader.hpp>

#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <io.h>
#define JCMD_ISATTY(fd) _isatty(fd)
#define JCMD_STDOUT_FD _fileno(stdout)
#define JCMD_STDERR_FD _fileno(stderr)
#else
#include <unistd.h>
#define JCMD_ISATTY(fd) isatty(fd)
#define JCMD_STDOUT_FD STDOUT_FILENO
#define JCMD_STDERR_FD STDERR_FILENO
#endif

namespace json_commander {

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
        std::cerr << manpage::to_ansi_text(root, {});
      } else {
        std::cerr << manpage::to_plain_text(root, {});
      }
      return 1;
    }

    return std::visit(
      [&](const auto& r) -> int {
        using T = std::decay_t<decltype(r)>;

        if constexpr (std::is_same_v<T, parse::ParseOk>) {
          return main_fn(r.config);
        } else if constexpr (std::is_same_v<T, parse::HelpRequest>) {
          if (JCMD_ISATTY(JCMD_STDOUT_FD)) {
            std::cout << manpage::to_ansi_text(root, r.command_path);
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
    } catch (...) {
      std::cerr << name
                << ": invalid CLI definition. Use json-commander validate to "
                   "check your schema.\n";
      return 1;
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
    } catch (...) {
      std::cerr << name
                << ": invalid CLI definition. Use json-commander validate to "
                   "check your schema.\n";
      return 1;
    }

    return run(root, argc, argv, std::move(main_fn));
  }

} // namespace json_commander
