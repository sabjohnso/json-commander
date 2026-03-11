#pragma once

#include <json_commander/model.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <charconv>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

namespace json_commander::conv {

  class Error : public std::runtime_error {
  public:
    explicit Error(const std::string& message)
        : std::runtime_error(message) {}
  };

  struct Converter {
    std::function<nlohmann::json(const std::string&)> parse;
    std::function<std::string(const nlohmann::json&)> format;
    std::string docv;
  };

  // -------------------------------------------------------------------------
  // Scalar converters
  // -------------------------------------------------------------------------

  inline Converter
  string_conv() {
    return {
      [](const std::string& s) -> nlohmann::json { return s; },
      [](const nlohmann::json& j) -> std::string {
        return j.get<std::string>();
      },
      "STRING",
    };
  }

  inline Converter
  int_conv() {
    return {
      [](const std::string& s) -> nlohmann::json {
        if (s.empty()) { throw Error("expected integer, got empty string"); }
        int value = 0;
        auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
        if (ec != std::errc{} || ptr != s.data() + s.size()) {
          throw Error("expected integer, got '" + s + "'");
        }
        return value;
      },
      [](const nlohmann::json& j) -> std::string {
        return std::to_string(j.get<int>());
      },
      "INT",
    };
  }

  inline Converter
  float_conv() {
    return {
      [](const std::string& s) -> nlohmann::json {
        if (s.empty()) { throw Error("expected float, got empty string"); }
        std::size_t pos = 0;
        double value = 0;
        try {
          value = std::stod(s, &pos);
        } catch (const std::invalid_argument&) {
          throw Error("expected float, got '" + s + "'");
        } catch (const std::out_of_range&) {
          throw Error("float value out of range: '" + s + "'");
        }
        if (pos != s.size()) { throw Error("expected float, got '" + s + "'"); }
        return value;
      },
      [](const nlohmann::json& j) -> std::string { return j.dump(); },
      "FLOAT",
    };
  }

