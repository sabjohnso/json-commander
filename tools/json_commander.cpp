// json-commander — A unified CLI tool for working with json-commander schemas.
//
// Subcommands:
//   validate       Validate a schema against the metaschema
//   config-schema  Generate a JSON Schema for runtime configuration
//   parse          Parse arguments against a schema, output config
//   help           Generate plain-text help for a schema
//   man            Generate a groff man page for a schema

#include <json_commander/cmd.hpp>
#include <json_commander/completion.hpp>
#include <json_commander/config_schema.hpp>
#include <json_commander/manpage.hpp>
#include <json_commander/model_emit.hpp>
#include <json_commander/parse.hpp>
#include <json_commander/run.hpp>
#include <json_commander/schema_loader.hpp>

#include "json_commander_schema.hpp"

#include <iostream>
#include <string>
#include <vector>

using namespace json_commander;

// ---------------------------------------------------------------------------
// CLI definition
// ---------------------------------------------------------------------------

model::Root
make_cli() {
  schema::Loader loader;
  auto j = nlohmann::json::parse(json_commander_tool_schema);
  return loader.load(j);
}

// ---------------------------------------------------------------------------
// Subcommand handlers
// ---------------------------------------------------------------------------

int
do_validate(const nlohmann::json& config) {
  auto schema_file = config.at("schema-file").get<std::string>();
  schema::Loader loader;
  loader.load(schema_file);
  std::cout << "ok\n";
  return 0;
}

int
do_config_schema(const nlohmann::json& config) {
  auto schema_file = config.at("schema-file").get<std::string>();

  std::vector<std::string> command_path;
  if (config.contains("subcommand")) {
    command_path = config.at("subcommand").get<std::vector<std::string>>();
  }

  schema::Loader loader;
  auto root = loader.load(schema_file);
  auto schema = config_schema::to_config_schema(root, command_path);
  std::cout << schema.dump(2) << "\n";
  return 0;
}

int
do_parse(const nlohmann::json& config) {
  auto schema_file = config.at("schema-file").get<std::string>();

  std::vector<std::string> schema_args;
  if (config.contains("schema-args")) {
    schema_args = config.at("schema-args").get<std::vector<std::string>>();
  }

  schema::Loader loader;
  auto root = loader.load(schema_file);
  auto spec = cmd::make(root);
  auto result = parse::parse(spec, schema_args);

  if (auto* ok = std::get_if<parse::ParseOk>(&result)) {
    std::cout << ok->config.dump(2) << "\n";
    return 0;
  }

  if (auto* help = std::get_if<parse::HelpRequest>(&result)) {
    std::cout << manpage::to_plain_text(root, help->command_path);
    return 0;
  }

  if (auto* man = std::get_if<parse::ManpageRequest>(&result)) {
    std::cout << manpage::to_groff(root, man->command_path);
    return 0;
  }

  if (std::holds_alternative<parse::VersionRequest>(result)) {
    if (root.version) {
      std::cout << root.name << " version " << *root.version << "\n";
    }
    return 0;
  }

  return 1;
}

int
do_help(const nlohmann::json& config) {
  auto schema_file = config.at("schema-file").get<std::string>();

  std::vector<std::string> command_path;
  if (config.contains("subcommand")) {
    command_path = config.at("subcommand").get<std::vector<std::string>>();
  }

  schema::Loader loader;
  auto root = loader.load(schema_file);
  std::cout << manpage::to_plain_text(root, command_path);
  return 0;
}

int
do_codegen(const nlohmann::json& config) {
  auto schema_file = config.at("schema-file").get<std::string>();
  auto fn_name = config.at("function-name").get<std::string>();

  schema::Loader loader;
  auto root = loader.load(schema_file);
  std::cout << model_emit::emit_model_hpp(root, fn_name);
  return 0;
}

int
do_man(const nlohmann::json& config) {
  auto schema_file = config.at("schema-file").get<std::string>();

  std::vector<std::string> command_path;
  if (config.contains("subcommand")) {
    command_path = config.at("subcommand").get<std::vector<std::string>>();
  }

  schema::Loader loader;
  auto root = loader.load(schema_file);
  std::cout << manpage::to_groff(root, command_path);
  return 0;
}

// ---------------------------------------------------------------------------
// Dispatch
// ---------------------------------------------------------------------------

int
dispatch(const parse::ParseOk& ok) {
  const auto& command = ok.command_path.at(0);

  if (command == "validate") return do_validate(ok.config);
  if (command == "config-schema") return do_config_schema(ok.config);
  if (command == "parse") return do_parse(ok.config);
  if (command == "help") return do_help(ok.config);
  if (command == "man") return do_man(ok.config);
  if (command == "codegen") return do_codegen(ok.config);

  std::cerr << "unknown command: " << command << "\n";
  return 1;
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int
run(const std::vector<std::string>& args) {
  auto cli = make_cli();
  auto spec = cmd::make(cli);
  auto result = parse::parse(spec, args);

  if (auto* ok = std::get_if<parse::ParseOk>(&result)) {
    if (ok->command_path.empty()) {
      if (JCMD_ISATTY(JCMD_STDERR_FD)) {
        int width = terminal_width(JCMD_STDERR_FD);
        std::cerr << manpage::to_ansi_text(cli, {}, width);
      } else {
        std::cerr << manpage::to_plain_text(cli, {});
      }
      return 1;
    }
    return dispatch(*ok);
  }

  if (auto* help = std::get_if<parse::HelpRequest>(&result)) {
    if (JCMD_ISATTY(JCMD_STDOUT_FD)) {
      int width = terminal_width(JCMD_STDOUT_FD);
      std::cout << manpage::to_ansi_text(cli, help->command_path, width);
    } else {
      std::cout << manpage::to_plain_text(cli, help->command_path);
    }
    return 0;
  }

  if (auto* man = std::get_if<parse::ManpageRequest>(&result)) {
    std::cout << manpage::to_groff(cli, man->command_path);
    return 0;
  }

  if (auto* comp = std::get_if<parse::CompletionRequest>(&result)) {
    if (comp->shell == "bash") {
      std::cout << completion::to_bash(cli);
    } else if (comp->shell == "zsh") {
      std::cout << completion::to_zsh(cli);
    } else if (comp->shell == "fish") {
      std::cout << completion::to_fish(cli);
    }
    return 0;
  }

  if (std::holds_alternative<parse::VersionRequest>(result)) {
    std::cout << "json-commander version " << *cli.version << "\n";
    return 0;
  }

  return 1;
}

int
main(int argc, char* argv[]) {
  try {
    return run({argv + 1, argv + argc});
  } catch (const schema::Error& e) {
    std::cerr << "schema error: " << e.what() << "\n";
    return 1;
  } catch (const parse::Error& e) {
    std::cerr << "error: " << e.what() << "\n";
    return 1;
  } catch (const std::runtime_error& e) {
    std::cerr << "error: " << e.what() << "\n";
    return 1;
  }
}
