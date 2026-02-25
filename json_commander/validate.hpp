#pragma once

#include <json_commander/model.hpp>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace json_commander::validate {

  class Error : public std::runtime_error {
  public:
    explicit Error(const std::string& message)
        : std::runtime_error(message) {}
  };

  struct Validator {
    std::function<void(
      const std::string& name, const std::optional<nlohmann::json>& value)>
      check;
    std::string description;
  };

  // -------------------------------------------------------------------------
  // Constraint validators
  // -------------------------------------------------------------------------

  inline Validator
  required() {
    return {
      [](const std::string& name, const std::optional<nlohmann::json>& value) {
        if (!value.has_value()) { throw Error(name + " is required"); }
      },
      "required",
    };
  }

  inline Validator
  must_exist_file() {
    return {
      [](const std::string& name, const std::optional<nlohmann::json>& value) {
        if (!value.has_value()) { return; }
        auto path = value->get<std::string>();
        if (!std::filesystem::is_regular_file(path)) {
          throw Error(name + ": " + path + " is not a regular file");
        }
      },
      "must_exist(file)",
    };
  }

  inline Validator
  must_exist_dir() {
    return {
      [](const std::string& name, const std::optional<nlohmann::json>& value) {
        if (!value.has_value()) { return; }
        auto path = value->get<std::string>();
        if (!std::filesystem::is_directory(path)) {
          throw Error(name + ": " + path + " is not a directory");
        }
      },
      "must_exist(dir)",
    };
  }

  inline Validator
  must_exist_path() {
    return {
      [](const std::string& name, const std::optional<nlohmann::json>& value) {
        if (!value.has_value()) { return; }
        auto path = value->get<std::string>();
        if (!std::filesystem::exists(path)) {
          throw Error(name + ": " + path + " does not exist");
        }
      },
      "must_exist(path)",
    };
  }

  // -------------------------------------------------------------------------
  // Composition
  // -------------------------------------------------------------------------

  inline Validator
  all_of(std::vector<Validator> validators) {
    if (validators.empty()) {
      return {
        [](const std::string&, const std::optional<nlohmann::json>&) {},
        "none",
      };
    }
    std::string desc;
    for (std::size_t i = 0; i < validators.size(); ++i) {
      if (i > 0) { desc += " + "; }
      desc += validators[i].description;
    }
    return {
      [validators = std::move(validators)](
        const std::string& name, const std::optional<nlohmann::json>& value) {
        for (const auto& v : validators) {
          v.check(name, value);
        }
      },
      std::move(desc),
    };
  }

  // -------------------------------------------------------------------------
  // Detail: type-aware must_exist helpers
  // -------------------------------------------------------------------------

  namespace detail {

    inline bool
    is_filesystem_type(model::ScalarType t) {
      return t == model::ScalarType::File || t == model::ScalarType::Dir ||
             t == model::ScalarType::Path;
    }

    inline std::optional<Validator>
    must_exist_for_scalar(model::ScalarType t) {
      switch (t) {
        case model::ScalarType::File:
          return must_exist_file();
        case model::ScalarType::Dir:
          return must_exist_dir();
        case model::ScalarType::Path:
          return must_exist_path();
        default:
          return std::nullopt;
      }
    }

    inline std::optional<Validator>
    must_exist_for_list(const model::ListType& lt) {
      auto scalar_v = must_exist_for_scalar(lt.element);
      if (!scalar_v.has_value()) { return std::nullopt; }
      auto inner = std::move(*scalar_v);
      return Validator{
        [inner](
          const std::string& name, const std::optional<nlohmann::json>& value) {
          if (!value.has_value()) { return; }
          for (std::size_t i = 0; i < value->size(); ++i) {
            auto elem = std::optional<nlohmann::json>((*value)[i]);
            inner.check(name + "[" + std::to_string(i) + "]", elem);
          }
        },
        inner.description,
      };
    }

    inline void
    check_element_at(
      const std::string& name,
      const nlohmann::json& arr,
      std::size_t index,
      model::ScalarType type) {
      auto v = must_exist_for_scalar(type);
      if (v.has_value()) {
        auto elem = std::optional<nlohmann::json>(arr[index]);
        v->check(name + "[" + std::to_string(index) + "]", elem);
      }
    }

    inline std::optional<Validator>
    must_exist_for_pair(const model::PairType& pt) {
      bool has_fs =
        is_filesystem_type(pt.first) || is_filesystem_type(pt.second);
      if (!has_fs) { return std::nullopt; }
      return Validator{
        [pt](
          const std::string& name, const std::optional<nlohmann::json>& value) {
          if (!value.has_value()) { return; }
          check_element_at(name, *value, 0, pt.first);
          check_element_at(name, *value, 1, pt.second);
        },
        "must_exist(pair)",
      };
    }

    inline std::optional<Validator>
    must_exist_for_triple(const model::TripleType& tt) {
      bool has_fs = is_filesystem_type(tt.first) ||
                    is_filesystem_type(tt.second) ||
                    is_filesystem_type(tt.third);
      if (!has_fs) { return std::nullopt; }
      return Validator{
        [tt](
          const std::string& name, const std::optional<nlohmann::json>& value) {
          if (!value.has_value()) { return; }
          check_element_at(name, *value, 0, tt.first);
          check_element_at(name, *value, 1, tt.second);
          check_element_at(name, *value, 2, tt.third);
        },
        "must_exist(triple)",
      };
    }

    inline std::optional<Validator>
    must_exist_for_type(const model::TypeSpec& spec) {
      return std::visit(
        [](const auto& t) -> std::optional<Validator> {
          using T = std::decay_t<decltype(t)>;
          if constexpr (std::is_same_v<T, model::ScalarType>) {
            return must_exist_for_scalar(t);
          } else if constexpr (std::is_same_v<T, model::ListType>) {
            return must_exist_for_list(t);
          } else if constexpr (std::is_same_v<T, model::PairType>) {
            return must_exist_for_pair(t);
          } else if constexpr (std::is_same_v<T, model::TripleType>) {
            return must_exist_for_triple(t);
          }
        },
        spec);
    }

  } // namespace detail

  // -------------------------------------------------------------------------
  // Factory functions
  // -------------------------------------------------------------------------

  inline Validator
  from_option(const model::Option& opt) {
    std::vector<Validator> parts;

    if (opt.required.value_or(false)) { parts.push_back(required()); }

    if (opt.must_exist.value_or(false)) {
      auto v = detail::must_exist_for_type(opt.type);
      if (v.has_value()) { parts.push_back(std::move(*v)); }
    }

    return all_of(std::move(parts));
  }

  inline Validator
  from_positional(const model::Positional& pos) {
    std::vector<Validator> parts;

    if (pos.required.value_or(false)) { parts.push_back(required()); }

    if (pos.must_exist.value_or(false)) {
      auto v = detail::must_exist_for_type(pos.type);
      if (v.has_value()) { parts.push_back(std::move(*v)); }
    }

    return all_of(std::move(parts));
  }

} // namespace json_commander::validate
