#pragma once

#include <json_commander/model.hpp>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>

namespace json_commander::model_emit {

  // ---------------------------------------------------------------------------
  // String escaping
  // ---------------------------------------------------------------------------

  namespace detail {

    inline std::string
    escape_cpp_string(const std::string& s) {
      std::string result;
      result.reserve(s.size() + 8);
      for (char c : s) {
        switch (c) {
          case '\\':
            result += "\\\\";
            break;
          case '"':
            result += "\\\"";
            break;
          case '\n':
            result += "\\n";
            break;
          case '\r':
            result += "\\r";
            break;
          case '\t':
            result += "\\t";
            break;
          default:
            result += c;
        }
      }
      return result;
    }

    inline std::string
    quoted(const std::string& s) {
      return "\"" + escape_cpp_string(s) + "\"";
    }

    // -------------------------------------------------------------------------
    // JSON literal emission
    // -------------------------------------------------------------------------

    inline std::string
    emit_json(const nlohmann::json& j) {
      if (j.is_null()) { return "nlohmann::json(nullptr)"; }
      if (j.is_boolean()) {
        return j.get<bool>() ? "nlohmann::json(true)" : "nlohmann::json(false)";
      }
      if (j.is_number_integer()) {
        return "nlohmann::json(" + std::to_string(j.get<int64_t>()) + ")";
      }
      if (j.is_number_unsigned()) {
        return "nlohmann::json(" + std::to_string(j.get<uint64_t>()) + "u)";
      }
      if (j.is_number_float()) {
        std::ostringstream oss;
        oss << std::setprecision(17) << j.get<double>();
        return "nlohmann::json(" + oss.str() + ")";
      }
      if (j.is_string()) {
        return "nlohmann::json(" + quoted(j.get<std::string>()) + ")";
      }
      if (j.is_array()) {
        std::string result = "nlohmann::json::array({";
        bool first = true;
        for (const auto& elem : j) {
          if (!first) { result += ", "; }
          result += emit_json(elem);
          first = false;
        }
        result += "})";
        return result;
      }
      if (j.is_object()) {
        std::string result = "nlohmann::json::object({";
        bool first = true;
        for (const auto& [k, v] : j.items()) {
          if (!first) { result += ", "; }
          result += "{" + quoted(k) + ", " + emit_json(v) + "}";
          first = false;
        }
        result += "})";
        return result;
      }
      return "nlohmann::json()";
    }

    // -------------------------------------------------------------------------
    // Indentation helper
    // -------------------------------------------------------------------------

    struct Emitter {
      std::ostringstream out;
      int indent = 0;

      std::string
      pad() const {
        return std::string(static_cast<size_t>(indent * 2), ' ');
      }

      void
      line(const std::string& s) {
        out << pad() << s << "\n";
      }

      // -----------------------------------------------------------------------
      // Scalar types
      // -----------------------------------------------------------------------

      std::string
      emit_scalar_type(model::ScalarType s) {
        switch (s) {
          case model::ScalarType::String:
            return "ScalarType::String";
          case model::ScalarType::Int:
            return "ScalarType::Int";
          case model::ScalarType::Float:
            return "ScalarType::Float";
          case model::ScalarType::Bool:
            return "ScalarType::Bool";
          case model::ScalarType::Enum:
            return "ScalarType::Enum";
          case model::ScalarType::File:
            return "ScalarType::File";
          case model::ScalarType::Dir:
            return "ScalarType::Dir";
          case model::ScalarType::Path:
            return "ScalarType::Path";
        }
        return "ScalarType::String";
      }

      // -----------------------------------------------------------------------
      // Compound types
      // -----------------------------------------------------------------------

      std::string
      emit_list_type(const model::ListType& lt) {
        std::string result =
          "ListType{.element = " + emit_scalar_type(lt.element);
        if (lt.separator) {
          result += ", .separator = " + quoted(*lt.separator);
        }
        result += "}";
        return result;
      }

      std::string
      emit_pair_type(const model::PairType& pt) {
        std::string result = "PairType{.first = " + emit_scalar_type(pt.first);
        result += ", .second = " + emit_scalar_type(pt.second);
        if (pt.separator) {
          result += ", .separator = " + quoted(*pt.separator);
        }
        result += "}";
        return result;
      }

