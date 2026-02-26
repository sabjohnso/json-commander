#pragma once

#include <fstream>
#include <json_commander/metaschema_data.hpp>
#include <json_commander/model_json.hpp>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace json_commander::schema {

  class Error : public std::runtime_error {
  public:
    explicit Error(const std::string& message)
        : std::runtime_error(message) {}
  };

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
      return load(j);
    }

  private:
    nlohmann::json_schema::json_validator validator_;
  };

} // namespace json_commander::schema
