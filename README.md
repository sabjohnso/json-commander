# JSON-Commander

[![CI](https://github.com/JSON-Commander/json-commander/actions/workflows/ci.yml/badge.svg)](https://github.com/JSON-Commander/json-commander/actions/workflows/ci.yml)

A C++20 library for defining command line interfaces via JSON schemas
(current version: 0.4.0).

Inspired by OCaml's [cmdliner](https://erratique.ch/software/cmdliner),
JSON-Commander takes a declarative approach: describe your CLI as a JSON schema,
and JSON-Commander handles argument parsing, environment variable fallback,
configuration file merging, man page generation, and runtime configuration
output.

## Features

- **Declarative CLI specification** -- define arguments, options, flags,
  subcommands, and their documentation in JSON
- **Argument parsing** -- long/short options, flag grouping (`-abc`),
  positional arguments, `--` termination, nested subcommands
- **Environment variable fallback** -- options and flags can fall back to
  environment variables when not provided on the command line
- **Man page generation** -- produce groff output suitable for `man(1)` or
  plain-text help for `--help`
- **Config schema generation** -- emit a JSON Schema (draft 2020-12)
  describing the runtime configuration output
- **Schema validation** -- validate CLI schemas against the json-commander
  metaschema before use
- **Unified JSON output** -- parsing produces a flat JSON object ready for
  application consumption
- **Simplified entry point** -- `json_commander::run()` handles parsing,
  help, version, and man page dispatch in a single call
- **C API** -- shared library with a single `jcmd_run()` function for
  embedding in C programs or FFI bindings

## Quick Start

### CMake Executable Helper (Easiest)

The fastest way to build a CLI with JSON-Commander: write a JSON schema, write
a callback function, and add three lines of CMake. No `main()` boilerplate
needed.

**1. Define your CLI as a JSON schema** (`greet.json`):

```json
{
  "name": "greet",
  "doc": ["A friendly greeting tool."],
  "version": "1.0.0",
  "args": [
    {
      "kind": "flag",
      "names": ["loud", "l"],
      "doc": ["Print the greeting in uppercase."]
    },
    {
      "kind": "positional",
      "name": "name",
      "doc": ["The name to greet."],
      "type": "string",
      "required": true
    }
  ]
}
```

**2. Write your callback** (`greet_main.hpp`):

```cpp
#pragma once
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>

namespace greet {
int run(const nlohmann::json &config) {
  std::string greeting = "Hello, " + config["name"].get<std::string>() + "!";
  if (config["loud"].get<bool>()) {
    std::transform(greeting.begin(), greeting.end(), greeting.begin(), [](unsigned char c) {
      return static_cast<char>(std::toupper(c));
    });
  }
  std::cout << greeting << "\n";
  return 0;
}
} // namespace greet
```

**3. Add to CMake** (`CMakeLists.txt`):

```cmake
find_package(json-commander REQUIRED)

json_commander_add_executable(greet
  SCHEMA greet.json
  MAIN greet::run
  FROM_HEADER greet_main.hpp)
```

That's it -- JSON-Commander generates `main()`, handles `--help`, `--version`,
and `--man` automatically.

**Parameters:**

| Parameter     | Required | Description                                                                         |
|---------------|----------|-------------------------------------------------------------------------------------|
| `SCHEMA`      | yes      | Path to the JSON schema file (relative to `CMAKE_CURRENT_SOURCE_DIR`)               |
| `MAIN`        | yes      | Fully-qualified name of the callback function (`int(const nlohmann::json&)`)        |
| `FROM_HEADER` | no       | Header file to `#include` for the callback; omit if the function is already visible |

Additional source files can be passed as unnamed arguments after the keyword
parameters.

The function is available after `find_package(json-commander)` or when
building as a subdirectory.

### JSON Schema + `run_file()` (Runtime Loading)

Load a JSON schema at runtime with `json_commander::run_file()`. This gives
you full control over `main()` while still using a declarative JSON definition:

```cpp
#include <json_commander/run.hpp>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
  return json_commander::run_file("greet.json", argc, argv, [](const nlohmann::json &config) {
    std::string greeting = "Hello, " + config["name"].get<std::string>() + "!";
    if (config["loud"].get<bool>()) {
      std::transform(greeting.begin(), greeting.end(), greeting.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
      });
    }
    std::cout << greeting << "\n";
    return 0;
  });
}
```

### Programmatic C++ Model

For maximum flexibility, define your CLI programmatically as a `model::Root`:

```cpp
#include <json_commander/run.hpp>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>

using namespace json_commander;

model::Root make_cli() {
  model::Flag loud;
  loud.names = {"loud", "l"};
  loud.doc = {"Print the greeting in uppercase."};

  model::Positional name;
  name.name = "name";
  name.doc = {"The name to greet."};
  name.type = model::ScalarType::String;
  name.required = true;

  model::Root root;
  root.name = "greet";
  root.doc = {"A friendly greeting tool."};
  root.version = "1.0.0";
  root.args = std::vector<model::Argument>{loud, name};
  return root;
}

int main(int argc, char *argv[]) {
  return json_commander::run(make_cli(), argc, argv, [](const nlohmann::json &config) {
    std::string greeting = "Hello, " + config["name"].get<std::string>() + "!";
    if (config["loud"].get<bool>()) {
      std::transform(greeting.begin(), greeting.end(), greeting.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
      });
    }
    std::cout << greeting << "\n";
    return 0;
  });
}
```

### C API

JSON-Commander also provides a C shared library with a single-function
interface, suitable for embedding in C programs or creating FFI bindings:

```c
#include <json_commander.h>
#include <stdio.h>

static int greet_main(const char *config_json) {
  /* config_json is a JSON string with parsed arguments */
  printf("Got config: %s\n", config_json);
  return 0;
}

int main(int argc, char *argv[]) {
  const char *schema = "{\"name\":\"greet\", ...}";
  return jcmd_run(schema, argc, argv, greet_main);
}
```

## JSON-Commander CLI Tool

JSON-Commander ships with a `json-commander` tool that dogfoods the library:

```sh
json-commander validate schema.json          # Validate a schema
json-commander help schema.json              # Print plain-text help
json-commander man schema.json               # Generate groff man page
json-commander man schema.json commit        # Man page for a subcommand
json-commander config-schema schema.json     # Generate runtime config JSON Schema
json-commander parse schema.json -- --loud Alice  # Parse args, output JSON
```

## Building

JSON-Commander uses CMake with Ninja Multi-Config. Dependencies are fetched
automatically via FetchContent.

```sh
cmake --preset <preset-name>
cmake --build build-<preset-name> --config Release
ctest --test-dir build-<preset-name> -C Release --output-on-failure
```

Presets are defined per-compiler in `CMakeUserPresets.json` (auto-generated
from `compilers.json`). The base hidden preset lives in `CMakePresets.json`.

### Code Formatting

Formatting is enforced via [pre-commit](https://pre-commit.com/) (clang-format, LLVM style):

```sh
pre-commit run --all-files
```

### Compiler Requirements

C++20. CI-tested with GCC 12--14, Clang 16--18, and Apple Clang (Xcode 15,
ARM).

### Build Configurations

| Configuration  | Flags                                            |
|----------------|--------------------------------------------------|
| Release        | `-g -DNDEBUG -O3`                                |
| RelWithDebInfo | `-fsanitize=undefined -fsanitize=address -g -O3` |

All builds use `-Wall -Wextra -pedantic -Werror`.

## Installation

```sh
cmake --install build-<preset-name> --config Release --prefix /usr/local
```

This installs:

| Component                                                      | Destination                 |
|----------------------------------------------------------------|-----------------------------|
| Library headers                                                | `include/json_commander/`   |
| `json-commander` CLI tool                                      | `bin/`                      |
| JSON schemas                                                   | `share/json_commander/`     |
| CMake config files (including `json_commander_add_executable`) | `lib/cmake/json-commander/` |

Downstream projects consume the library via `find_package`:

```cmake
find_package(json-commander REQUIRED)

# Use the executable helper ...
json_commander_add_executable(my_tool
  SCHEMA my_tool.json
  MAIN my_tool::run
  FROM_HEADER my_tool_main.hpp)

# ... or link directly
target_link_libraries(my_app PRIVATE
  json_commander::header
  json_commander::library)
```

## Dependencies

Managed via FetchContent through the `cmake_utilities` submodule:

| Dependency                                                                          | Purpose                         |
|-------------------------------------------------------------------------------------|---------------------------------|
| [nlohmann/json](https://github.com/nlohmann/json) v3.10.0                           | JSON parsing and representation |
| [nlohmann/json-schema-validator](https://github.com/pboettch/json-schema-validator) | JSON Schema validation          |
| [Catch2](https://github.com/catchorg/Catch2)                                        | Testing framework               |

## Project Structure

```
json_commander/            Library headers (C++)
  schema/                  JSON-Commander metaschema (JSON Schema)
  config.hpp.in            Generated config (version, schema paths, compiler info)
  model.hpp                C++ data model (Root, Command, Argument, ...)
  model_json.hpp           JSON serialization/deserialization
  schema_loader.hpp        Schema validation and loading
  conv.hpp                 String-to-JSON type converters
  validate.hpp             Constraint validators (required, must_exist, ...)
  arg.hpp                  Compiled argument specifications
  cmd.hpp                  Command/subcommand compilation
  parse.hpp                Argument parsing engine
  run.hpp                  Simplified run() entry point
  manpage.hpp              Man page and help text generation
  config_schema.hpp        Runtime config JSON Schema generation
json_commander_c/          C API shared library
  json_commander.h         Public C header (jcmd_run)
  json_commander.cpp       C API implementation
json_commander_testing/    Test sources (Catch2)
examples/
  greet/                   Simple flag + positional example (C++)
  serve/                   Schema-driven server example (uses json_commander_add_executable)
  greet-c/                 Inline schema example (C API)
  serve-c/                 File-loaded schema example (C API)
  fake-git/                Nested subcommands example (modeled after git)
tools/
  json_commander.cpp       Unified CLI tool
  json_commander.json      CLI tool's own schema (self-hosting)
.github/workflows/         CI configuration
```

## Architecture

JSON-Commander follows a pipeline design:

```
JSON Schema  -->  model::Root  -->  cmd::RootSpec  -->  parse::parse()  -->  JSON config
                       |
                       +-->  manpage::to_groff() / to_plain_text()
                       +-->  config_schema::to_config_schema()
```

1. **Model** (`model.hpp`) -- C++ types mirroring the JSON schema: `Root`,
   `Command`, `Argument` (variant of `Flag`, `Option`, `Positional`,
   `FlagGroup`), `TypeSpec`, and documentation types.

2. **Schema Loader** (`schema_loader.hpp`) -- validates a JSON document
   against the json-commander metaschema and deserializes it into `model::Root`.

3. **Converters** (`conv.hpp`) -- bidirectional string-to-JSON converters
   for scalar types (string, int, float, bool, enum, file, dir, path) and
   compound types (list, pair, triple).

4. **Validators** (`validate.hpp`) -- constraint checkers (required,
   must_exist) composed via `all_of`.

5. **Arg/Cmd** (`arg.hpp`, `cmd.hpp`) -- compile model types into
   parsing-ready specifications with resolved defaults, bundled converters,
   and validators.

6. **Parser** (`parse.hpp`) -- consumes compiled specs and CLI tokens,
   produces `ParseResult` (variant of `ParseOk`, `HelpRequest`,
   `ManpageRequest`, `VersionRequest`).

7. **Man page** (`manpage.hpp`) -- assembles man page sections from model
   types, renders to groff or plain text.

8. **Config schema** (`config_schema.hpp`) -- generates a JSON Schema
   describing the runtime configuration that `parse::parse` produces.

9. **Run** (`run.hpp`) -- simplified entry point that handles schema
   loading, parsing, and result dispatch (`--help`, `--version`, `--man`)
   in a single `run()` call.

10. **C API** (`json_commander_c/`) -- shared library exposing `jcmd_run()`,
    a single C function that wraps the full pipeline for use from C or FFI.

## License

See [LICENSE](LICENSE) for details.