      std::string
      emit_triple_type(const model::TripleType& tt) {
        std::string result =
          "TripleType{.first = " + emit_scalar_type(tt.first);
        result += ", .second = " + emit_scalar_type(tt.second);
        result += ", .third = " + emit_scalar_type(tt.third);
        if (tt.separator) {
          result += ", .separator = " + quoted(*tt.separator);
        }
        result += "}";
        return result;
      }

      std::string
      emit_type_spec(const model::TypeSpec& ts) {
        return std::visit(
          [this](const auto& v) -> std::string {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, model::ScalarType>) {
              return emit_scalar_type(v);
            } else if constexpr (std::is_same_v<T, model::ListType>) {
              return emit_list_type(v);
            } else if constexpr (std::is_same_v<T, model::PairType>) {
              return emit_pair_type(v);
            } else {
              return emit_triple_type(v);
            }
          },
          ts);
      }

      // -----------------------------------------------------------------------
      // String collections
      // -----------------------------------------------------------------------

      std::string
      emit_doc_string(const model::DocString& ds) {
        std::string result = "{";
        bool first = true;
        for (const auto& s : ds) {
          if (!first) { result += ", "; }
          result += quoted(s);
          first = false;
        }
        result += "}";
        return result;
      }

      std::string
      emit_arg_names(const model::ArgNames& names) {
        return emit_doc_string(names); // same structure
      }

      // -----------------------------------------------------------------------
      // Env types
      // -----------------------------------------------------------------------

      std::string
      emit_env_binding_obj(const model::EnvBindingObj& e) {
        std::string result = "EnvBindingObj{.var = " + quoted(e.var);
        if (e.doc) { result += ", .doc = " + emit_doc_string(*e.doc); }
        result += "}";
        return result;
      }

      std::string
      emit_env_binding(const model::EnvBinding& eb) {
        return std::visit(
          [this](const auto& v) -> std::string {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::string>) {
              return "EnvBinding(" + quoted(v) + ")";
            } else {
              return "EnvBinding(" + emit_env_binding_obj(v) + ")";
            }
          },
          eb);
      }

      // -----------------------------------------------------------------------
      // Optional emission helpers
      // -----------------------------------------------------------------------

      template <typename T>
      std::string
      emit_optional(
        const std::optional<T>& opt, std::function<std::string(const T&)> fn) {
        if (opt) { return fn(*opt); }
        return "std::nullopt";
      }

      std::string
      emit_opt_string(const std::optional<std::string>& opt) {
        return emit_optional<std::string>(
          opt, [](const std::string& s) { return quoted(s); });
      }

      std::string
      emit_opt_bool(const std::optional<bool>& opt) {
        if (opt) { return *opt ? "true" : "false"; }
        return "std::nullopt";
      }

      std::string
      emit_opt_int(const std::optional<int>& opt) {
        if (opt) { return std::to_string(*opt); }
        return "std::nullopt";
      }

      std::string
      emit_opt_json(const std::optional<nlohmann::json>& opt) {
        if (opt) { return emit_json(*opt); }
        return "std::nullopt";
      }

      std::string
      emit_opt_doc_string(const std::optional<model::DocString>& opt) {
        if (opt) { return emit_doc_string(*opt); }
        return "std::nullopt";
      }

      std::string
      emit_opt_env_binding(const std::optional<model::EnvBinding>& opt) {
        if (opt) { return emit_env_binding(*opt); }
        return "std::nullopt";
      }

      std::string
      emit_opt_choices(const std::optional<std::vector<std::string>>& opt) {
        if (opt) { return "std::vector<std::string>" + emit_doc_string(*opt); }
        return "std::nullopt";
      }

      std::string
      emit_env_info(const model::EnvInfo& e) {
        return "EnvInfo{.var = " + quoted(e.var) +
               ", .doc = " + emit_opt_doc_string(e.doc) + "}";
      }

      std::string
      emit_exit_info(const model::ExitInfo& e) {
        return "ExitInfo{.code = " + std::to_string(e.code) +
               ", .max = " + emit_opt_int(e.max) +
               ", .doc = " + emit_doc_string(e.doc) + "}";
      }

      // -----------------------------------------------------------------------
      // Argument types (all fields emitted to satisfy
      // -Wmissing-field-initializers)
      // -----------------------------------------------------------------------

