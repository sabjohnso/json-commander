# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

JSON-Commander is a C++20 header-only library for defining CLI interfaces via JSON schemas. It takes a declarative approach inspired by OCaml's cmdliner: describe your CLI as JSON, and the library handles argument parsing, env var fallback, config merging, man page generation, and unified JSON config output.

## Build Commands

The project uses CMake with Ninja Multi-Config. Dependencies (nlohmann/json, json-schema-validator, Catch2) are fetched automatically via FetchContent.

```sh
# Configure (preset names come from CMakeUserPresets.json, generated per-compiler)
cmake --preset <preset-name>

# Build
cmake --build build --config Release

# Run all tests
ctest --test-dir build -C Release --output-on-failure

# Run a single test (test names match module names: metaschema, model, schema_loader, conv, validate, arg, cmd, manpage, parse, config_schema, run)
ctest --test-dir build -C Release --output-on-failure -R <test_name>

# Run a single test executable directly (useful for Catch2 tag filtering)
./build/Release/<test_name>_test "[tag]"

# Reformat all .hpp files in-place (uses clang-format with LLVM-based style)
./scripts/check-format.sh

# CI verifies formatting with: ./scripts/check-format.sh && git diff --exit-code
```

Build configurations: Release (`-O3 -DNDEBUG`) and RelWithDebInfo (adds ASan + UBSan). All builds use `-Wall -Wextra -pedantic -Werror`.

## Architecture

Linear pipeline design:

```
JSON Schema  →  model::Root  →  cmd::RootSpec  →  parse::parse()  →  JSON config
                     |
                     ├→  manpage::to_groff() / to_plain_text()
                     └→  config_schema::to_config_schema()
```

**Pipeline stages** (each in its own header under `json_commander/`):

1. **model.hpp** — C++ data model: `Root`, `Command`, `Argument` (variant of `Flag`|`FlagGroup`|`Option`|`Positional`), `TypeSpec` (variant of `ScalarType`|`ListType`|`PairType`|`TripleType`)
2. **model_json.hpp** — ADL-based `to_json`/`from_json` for nlohmann::json integration
3. **schema_loader.hpp** — Validates JSON against the metaschema (`json_commander/schema/json_commander.schema.json`), deserializes to `model::Root`
4. **conv.hpp** — Bidirectional string↔JSON converters for scalar and compound types
5. **validate.hpp** — Constraint validators (`required`, `must_exist_file/dir/path`), composed via `all_of`
6. **arg.hpp** / **cmd.hpp** — Compile model types into parsing-ready `ArgSpec`/`CommandSpec`/`RootSpec` with resolved defaults, bundled converters, and validators
7. **parse.hpp** — Consumes specs + CLI tokens, produces `ParseResult` (variant: `ParseOk`|`HelpRequest`|`VersionRequest`|`ManpageRequest`)
8. **manpage.hpp** — Renders man page sections to groff or plain text
9. **config_schema.hpp** — Generates JSON Schema (draft 2020-12) describing the runtime config output
10. **run.hpp** — Simplified entry point: `json_commander::run()` handles schema loading, parsing, and dispatch (`--help`, `--version`, `--man`) in a single call
11. **json_commander_c/** — C shared library exposing `jcmd_run()` for embedding in C programs or FFI bindings

## Key Patterns

- **std::variant polymorphism** throughout (Argument, TypeSpec, ParseResult) — use `std::visit` and `std::get_if` to handle variants
- **Factory functions** (`arg::make`, `cmd::make`) compile model types into spec types — this is the "compilation" step
- **Functional core, imperative shell** — model/spec types are pure data; parsing is the imperative boundary
- **Self-hosting** — the `json-commander` CLI tool (`tools/`) uses its own library to define its CLI via `tools/json_commander.json`
- **Namespace-per-module** — `model::`, `arg::`, `cmd::`, `conv::`, `validate::`, `parse::`, `manpage::`, `config_schema::`, `schema::`
- **Custom exceptions** inheriting `std::runtime_error` — `parse::Error`, `conv::Error`, `validate::Error`, `schema::Error`

## Testing

Tests use Catch2 and are organized by module in `json_commander_testing/`. Each test file corresponds to a library header. Tests use Catch2 tags for filtering (e.g., `[parse][phase1]`, `[conv][scalars]`).

## Code Style

- LLVM-based clang-format (`.clang-format`): 100 column limit, all namespace indentation, always break after return type
- camelCase for functions/variables, PascalCase for types
- `std::optional` over nullable pointers
- Structured bindings where appropriate
- Current version: 0.3.0 (semantic versioning)
