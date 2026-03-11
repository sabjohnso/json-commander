#pragma once

#include <filesystem>
#include <fstream>
#include <json_commander/metaschema_data.hpp>
#include <json_commander/model_json.hpp>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <set>
#include <stdexcept>
#include <string>

namespace json_commander::schema {

  class Error : public std::runtime_error {
  public:
    explicit Error(const std::string& message)
        : std::runtime_error(message) {}
  };

  namespace detail {

    inline void
    validate_ref_path(
      const std::filesystem::path& ref_path,
      const std::filesystem::path& base_dir) {
      auto canonical_ref = std::filesystem::weakly_canonical(ref_path);
      auto canonical_base = std::filesystem::weakly_canonical(base_dir);
      auto base_str = canonical_base.string() + std::string(1, '/');
      auto ref_str = canonical_ref.string();
      if (
        ref_str.rfind(base_str, 0) != 0 && ref_str != canonical_base.string()) {
        throw Error(
          "external command file escapes base directory: " + ref_path.string());
      }
    }

    using VisitedSet = std::set<std::string>;

    inline void
    resolve_external_refs(
      nlohmann::json& j,
      const std::filesystem::path& base_dir,
      VisitedSet& visited) {
      if (!j.is_object() || !j.contains("commands")) { return; }

      auto& commands = j["commands"];
      if (!commands.is_array()) { return; }

      for (auto& entry : commands) {
        if (entry.is_string()) {
          auto ref_path = base_dir / entry.get<std::string>();
          validate_ref_path(ref_path, base_dir);

          auto canonical = std::filesystem::weakly_canonical(ref_path).string();
          if (visited.count(canonical)) {
            throw Error("circular external reference: " + ref_path.string());
          }
          visited.insert(canonical);

          std::ifstream f(ref_path);
          if (!f.is_open()) {
            throw Error(
              "failed to open external command file: " + ref_path.string());
          }
          try {
            entry = nlohmann::json::parse(f);
          } catch (const nlohmann::json::parse_error& e) {
            throw Error(
              "failed to parse external command file: " + ref_path.string() +
              ": " + e.what());
          }
          // Recurse into the loaded command (it may have its own external refs)
          resolve_external_refs(entry, ref_path.parent_path(), visited);
        } else if (entry.is_object()) {
          resolve_external_refs(entry, base_dir, visited);
        }
      }
    }

  } // namespace detail

  class Loader {
  public:
    Loader() {
      auto metaschema = nlohmann::json::parse(detail::metaschema_json);
      validator_.set_root_schema(metaschema);
    }

    model::Root
    load(const nlohmann::json& j) const {
      try {
        validator_.validate(j);
      } catch (const std::exception& e) { throw Error(e.what()); }
      return j.get<model::Root>();
    }

    model::Root
    load(const std::string& path) const {
      std::ifstream f(path);
      if (!f.is_open()) { throw Error("failed to open file: " + path); }
      nlohmann::json j;
      try {
        j = nlohmann::json::parse(f);
      } catch (const nlohmann::json::parse_error& e) {
        throw Error("failed to parse JSON: " + path + ": " + e.what());
      }
      auto base_dir = std::filesystem::path(path).parent_path();
      if (base_dir.empty()) { base_dir = "."; }
      detail::VisitedSet visited;
      detail::resolve_external_refs(j, base_dir, visited);
      return load(j);
    }

  private:
    nlohmann::json_schema::json_validator validator_;
  };

} // namespace json_commander::schema