      std::string
      emit_flag(const model::Flag& f) {
        std::string result = "Flag{\n";
        ++indent;
        result += pad() + ".names = " + emit_arg_names(f.names) + ",\n";
        result += pad() + ".doc = " + emit_doc_string(f.doc) + ",\n";
        result += pad() + ".dest = " + emit_opt_string(f.dest) + ",\n";
        result += pad() + ".env = " + emit_opt_env_binding(f.env) + ",\n";
        result += pad() + ".repeated = " + emit_opt_bool(f.repeated) + ",\n";
        result +=
          pad() + ".deprecated = " + emit_opt_string(f.deprecated) + ",\n";
        result += pad() + ".docs = " + emit_opt_string(f.docs) + ",\n";
        --indent;
        result += pad() + "}";
        return result;
      }

      std::string
      emit_flag_group_entry(const model::FlagGroupEntry& e) {
        std::string result = "FlagGroupEntry{\n";
        ++indent;
        result += pad() + ".names = " + emit_arg_names(e.names) + ",\n";
        result += pad() + ".doc = " + emit_doc_string(e.doc) + ",\n";
        result += pad() + ".value = " + emit_json(e.value) + ",\n";
        --indent;
        result += pad() + "}";
        return result;
      }

      std::string
      emit_flag_group(const model::FlagGroup& fg) {
        std::string result = "FlagGroup{\n";
        ++indent;
        result += pad() + ".dest = " + quoted(fg.dest) + ",\n";
        result += pad() + ".doc = " + emit_doc_string(fg.doc) + ",\n";
        result +=
          pad() + ".default_value = " + emit_json(fg.default_value) + ",\n";
        result += pad() + ".flags = {\n";
        ++indent;
        for (const auto& e : fg.flags) {
          result += pad() + emit_flag_group_entry(e) + ",\n";
        }
        --indent;
        result += pad() + "},\n";
        result += pad() + ".repeated = " + emit_opt_bool(fg.repeated) + ",\n";
        result += pad() + ".docs = " + emit_opt_string(fg.docs) + ",\n";
        --indent;
        result += pad() + "}";
        return result;
      }

      std::string
      emit_option(const model::Option& o) {
        std::string result = "Option{\n";
        ++indent;
        result += pad() + ".names = " + emit_arg_names(o.names) + ",\n";
        result += pad() + ".doc = " + emit_doc_string(o.doc) + ",\n";
        result += pad() + ".type = " + emit_type_spec(o.type) + ",\n";
        result += pad() + ".docv = " + emit_opt_string(o.docv) + ",\n";
        result +=
          pad() + ".default_value = " + emit_opt_json(o.default_value) + ",\n";
        result += pad() + ".required = " + emit_opt_bool(o.required) + ",\n";
        result += pad() + ".repeated = " + emit_opt_bool(o.repeated) + ",\n";
        result += pad() + ".choices = " + emit_opt_choices(o.choices) + ",\n";
        result +=
          pad() + ".must_exist = " + emit_opt_bool(o.must_exist) + ",\n";
        result += pad() + ".dest = " + emit_opt_string(o.dest) + ",\n";
        result += pad() + ".env = " + emit_opt_env_binding(o.env) + ",\n";
        result += pad() + ".docs = " + emit_opt_string(o.docs) + ",\n";
        --indent;
        result += pad() + "}";
        return result;
      }

      std::string
      emit_positional(const model::Positional& p) {
        std::string result = "Positional{\n";
        ++indent;
        result += pad() + ".name = " + quoted(p.name) + ",\n";
        result += pad() + ".doc = " + emit_doc_string(p.doc) + ",\n";
        result += pad() + ".type = " + emit_type_spec(p.type) + ",\n";
        result += pad() + ".docv = " + emit_opt_string(p.docv) + ",\n";
        result +=
          pad() + ".default_value = " + emit_opt_json(p.default_value) + ",\n";
        result += pad() + ".required = " + emit_opt_bool(p.required) + ",\n";
        result += pad() + ".repeated = " + emit_opt_bool(p.repeated) + ",\n";
        result +=
          pad() + ".must_exist = " + emit_opt_bool(p.must_exist) + ",\n";
        result += pad() + ".docs = " + emit_opt_string(p.docs) + ",\n";
        --indent;
        result += pad() + "}";
        return result;
      }

