// fake-git — A schema-driven CLI example modeling git's porcelain interface.
//
// Demonstrates:
//   - Nested subcommands (stash push, remote add, …)
//   - Flag groups for mutually exclusive modes (--rebase/--no-rebase)
//   - Repeated options (-c KEY=VALUE …)
//   - Environment variable bindings (GIT_DIR, GIT_WORK_TREE)
//   - Repeated positional arguments ([pathspec…])

#include <json_commander/cmd.hpp>
#include <json_commander/manpage.hpp>
#include <json_commander/parse.hpp>
#include <json_commander/schema_loader.hpp>

#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <io.h>
#define FAKEGIT_ISATTY(fd) _isatty(fd)
#define FAKEGIT_STDOUT_FD _fileno(stdout)
#else
#include <unistd.h>
#define FAKEGIT_ISATTY(fd) isatty(fd)
#define FAKEGIT_STDOUT_FD STDOUT_FILENO
#endif

using namespace json_commander;

// ---------------------------------------------------------------------------
// CLI definition
// ---------------------------------------------------------------------------

model::Root
make_cli() {
  schema::Loader loader;
  return loader.load(std::string(FAKEGIT_SCHEMA));
}

// ---------------------------------------------------------------------------
// Application logic
// ---------------------------------------------------------------------------

int
run(const std::vector<std::string> &args) {
  auto cli = make_cli();
  auto spec = cmd::make(cli);
  auto result = parse::parse(spec, args);

  if (auto *ok = std::get_if<parse::ParseOk>(&result)) {
    std::cout << ok->config.dump(2) << "\n";
    return 0;
  }

  if (auto *help = std::get_if<parse::HelpRequest>(&result)) {
    if (FAKEGIT_ISATTY(FAKEGIT_STDOUT_FD)) {
      std::cout << manpage::to_ansi_text(cli, help->command_path);
    } else {
      std::cout << manpage::to_plain_text(cli, help->command_path);
    }
    return 0;
  }

  if (auto *man = std::get_if<parse::ManpageRequest>(&result)) {
    std::cout << manpage::to_groff(cli, man->command_path);
    return 0;
  }

  if (std::holds_alternative<parse::VersionRequest>(result)) {
    std::cout << "fake-git version " << *cli.version << "\n";
    return 0;
  }

  return 1;
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int
main(int argc, char *argv[]) {
  try {
    return run({argv + 1, argv + argc});
  } catch (const schema::Error &e) {
    std::cerr << "schema error: " << e.what() << "\n";
    return 1;
  } catch (const parse::Error &e) {
    std::cerr << "error: " << e.what() << "\n";
    return 1;
  }
}
