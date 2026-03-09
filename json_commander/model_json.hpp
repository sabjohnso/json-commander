#pragma once

#include <json_commander/model.hpp>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace json_commander::model {

  // ---------------------------------------------------------------------------
  // Helpers
  // ---------------------------------------------------------------------------

  namespace detail {

    template <typename T>
    void
    set_optional(
      nlohmann::json& j, const char* key, const std::optional<T>& opt) {
      if (opt.has_value()) { j[key] = *opt; }
    }

    template <typename T>
    void
    get_optional(
      const nlohmann::json& j, const char* key, std::optional<T>& opt) {
      if (j.contains(key)) {
        opt = j.at(key).get<T>();
      } else {
        opt = std::nullopt;
      }
    }

  } // namespace detail

  // ---------------------------------------------------------------------------
  // ScalarType
  // ---------------------------------------------------------------------------

  inline void
  to_json(nlohmann::json& j, const ScalarType& s) {
    static const char* names[] = {
      "string", "int", "float", "bool", "enum", "file", "dir", "path"};
    j = names[static_cast<int>(s)];
  }

  inline void
  from_json(const nlohmann::json& j, ScalarType& s) {
    static const std::pair<const char*, ScalarType> table[] = {
      {"string", ScalarType::String},
      {"int", ScalarType::Int},
      {"float", ScalarType::Float},
      {"bool", ScalarType::Bool},
      {"enum", ScalarType::Enum},
      {"file", ScalarType::File},
      {"dir", ScalarType::Dir},
      {"path", ScalarType::Path},
    };
    auto str = j.get<std::string>();
    for (const auto& [name, val] : table) {
      if (str == name) {
        s = val;
        return;
      }
    }
    throw std::invalid_argument("unknown scalar type: " + str);
  }

  // ---------------------------------------------------------------------------
  // ListType
  // ---------------------------------------------------------------------------

  inline void
  to_json(nlohmann::json& j, const ListType& lt) {
    nlohmann::json inner;
    inner["element"] = lt.element;
    detail::set_optional(inner, "separator", lt.separator);
    j = {{"list", inner}};
  }

  inline void
  from_json(const nlohmann::json& j, ListType& lt) {
    const auto& inner = j.at("list");
    inner.at("element").get_to(lt.element);
    detail::get_optional(inner, "separator", lt.separator);
  }

  // ---------------------------------------------------------------------------
  // PairType
  // ---------------------------------------------------------------------------

  inline void
  to_json(nlohmann::json& j, const PairType& pt) {
    nlohmann::json inner;
    inner["first"] = pt.first;
    inner["second"] = pt.second;
    detail::set_optional(inner, "separator", pt.separator);
    j = {{"pair", inner}};
  }

  inline void
  from_json(const nlohmann::json& j, PairType& pt) {
    const auto& inner = j.at("pair");
    inner.at("first").get_to(pt.first);
    inner.at("second").get_to(pt.second);
    detail::get_optional(inner, "separator", pt.separator);
  }

  // ---------------------------------------------------------------------------
  // TripleType
  // ---------------------------------------------------------------------------

  inline void
  to_json(nlohmann::json& j, const TripleType& tt) {
    nlohmann::json inner;
    inner["first"] = tt.first;
    inner["second"] = tt.second;
    inner["third"] = tt.third;
    detail::set_optional(inner, "separator", tt.separator);
    j = {{"triple", inner}};
  }

  inline void
  from_json(const nlohmann::json& j, TripleType& tt) {
    const auto& inner = j.at("triple");
    inner.at("first").get_to(tt.first);
    inner.at("second").get_to(tt.second);
    inner.at("third").get_to(tt.third);
    detail::get_optional(inner, "separator", tt.separator);
  }

  // ---------------------------------------------------------------------------
  // TypeSpec
  // ---------------------------------------------------------------------------

  inline void
  to_json(nlohmann::json& j, const TypeSpec& ts) {
    std::visit([&j](const auto& v) { to_json(j, v); }, ts);
  }

  inline void
  from_json(const nlohmann::json& j, TypeSpec& ts) {
    if (j.is_string()) {
      ScalarType s;
      from_json(j, s);
      ts = s;
    } else if (j.contains("list")) {
      ListType lt;
      from_json(j, lt);
      ts = lt;
    } else if (j.contains("pair")) {
      PairType pt;
      from_json(j, pt);
      ts = pt;
    } else if (j.contains("triple")) {
      TripleType tt;
      from_json(j, tt);
      ts = tt;
    } else {
      throw std::invalid_argument("unknown type_spec format");
    }
  }

  // ---------------------------------------------------------------------------
  // EnvBindingObj
  // ---------------------------------------------------------------------------

  inline void
  to_json(nlohmann::json& j, const EnvBindingObj& e) {
    j = nlohmann::json::object();
    j["var"] = e.var;
    detail::set_optional(j, "doc", e.doc);
  }

  inline void
  from_json(const nlohmann::json& j, EnvBindingObj& e) {
    j.at("var").get_to(e.var);
    detail::get_optional(j, "doc", e.doc);
  }

  // ---------------------------------------------------------------------------
  // EnvBinding
  // ---------------------------------------------------------------------------

  inline void
  to_json(nlohmann::json& j, const EnvBinding& eb) {
    std::visit(
      [&j](const auto& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::string>) {
          j = v;
        } else {
          to_json(j, v);
        }
      },
      eb);
  }

  inline void
  from_json(const nlohmann::json& j, EnvBinding& eb) {
    if (j.is_string()) {
      eb = j.get<std::string>();
    } else {
      EnvBindingObj obj;
      from_json(j, obj);
      eb = obj;
    }
  }

  // ---------------------------------------------------------------------------
  // EnvInfo
  // ---------------------------------------------------------------------------

  inline void
  to_json(nlohmann::json& j, const EnvInfo& e) {
    j = nlohmann::json::object();
    j["var"] = e.var;
    detail::set_optional(j, "doc", e.doc);
  }

  inline void
  from_json(const nlohmann::json& j, EnvInfo& e) {
    j.at("var").get_to(e.var);
    detail::get_optional(j, "doc", e.doc);
  }

  // ---------------------------------------------------------------------------
  // ExitInfo
  // ---------------------------------------------------------------------------

  inline void
  to_json(nlohmann::json& j, const ExitInfo& e) {
    j = nlohmann::json::object();
    j["code"] = e.code;
    detail::set_optional(j, "max", e.max);
    j["doc"] = e.doc;
  }

  inline void
  from_json(const nlohmann::json& j, ExitInfo& e) {
    j.at("code").get_to(e.code);
    detail::get_optional(j, "max", e.max);
    j.at("doc").get_to(e.doc);
  }

  // ---------------------------------------------------------------------------
  // Flag
  // ---------------------------------------------------------------------------

  inline void
  to_json(nlohmann::json& j, const Flag& f) {
    j = nlohmann::json::object();
    j["kind"] = "flag";
    j["names"] = f.names;
    j["doc"] = f.doc;
    detail::set_optional(j, "dest", f.dest);
    detail::set_optional(j, "env", f.env);
    detail::set_optional(j, "repeated", f.repeated);
    detail::set_optional(j, "deprecated", f.deprecated);
    detail::set_optional(j, "docs", f.docs);
  }

  inline void
  from_json(const nlohmann::json& j, Flag& f) {
    j.at("names").get_to(f.names);
    j.at("doc").get_to(f.doc);
    detail::get_optional(j, "dest", f.dest);
    detail::get_optional(j, "env", f.env);
    detail::get_optional(j, "repeated", f.repeated);
    detail::get_optional(j, "deprecated", f.deprecated);
    detail::get_optional(j, "docs", f.docs);
  }

  // ---------------------------------------------------------------------------
  // FlagGroupEntry
  // ---------------------------------------------------------------------------

  inline void
  to_json(nlohmann::json& j, const FlagGroupEntry& e) {
    j = nlohmann::json::object();
    j["names"] = e.names;
    j["doc"] = e.doc;
    j["value"] = e.value;
  }

  inline void
  from_json(const nlohmann::json& j, FlagGroupEntry& e) {
    j.at("names").get_to(e.names);
    j.at("doc").get_to(e.doc);
    e.value = j.at("value");
  }

  // ---------------------------------------------------------------------------
  // FlagGroup
  // ---------------------------------------------------------------------------

  inline void
  to_json(nlohmann::json& j, const FlagGroup& fg) {
    j = nlohmann::json::object();
    j["kind"] = "flag_group";
    j["dest"] = fg.dest;
    j["doc"] = fg.doc;
    j["default"] = fg.default_value;
    j["flags"] = fg.flags;
    detail::set_optional(j, "repeated", fg.repeated);
    detail::set_optional(j, "docs", fg.docs);
  }

  inline void
  from_json(const nlohmann::json& j, FlagGroup& fg) {
    j.at("dest").get_to(fg.dest);
    j.at("doc").get_to(fg.doc);
    fg.default_value = j.at("default");
    j.at("flags").get_to(fg.flags);
    detail::get_optional(j, "repeated", fg.repeated);
    detail::get_optional(j, "docs", fg.docs);
  }

  // ---------------------------------------------------------------------------
  // Option
  // ---------------------------------------------------------------------------

  inline void
  to_json(nlohmann::json& j, const Option& o) {
    j = nlohmann::json::object();
    j["kind"] = "option";
    j["names"] = o.names;
    j["doc"] = o.doc;
    j["type"] = o.type;
    detail::set_optional(j, "docv", o.docv);
    if (o.default_value.has_value()) { j["default"] = *o.default_value; }
    detail::set_optional(j, "required", o.required);
    detail::set_optional(j, "repeated", o.repeated);
    detail::set_optional(j, "choices", o.choices);
    detail::set_optional(j, "must_exist", o.must_exist);
    detail::set_optional(j, "dest", o.dest);
    detail::set_optional(j, "env", o.env);
    detail::set_optional(j, "docs", o.docs);
  }

  inline void
  from_json(const nlohmann::json& j, Option& o) {
    j.at("names").get_to(o.names);
    j.at("doc").get_to(o.doc);
    j.at("type").get_to(o.type);
    detail::get_optional(j, "docv", o.docv);
    if (j.contains("default")) {
      o.default_value = j.at("default");
    } else {
      o.default_value = std::nullopt;
    }
    detail::get_optional(j, "required", o.required);
    detail::get_optional(j, "repeated", o.repeated);
    detail::get_optional(j, "choices", o.choices);
    detail::get_optional(j, "must_exist", o.must_exist);
    detail::get_optional(j, "dest", o.dest);
    detail::get_optional(j, "env", o.env);
    detail::get_optional(j, "docs", o.docs);
  }

  // ---------------------------------------------------------------------------
  // Positional
  // ---------------------------------------------------------------------------

  inline void
  to_json(nlohmann::json& j, const Positional& p) {
    j = nlohmann::json::object();
    j["kind"] = "positional";
    j["name"] = p.name;
    j["doc"] = p.doc;
    j["type"] = p.type;
    detail::set_optional(j, "docv", p.docv);
    if (p.default_value.has_value()) { j["default"] = *p.default_value; }
    detail::set_optional(j, "required", p.required);
    detail::set_optional(j, "repeated", p.repeated);
    detail::set_optional(j, "must_exist", p.must_exist);
    detail::set_optional(j, "docs", p.docs);
  }

  inline void
  from_json(const nlohmann::json& j, Positional& p) {
    j.at("name").get_to(p.name);
    j.at("doc").get_to(p.doc);
    j.at("type").get_to(p.type);
    detail::get_optional(j, "docv", p.docv);
    if (j.contains("default")) {
      p.default_value = j.at("default");
    } else {
      p.default_value = std::nullopt;
    }
    detail::get_optional(j, "required", p.required);
    detail::get_optional(j, "repeated", p.repeated);
    detail::get_optional(j, "must_exist", p.must_exist);
    detail::get_optional(j, "docs", p.docs);
  }

  // ---------------------------------------------------------------------------
  // Argument
  // ---------------------------------------------------------------------------

  inline void
  to_json(nlohmann::json& j, const Argument& a) {
    std::visit([&j](const auto& v) { to_json(j, v); }, a);
  }

  inline void
  from_json(const nlohmann::json& j, Argument& a) {
    auto kind = j.at("kind").get<std::string>();
    if (kind == "flag") {
      Flag f;
      from_json(j, f);
      a = f;
    } else if (kind == "flag_group") {
      FlagGroup fg;
      from_json(j, fg);
      a = fg;
    } else if (kind == "option") {
      Option o;
      from_json(j, o);
      a = o;
    } else if (kind == "positional") {
      Positional p;
      from_json(j, p);
      a = p;
    } else {
      throw std::invalid_argument("unknown argument kind: " + kind);
    }
  }

  // ---------------------------------------------------------------------------
  // ParagraphBlock
  // ---------------------------------------------------------------------------

  inline void
  to_json(nlohmann::json& j, const ParagraphBlock& b) {
    j = nlohmann::json::object();
    j["paragraph"] = b.paragraph;
  }

  inline void
  from_json(const nlohmann::json& j, ParagraphBlock& b) {
    j.at("paragraph").get_to(b.paragraph);
  }

  // ---------------------------------------------------------------------------
  // PreBlock
  // ---------------------------------------------------------------------------

  inline void
  to_json(nlohmann::json& j, const PreBlock& b) {
    j = nlohmann::json::object();
    j["pre"] = b.pre;
  }

  inline void
  from_json(const nlohmann::json& j, PreBlock& b) {
    j.at("pre").get_to(b.pre);
  }

  // ---------------------------------------------------------------------------
  // LabelTextBlock
  // ---------------------------------------------------------------------------

  inline void
  to_json(nlohmann::json& j, const LabelTextBlock& b) {
    j = nlohmann::json::object();
    j["label"] = b.label;
    j["text"] = b.text;
  }

  inline void
  from_json(const nlohmann::json& j, LabelTextBlock& b) {
    j.at("label").get_to(b.label);
    j.at("text").get_to(b.text);
  }

  // ---------------------------------------------------------------------------
  // NoBlankBlock
  // ---------------------------------------------------------------------------

  inline void
  to_json(nlohmann::json& j, const NoBlankBlock&) {
    j = nlohmann::json::object();
    j["noblank"] = true;
  }

  inline void
  from_json(const nlohmann::json&, NoBlankBlock&) {}

  // ---------------------------------------------------------------------------
  // ManBlock
  // ---------------------------------------------------------------------------

  inline void
  to_json(nlohmann::json& j, const ManBlock& mb) {
    std::visit([&j](const auto& v) { to_json(j, v); }, mb);
  }

  inline void
  from_json(const nlohmann::json& j, ManBlock& mb) {
    if (j.contains("paragraph")) {
      ParagraphBlock b;
      from_json(j, b);
      mb = b;
    } else if (j.contains("pre")) {
      PreBlock b;
      from_json(j, b);
      mb = b;
    } else if (j.contains("label")) {
      LabelTextBlock b;
      from_json(j, b);
      mb = b;
    } else if (j.contains("noblank")) {
      mb = NoBlankBlock{};
    } else {
      throw std::invalid_argument("unknown man_block format");
    }
  }

  // ---------------------------------------------------------------------------
  // ManSection
  // ---------------------------------------------------------------------------

  inline void
  to_json(nlohmann::json& j, const ManSection& s) {
    j = nlohmann::json::object();
    j["name"] = s.name;
    j["blocks"] = s.blocks;
    if (s.after.has_value()) { j["after"] = *s.after; }
  }

  inline void
  from_json(const nlohmann::json& j, ManSection& s) {
    j.at("name").get_to(s.name);
    j.at("blocks").get_to(s.blocks);
    if (j.contains("after")) { s.after = j.at("after").get<std::string>(); }
  }

  // ---------------------------------------------------------------------------
  // ManXref
  // ---------------------------------------------------------------------------

  inline void
  to_json(nlohmann::json& j, const ManXref& x) {
    j = nlohmann::json::object();
    j["name"] = x.name;
    j["section"] = x.section;
  }

  inline void
  from_json(const nlohmann::json& j, ManXref& x) {
    j.at("name").get_to(x.name);
    j.at("section").get_to(x.section);
  }

  // ---------------------------------------------------------------------------
  // Man
  // ---------------------------------------------------------------------------

  inline void
  to_json(nlohmann::json& j, const Man& m) {
    j = nlohmann::json::object();
    detail::set_optional(j, "section", m.section);
    detail::set_optional(j, "sections", m.sections);
    detail::set_optional(j, "xrefs", m.xrefs);
  }

  inline void
  from_json(const nlohmann::json& j, Man& m) {
    detail::get_optional(j, "section", m.section);
    detail::get_optional(j, "sections", m.sections);
    detail::get_optional(j, "xrefs", m.xrefs);
  }

  // ---------------------------------------------------------------------------
  // ConfigPaths
  // ---------------------------------------------------------------------------

  inline void
  to_json(nlohmann::json& j, const ConfigPaths& cp) {
    j = nlohmann::json::object();
    detail::set_optional(j, "system", cp.system);
    detail::set_optional(j, "user", cp.user);
    detail::set_optional(j, "local", cp.local);
  }

  inline void
  from_json(const nlohmann::json& j, ConfigPaths& cp) {
    detail::get_optional(j, "system", cp.system);
    detail::get_optional(j, "user", cp.user);
    detail::get_optional(j, "local", cp.local);
  }

  // ---------------------------------------------------------------------------
  // Config
  // ---------------------------------------------------------------------------

  inline void
  to_json(nlohmann::json& j, const Config& c) {
    j = nlohmann::json::object();
    j["format"] = c.format;
    detail::set_optional(j, "paths", c.paths);
  }

  inline void
  from_json(const nlohmann::json& j, Config& c) {
    j.at("format").get_to(c.format);
    detail::get_optional(j, "paths", c.paths);
  }

  // ---------------------------------------------------------------------------
  // Command
  // ---------------------------------------------------------------------------

  inline void
  to_json(nlohmann::json& j, const Command& cmd) {
    j = nlohmann::json::object();
    j["name"] = cmd.name;
    j["doc"] = cmd.doc;
    detail::set_optional(j, "args", cmd.args);
    detail::set_optional(j, "commands", cmd.commands);
    detail::set_optional(j, "man", cmd.man);
    detail::set_optional(j, "envs", cmd.envs);
    detail::set_optional(j, "exits", cmd.exits);
  }

  inline void
  from_json(const nlohmann::json& j, Command& cmd) {
    j.at("name").get_to(cmd.name);
    j.at("doc").get_to(cmd.doc);
    detail::get_optional(j, "args", cmd.args);
    detail::get_optional(j, "commands", cmd.commands);
    detail::get_optional(j, "man", cmd.man);
    detail::get_optional(j, "envs", cmd.envs);
    detail::get_optional(j, "exits", cmd.exits);
  }

  // ---------------------------------------------------------------------------
  // Root
  // ---------------------------------------------------------------------------

  inline void
  to_json(nlohmann::json& j, const Root& r) {
    j = nlohmann::json::object();
    j["name"] = r.name;
    j["doc"] = r.doc;
    detail::set_optional(j, "args", r.args);
    detail::set_optional(j, "commands", r.commands);
    detail::set_optional(j, "man", r.man);
    detail::set_optional(j, "envs", r.envs);
    detail::set_optional(j, "exits", r.exits);
    detail::set_optional(j, "version", r.version);
    detail::set_optional(j, "config", r.config);
  }

  inline void
  from_json(const nlohmann::json& j, Root& r) {
    j.at("name").get_to(r.name);
    j.at("doc").get_to(r.doc);
    detail::get_optional(j, "args", r.args);
    detail::get_optional(j, "commands", r.commands);
    detail::get_optional(j, "man", r.man);
    detail::get_optional(j, "envs", r.envs);
    detail::get_optional(j, "exits", r.exits);
    detail::get_optional(j, "version", r.version);
    detail::get_optional(j, "config", r.config);
  }

} // namespace json_commander::model