      std::string
      emit_argument(const model::Argument& a) {
        return std::visit(
          [this](const auto& v) -> std::string {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, model::Flag>) {
              return "Argument(" + emit_flag(v) + ")";
            } else if constexpr (std::is_same_v<T, model::FlagGroup>) {
              return "Argument(" + emit_flag_group(v) + ")";
            } else if constexpr (std::is_same_v<T, model::Option>) {
              return "Argument(" + emit_option(v) + ")";
            } else {
              return "Argument(" + emit_positional(v) + ")";
            }
          },
          a);
      }

      // -----------------------------------------------------------------------
      // Man page types
      // -----------------------------------------------------------------------

      std::string
      emit_man_block(const model::ManBlock& mb) {
        return std::visit(
          [this](const auto& v) -> std::string {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, model::ParagraphBlock>) {
              return "ManBlock(ParagraphBlock{.paragraph = " +
                     emit_doc_string(v.paragraph) + "})";
            } else if constexpr (std::is_same_v<T, model::PreBlock>) {
              return "ManBlock(PreBlock{.pre = " + emit_doc_string(v.pre) +
                     "})";
            } else if constexpr (std::is_same_v<T, model::LabelTextBlock>) {
              return "ManBlock(LabelTextBlock{.label = " + quoted(v.label) +
                     ", .text = " + emit_doc_string(v.text) + "})";
            } else {
              return "ManBlock(NoBlankBlock{})";
            }
          },
          mb);
      }

      std::string
      emit_man_section(const model::ManSection& s) {
        std::string result = "ManSection{\n";
        ++indent;
        result += pad() + ".name = " + quoted(s.name) + ",\n";
        result += pad() + ".blocks = {\n";
        ++indent;
        for (const auto& b : s.blocks) {
          result += pad() + emit_man_block(b) + ",\n";
        }
        --indent;
        result += pad() + "},\n";
        result += pad() + ".after = " + emit_opt_string(s.after) + ",\n";
        --indent;
        result += pad() + "}";
        return result;
      }

      std::string
      emit_man_xref(const model::ManXref& x) {
        return "ManXref{.name = " + quoted(x.name) +
               ", .section = " + std::to_string(x.section) + "}";
      }

      std::string
      emit_man(const model::Man& m) {
        std::string result = "Man{\n";
        ++indent;
        result += pad() + ".section = " + emit_opt_int(m.section) + ",\n";
        if (m.sections) {
          result += pad() + ".sections = std::vector<ManSection>{\n";
          ++indent;
          for (const auto& s : *m.sections) {
            result += pad() + emit_man_section(s) + ",\n";
          }
          --indent;
          result += pad() + "},\n";
        } else {
          result += pad() + ".sections = std::nullopt,\n";
        }
        if (m.xrefs) {
          result += pad() + ".xrefs = std::vector<ManXref>{\n";
          ++indent;
          for (const auto& x : *m.xrefs) {
            result += pad() + emit_man_xref(x) + ",\n";
          }
          --indent;
          result += pad() + "},\n";
        } else {
          result += pad() + ".xrefs = std::nullopt,\n";
        }
        --indent;
        result += pad() + "}";
        return result;
      }

      // -----------------------------------------------------------------------
      // Config types
      // -----------------------------------------------------------------------

      std::string
      emit_config_paths(const model::ConfigPaths& cp) {
        return "ConfigPaths{.system = " + emit_opt_string(cp.system) +
               ", .user = " + emit_opt_string(cp.user) +
               ", .local = " + emit_opt_string(cp.local) + "}";
      }

      std::string
      emit_config(const model::Config& c) {
        std::string result = "Config{.format = " + quoted(c.format);
        if (c.paths) {
          result += ", .paths = " + emit_config_paths(*c.paths);
        } else {
          result += ", .paths = std::nullopt";
        }
        result += "}";
        return result;
      }

      // -----------------------------------------------------------------------
      // Command and Root
      // -----------------------------------------------------------------------

      std::string
      emit_args_vector(const std::vector<model::Argument>& args) {
        std::string result = "std::vector<Argument>{\n";
        ++indent;
        for (const auto& a : args) {
          result += pad() + emit_argument(a) + ",\n";
        }
        --indent;
        result += pad() + "}";
        return result;
      }

      std::string
      emit_opt_args(const std::optional<std::vector<model::Argument>>& args) {
        if (args) { return emit_args_vector(*args); }
        return "std::nullopt";
      }

      std::string
      emit_opt_commands(
        const std::optional<std::vector<model::Command>>& cmds) {
        if (!cmds) { return "std::nullopt"; }
        std::string result = "std::vector<Command>{\n";
        ++indent;
        for (const auto& c : *cmds) {
          result += pad() + emit_command(c) + ",\n";
        }
        --indent;
        result += pad() + "}";
        return result;
      }

      std::string
      emit_opt_man(const std::optional<model::Man>& m) {
        if (m) { return emit_man(*m); }
        return "std::nullopt";
      }

      std::string
      emit_opt_envs(const std::optional<std::vector<model::EnvInfo>>& envs) {
        if (!envs) { return "std::nullopt"; }
        std::string result = "std::vector<EnvInfo>{\n";
        ++indent;
        for (const auto& e : *envs) {
          result += pad() + emit_env_info(e) + ",\n";
        }
        --indent;
        result += pad() + "}";
        return result;
      }

      std::string
      emit_opt_exits(const std::optional<std::vector<model::ExitInfo>>& exits) {
        if (!exits) { return "std::nullopt"; }
        std::string result = "std::vector<ExitInfo>{\n";
        ++indent;
        for (const auto& e : *exits) {
          result += pad() + emit_exit_info(e) + ",\n";
        }
        --indent;
        result += pad() + "}";
        return result;
      }

      std::string
      emit_opt_config(const std::optional<model::Config>& c) {
        if (c) { return emit_config(*c); }
        return "std::nullopt";
      }

      std::string
      emit_command(const model::Command& cmd) {
        std::string result = "Command{\n";
        ++indent;
        result += pad() + ".name = " + quoted(cmd.name) + ",\n";
        result += pad() + ".doc = " + emit_doc_string(cmd.doc) + ",\n";
        result += pad() + ".args = " + emit_opt_args(cmd.args) + ",\n";
        result +=
          pad() + ".commands = " + emit_opt_commands(cmd.commands) + ",\n";
        result += pad() + ".man = " + emit_opt_man(cmd.man) + ",\n";
        result += pad() + ".envs = " + emit_opt_envs(cmd.envs) + ",\n";
        result += pad() + ".exits = " + emit_opt_exits(cmd.exits) + ",\n";
        --indent;
        result += pad() + "}";
        return result;
      }

      std::string
      emit_root(const model::Root& root) {
        std::string result = "Root{\n";
        ++indent;
        result += pad() + ".name = " + quoted(root.name) + ",\n";
        result += pad() + ".doc = " + emit_doc_string(root.doc) + ",\n";
        result += pad() + ".args = " + emit_opt_args(root.args) + ",\n";
        result +=
          pad() + ".commands = " + emit_opt_commands(root.commands) + ",\n";
        result += pad() + ".man = " + emit_opt_man(root.man) + ",\n";
        result += pad() + ".envs = " + emit_opt_envs(root.envs) + ",\n";
        result += pad() + ".exits = " + emit_opt_exits(root.exits) + ",\n";
        result += pad() + ".version = " + emit_opt_string(root.version) + ",\n";
        result += pad() + ".config = " + emit_opt_config(root.config) + ",\n";
        --indent;
        result += pad() + "}";
        return result;
      }
    };

  } // namespace detail

  // ---------------------------------------------------------------------------
  // Public API
  // ---------------------------------------------------------------------------

  inline std::string
  emit_model_hpp(const model::Root& root, const std::string& fn_name) {
    detail::Emitter emitter;
    emitter.indent = 1;
    auto root_expr = emitter.emit_root(root);

    std::ostringstream out;
    out << "// Generated by json-commander codegen — do not edit.\n";
    out << "#pragma once\n\n";
    out << "#include <json_commander/model.hpp>\n";
    out << "#include <nlohmann/json.hpp>\n\n";
    out << "inline json_commander::model::Root\n";
    out << fn_name << "() {\n";
    out << "  using namespace json_commander::model;\n";
    out << "  return " << root_expr << ";\n";
    out << "}\n";

    return out.str();
  }

} // namespace json_commander::model_emit