  inline Converter
  bool_conv() {
    return {
      [](const std::string& s) -> nlohmann::json {
        std::string lower = s;
        std::transform(
          lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
            return std::tolower(c);
          });
        if (lower == "true") return true;
        if (lower == "false") return false;
        throw Error("expected 'true' or 'false', got '" + s + "'");
      },
      [](const nlohmann::json& j) -> std::string {
        return j.get<bool>() ? "true" : "false";
      },
      "BOOL",
    };
  }

  inline Converter
  enum_conv(const std::vector<std::string>& choices) {
    return {
      [choices](const std::string& s) -> nlohmann::json {
        for (const auto& c : choices) {
          if (c == s) return s;
        }
        std::string msg = "invalid choice '" + s + "', expected one of:";
        for (const auto& c : choices) {
          msg += " " + c;
        }
        throw Error(msg);
      },
      [](const nlohmann::json& j) -> std::string {
        return j.get<std::string>();
      },
      "ENUM",
    };
  }

  inline Converter
  file_conv() {
    return {
      [](const std::string& s) -> nlohmann::json { return s; },
      [](const nlohmann::json& j) -> std::string {
        return j.get<std::string>();
      },
      "FILE",
    };
  }

  inline Converter
  dir_conv() {
    return {
      [](const std::string& s) -> nlohmann::json { return s; },
      [](const nlohmann::json& j) -> std::string {
        return j.get<std::string>();
      },
      "DIR",
    };
  }

  inline Converter
  path_conv() {
    return {
      [](const std::string& s) -> nlohmann::json { return s; },
      [](const nlohmann::json& j) -> std::string {
        return j.get<std::string>();
      },
      "PATH",
    };
  }

  // -------------------------------------------------------------------------
  // Detail: string splitting
  // -------------------------------------------------------------------------

  namespace detail {

    inline std::vector<std::string>
    split(const std::string& s, const std::string& sep) {
      if (s.empty()) return {};
      std::vector<std::string> parts;
      std::size_t start = 0;
      while (true) {
        auto pos = s.find(sep, start);
        if (pos == std::string::npos) {
          parts.push_back(s.substr(start));
          break;
        }
        parts.push_back(s.substr(start, pos - start));
        start = pos + sep.size();
      }
      return parts;
    }

  } // namespace detail

  // -------------------------------------------------------------------------
  // Compound converters
  // -------------------------------------------------------------------------

  inline Converter
  list_conv(
    Converter element,
    const std::string& separator = ",",
    std::size_t max_elements = 10000) {
    return {
      [element, separator, max_elements](
        const std::string& s) -> nlohmann::json {
        if (s.empty()) return nlohmann::json::array();
        auto parts = detail::split(s, separator);
        if (parts.size() > max_elements) {
          throw Error(
            "list exceeds maximum element count (" +
            std::to_string(max_elements) + ")");
        }
        auto result = nlohmann::json::array();
        for (const auto& part : parts) {
          result.push_back(element.parse(part));
        }
        return result;
      },
      [element, separator](const nlohmann::json& j) -> std::string {
        std::string result;
        for (std::size_t i = 0; i < j.size(); ++i) {
          if (i > 0) result += separator;
          result += element.format(j[i]);
        }
        return result;
      },
      element.docv + separator + "...",
    };
  }

  inline Converter
  pair_conv(
    Converter first, Converter second, const std::string& separator = ",") {
    return {
      [first, second, separator](const std::string& s) -> nlohmann::json {
        auto pos = s.find(separator);
        if (pos == std::string::npos) {
          throw Error(
            "expected pair separated by '" + separator + "', got '" + s + "'");
        }
        auto a = s.substr(0, pos);
        auto b = s.substr(pos + separator.size());
        auto result = nlohmann::json::array();
        result.push_back(first.parse(a));
        result.push_back(second.parse(b));
        return result;
      },
      [first, second, separator](const nlohmann::json& j) -> std::string {
        return first.format(j[0]) + separator + second.format(j[1]);
      },
      first.docv + separator + second.docv,
    };
  }

  inline Converter
  triple_conv(
    Converter first,
    Converter second,
    Converter third,
    const std::string& separator = ",") {
    return {
      [first, second, third, separator](
        const std::string& s) -> nlohmann::json {
        auto pos1 = s.find(separator);
        if (pos1 == std::string::npos) {
          throw Error(
            "expected triple separated by '" + separator + "', got '" + s +
            "'");
        }
        auto pos2 = s.find(separator, pos1 + separator.size());
        if (pos2 == std::string::npos) {
          throw Error(
            "expected triple separated by '" + separator + "', got '" + s +
            "'");
        }
        auto a = s.substr(0, pos1);
        auto b =
          s.substr(pos1 + separator.size(), pos2 - pos1 - separator.size());
        auto c = s.substr(pos2 + separator.size());
        auto result = nlohmann::json::array();
        result.push_back(first.parse(a));
        result.push_back(second.parse(b));
        result.push_back(third.parse(c));
        return result;
      },
      [first, second, third, separator](
        const nlohmann::json& j) -> std::string {
        return first.format(j[0]) + separator + second.format(j[1]) +
               separator + third.format(j[2]);
      },
      first.docv + separator + second.docv + separator + third.docv,
    };
  }

  // -------------------------------------------------------------------------
  // Factory functions
  // -------------------------------------------------------------------------

  inline Converter
  make(model::ScalarType type) {
    switch (type) {
      case model::ScalarType::String:
        return string_conv();
      case model::ScalarType::Int:
        return int_conv();
      case model::ScalarType::Float:
        return float_conv();
      case model::ScalarType::Bool:
        return bool_conv();
      case model::ScalarType::Enum:
        return string_conv();
      case model::ScalarType::File:
        return file_conv();
      case model::ScalarType::Dir:
        return dir_conv();
      case model::ScalarType::Path:
        return path_conv();
    }
    return string_conv();
  }

  inline Converter
  make(
    const model::TypeSpec& spec,
    const std::optional<std::vector<std::string>>& choices = std::nullopt) {
    return std::visit(
      [&choices](const auto& t) -> Converter {
        using T = std::decay_t<decltype(t)>;
        if constexpr (std::is_same_v<T, model::ScalarType>) {
          if (t == model::ScalarType::Enum && choices.has_value()) {
            return enum_conv(choices.value());
          }
          return make(t);
        } else if constexpr (std::is_same_v<T, model::ListType>) {
          auto sep = t.separator.value_or(",");
          return list_conv(make(t.element), sep);
        } else if constexpr (std::is_same_v<T, model::PairType>) {
          auto sep = t.separator.value_or(",");
          return pair_conv(make(t.first), make(t.second), sep);
        } else if constexpr (std::is_same_v<T, model::TripleType>) {
          auto sep = t.separator.value_or(",");
          return triple_conv(make(t.first), make(t.second), make(t.third), sep);
        }
      },
      spec);
  }

} // namespace json_commander::conv
