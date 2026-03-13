// json_commander_wasm/wasm_shim.cpp
//
// Wasm shim layer: adapts the json-commander C++ library for browser use.
// Exposes five stateless functions via Emscripten embind, each accepting
// and returning JSON-encoded strings.

#include <json_commander/cmd.hpp>
#include <json_commander/config_schema.hpp>
#include <json_commander/manpage.hpp>
#include <json_commander/parse.hpp>
#include <json_commander/schema_loader.hpp>

#include <emscripten/bind.h>
#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace json_commander::wasm {

  using json = nlohmann::json;

  // ---------------------------------------------------------------------------
  // Error response helpers
  // ---------------------------------------------------------------------------

  namespace detail {

    json
    errorResponse(const std::string& message, const std::string& path = "") {
      json err = {{"message", message}};
      if (!path.empty()) { err["path"] = path; }
      return {{"success", false}, {"errors", json::array({err})}};
    }

    json
    catchAll(auto&& fn) {
      try {
        return fn();
      } catch (const nlohmann::json::parse_error& e) {
        return errorResponse(std::string("Invalid JSON: ") + e.what());
      } catch (const schema::Error& e) {
        return errorResponse(e.what());
      } catch (const parse::Error& e) {
        return errorResponse(e.what());
      } catch (const conv::Error& e) {
        return errorResponse(e.what());
      } catch (const validate::Error& e) {
        return errorResponse(e.what());
      } catch (const std::exception& e) {
        return errorResponse(std::string("Internal error: ") + e.what());
      }
    }

    model::Root
    loadSchema(const std::string& schemaJson) {
      auto j = json::parse(schemaJson);
      schema::Loader loader;
      return loader.load(j);
    }

    std::vector<std::string>
    parseCommandPath(const json& input) {
      std::vector<std::string> path;
      if (input.contains("commandPath") && input["commandPath"].is_array()) {
        for (const auto& elem : input["commandPath"]) {
          path.push_back(elem.get<std::string>());
        }
      }
      return path;
    }

    std::vector<std::string>
    findCommand(
      const model::Root& root, const std::vector<std::string>& commandPath) {
      // Validate that the command path exists in the model
      const std::vector<model::Command>* commands =
        root.commands.has_value() ? &(*root.commands) : nullptr;
      for (const auto& name : commandPath) {
        if (commands == nullptr) {
          throw schema::Error("Unknown subcommand: " + name);
        }
        bool found = false;
        for (const auto& cmd : *commands) {
          if (cmd.name == name) {
            commands = cmd.commands.has_value() ? &(*cmd.commands) : nullptr;
            found = true;
            break;
          }
        }
        if (!found) { throw schema::Error("Unknown subcommand: " + name); }
      }
      return commandPath;
    }

  } // namespace detail

  // ---------------------------------------------------------------------------
  // Public API functions
  // ---------------------------------------------------------------------------

  std::string
  validateSchema(const std::string& inputJson) {
    return detail::catchAll([&]() -> json {
             auto input = json::parse(inputJson);
             auto schemaJson = input["schemaJson"].get<std::string>();
             detail::loadSchema(schemaJson);
             return {{"success", true}};
           })
      .dump();
  }

  std::string
  generateHelp(const std::string& inputJson) {
    return detail::catchAll([&]() -> json {
             auto input = json::parse(inputJson);
             auto schemaJson = input["schemaJson"].get<std::string>();
             auto root = detail::loadSchema(schemaJson);
             auto commandPath = detail::parseCommandPath(input);
             detail::findCommand(root, commandPath);
             auto text = manpage::to_plain_text(root, commandPath);
             return {{"success", true}, {"text", text}};
           })
      .dump();
  }

  std::string
  generateManpage(const std::string& inputJson) {
    return detail::catchAll([&]() -> json {
             auto input = json::parse(inputJson);
             auto schemaJson = input["schemaJson"].get<std::string>();
             auto root = detail::loadSchema(schemaJson);
             auto commandPath = detail::parseCommandPath(input);
             detail::findCommand(root, commandPath);
             auto text = manpage::to_groff(root, commandPath);
             return {{"success", true}, {"text", text}};
           })
      .dump();
  }

  std::string
  generateConfigSchema(const std::string& inputJson) {
    return detail::catchAll([&]() -> json {
             auto input = json::parse(inputJson);
             auto schemaJson = input["schemaJson"].get<std::string>();
             auto root = detail::loadSchema(schemaJson);
             auto commandPath = detail::parseCommandPath(input);
             detail::findCommand(root, commandPath);
             auto schema = config_schema::to_config_schema(root, commandPath);
             return {{"success", true}, {"schema", schema}};
           })
      .dump();
  }

  std::string
  parseArgs(const std::string& inputJson) {
    return detail::catchAll([&]() -> json {
             auto input = json::parse(inputJson);
             auto schemaJson = input["schemaJson"].get<std::string>();
             auto root = detail::loadSchema(schemaJson);

             // Build argv from input (no program name needed — parse::parse
             // expects args without argv[0])
             std::vector<std::string> argv;
             if (input.contains("argv") && input["argv"].is_array()) {
               for (const auto& arg : input["argv"]) {
                 argv.push_back(arg.get<std::string>());
               }
             }

             // Check for --help, --version, --man and reject them
             for (const auto& arg : argv) {
               if (arg == "--help" || arg == "-h") {
                 return detail::errorResponse(
                   "Use generateHelp() for help text instead of --help");
               }
               if (arg == "--version") {
                 return detail::errorResponse(
                   "Version information is available in the schema metadata "
                   "instead of --version");
               }
               if (arg == "--man") {
                 return detail::errorResponse(
                   "Use generateManpage() for man pages instead of --man");
               }
             }

             // Build environment lookup
             parse::EnvLookup envLookup = parse::no_env();
             if (input.contains("env") && input["env"].is_object()) {
               auto envMap =
                 input["env"].get<std::map<std::string, std::string>>();
               envLookup =
                 [envMap = std::move(envMap)](
                   const std::string& var) -> std::optional<std::string> {
                 auto it = envMap.find(var);
                 if (it == envMap.end()) { return std::nullopt; }
                 return it->second;
               };
             }

             // Compile and parse
             auto spec = cmd::make(root);
             auto result = parse::parse(spec, argv, envLookup);

             return std::visit(
               [](auto&& r) -> json {
                 using T = std::decay_t<decltype(r)>;
                 if constexpr (std::is_same_v<T, parse::ParseOk>) {
                   return {{"success", true}, {"config", r.config}};
                 } else if constexpr (std::is_same_v<T, parse::HelpRequest>) {
                   return detail::errorResponse(
                     "Use generateHelp() for help text instead of --help");
                 } else if constexpr (std::
                                        is_same_v<T, parse::VersionRequest>) {
                   return detail::errorResponse(
                     "Version information is available in the schema metadata "
                     "instead of --version");
                 } else if constexpr (std::
                                        is_same_v<T, parse::ManpageRequest>) {
                   return detail::errorResponse(
                     "Use generateManpage() for man pages instead of --man");
                 } else if constexpr (std::is_same_v<
                                        T,
                                        parse::CompletionRequest>) {
                   return detail::errorResponse(
                     "Shell completion is not available in the browser");
                 }
               },
               result);
           })
      .dump();
  }

} // namespace json_commander::wasm

// ---------------------------------------------------------------------------
// Embind bindings
// ---------------------------------------------------------------------------

EMSCRIPTEN_BINDINGS(json_commander_wasm) {
  emscripten::function("validateSchema", &json_commander::wasm::validateSchema);
  emscripten::function("generateHelp", &json_commander::wasm::generateHelp);
  emscripten::function(
    "generateManpage", &json_commander::wasm::generateManpage);
  emscripten::function(
    "generateConfigSchema", &json_commander::wasm::generateConfigSchema);
  emscripten::function("parseArgs", &json_commander::wasm::parseArgs);
}
