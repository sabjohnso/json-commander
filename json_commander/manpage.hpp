#pragma once

#include <json_commander/conv.hpp>
#include <json_commander/model.hpp>

#include <algorithm>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace json_commander::manpage {

  // -------------------------------------------------------------------------
  // Standard section names
  // -------------------------------------------------------------------------

  inline const std::string s_name = "NAME";
  inline const std::string s_synopsis = "SYNOPSIS";
  inline const std::string s_description = "DESCRIPTION";
  inline const std::string s_commands = "COMMANDS";
  inline const std::string s_arguments = "ARGUMENTS";
  inline const std::string s_options = "OPTIONS";
  inline const std::string s_exit_status = "EXIT STATUS";
  inline const std::string s_environment = "ENVIRONMENT";
  inline const std::string s_see_also = "SEE ALSO";

  // -------------------------------------------------------------------------
  // Detail: DocString rendering
  // -------------------------------------------------------------------------

  namespace detail {

    inline std::string
    docstring_to_text(const model::DocString& doc) {
      std::string result;
      bool in_paragraph = false;
      for (const auto& line : doc) {
        if (line.empty()) {
          result += "\n\n";
          in_paragraph = false;
        } else {
          if (in_paragraph) { result += ' '; }
          result += line;
          in_paragraph = true;
        }
      }
      return result;
    }

    inline std::string
    format_names(const model::ArgNames& names) {
      std::string result;
      for (std::size_t i = 0; i < names.size(); ++i) {
        if (i > 0) { result += ", "; }
        std::string prefix = (names[i].size() == 1) ? "\\-" : "\\-\\-";
        std::string escaped;
        for (char c : names[i]) {
          if (c == '-') {
            escaped += "\\-";
          } else {
            escaped += c;
          }
        }
        result += "\\fB" + prefix + escaped + "\\fR";
      }
      return result;
    }

    inline std::string
    type_docv(
      const model::TypeSpec& type,
      const std::optional<std::vector<std::string>>& choices = std::nullopt) {
      return conv::make(type, choices).docv;
    }

    inline std::string
    format_option_label(const model::Option& opt) {
      std::string docv = opt.docv.value_or(type_docv(opt.type, opt.choices));
      if (opt.names.size() == 1) {
        const auto& name = opt.names[0];
        bool is_short = (name.size() == 1);
        std::string sep = is_short ? " " : "=";
        return format_names(opt.names) + sep + "\\fI" + docv + "\\fR";
      }
      std::string result;
      for (std::size_t i = 0; i < opt.names.size(); ++i) {
        if (i > 0) { result += ", "; }
        const auto& name = opt.names[i];
        bool is_short = (name.size() == 1);
        std::string prefix = is_short ? "\\-" : "\\-\\-";
        std::string escaped;
        for (char c : name) {
          if (c == '-') {
            escaped += "\\-";
          } else {
            escaped += c;
          }
        }
        std::string sep = is_short ? " " : "=";
        result +=
          "\\fB" + prefix + escaped + "\\fR" + sep + "\\fI" + docv + "\\fR";
      }
      return result;
    }

    inline std::string
    format_positional_label(const model::Positional& pos) {
      std::string docv;
      if (pos.docv.has_value()) {
        docv = *pos.docv;
      } else {
        docv = pos.name;
        std::transform(
          docv.begin(), docv.end(), docv.begin(), [](unsigned char c) {
            return std::toupper(c);
          });
      }
      return "\\fI" + docv + "\\fR";
    }

    inline std::string
    format_flag_group_entry_label(const model::FlagGroupEntry& entry) {
      return format_names(entry.names);
    }

  } // namespace detail

  // -------------------------------------------------------------------------
  // Text rendering policies and templates
  // -------------------------------------------------------------------------

  namespace detail {

    struct StripFont {
      static void
      on_bold(std::string& /*result*/) {}
      static void
      on_italic(std::string& /*result*/) {}
      static void
      on_reset(std::string& /*result*/) {}
      static std::string
      section_header(const std::string& name) {
        return name + "\n";
      }
    };

    struct AnsiFont {
      static void
      on_bold(std::string& result) {
        result += "\033[1m";
      }
      static void
      on_italic(std::string& result) {
        result += "\033[4m";
      }
      static void
      on_reset(std::string& result) {
        result += "\033[0m";
      }
      static std::string
      section_header(const std::string& name) {
        return "\033[1m" + name + "\033[0m\n";
      }
    };

    inline int
    display_width(const std::string& text) {
      int width = 0;
      for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\033' && i + 1 < text.size() && text[i + 1] == '[') {
          // Skip ANSI escape: \033[ ... m
          i += 2;
          while (i < text.size() && text[i] != 'm') {
            ++i;
          }
        } else {
          ++width;
        }
      }
      return width;
    }

    inline std::string
    wrap_text(const std::string& text, int indent, int width) {
      if (width == 0) { return text; }
      int available = width - indent;
      if (available < 10) { return text; }

      std::string indent_str(static_cast<std::size_t>(indent), ' ');
      std::string result;

      // Split on paragraph breaks first
      std::vector<std::string> paragraphs;
      std::size_t pos = 0;
      while (pos < text.size()) {
        auto brk = text.find("\n\n", pos);
        if (brk == std::string::npos) {
          paragraphs.push_back(text.substr(pos));
          break;
        }
        paragraphs.push_back(text.substr(pos, brk - pos));
        pos = brk + 2;
      }

      for (std::size_t pi = 0; pi < paragraphs.size(); ++pi) {
        if (pi > 0) { result += "\n\n" + indent_str; }

        const auto& para = paragraphs[pi];

        // Split into words, preserving ANSI codes attached to words
        std::vector<std::string> words;
        std::string current_word;
        for (std::size_t i = 0; i < para.size(); ++i) {
          if (para[i] == ' ') {
            if (!current_word.empty()) {
              words.push_back(current_word);
              current_word.clear();
            }
          } else {
            current_word += para[i];
          }
        }
        if (!current_word.empty()) { words.push_back(current_word); }

        int line_width = 0;
        bool first_word = true;
        for (const auto& word : words) {
          int word_width = display_width(word);
          if (first_word) {
            result += word;
            line_width = word_width;
            first_word = false;
          } else if (line_width + 1 + word_width <= available) {
            result += " " + word;
            line_width += 1 + word_width;
          } else {
            result += "\n" + indent_str + word;
            line_width = word_width;
          }
        }
      }

      return result;
    }

    template <typename FontPolicy>
    inline std::string
    unescape_with(const std::string& text) {
      std::string result;
      result.reserve(text.size());
      for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\\' && i + 1 < text.size()) {
          char next = text[i + 1];
          if (next == 'f' && i + 2 < text.size()) {
            char code = text[i + 2];
            if (code == 'B') {
              FontPolicy::on_bold(result);
            } else if (code == 'I') {
              FontPolicy::on_italic(result);
            } else if (code == 'R') {
              FontPolicy::on_reset(result);
            }
            i += 2;
          } else if (next == '-') {
            result += '-';
            ++i;
          } else if (next == '&') {
            ++i;
          } else if (next == '\\') {
            result += '\\';
            ++i;
          } else {
            result += text[i];
          }
        } else {
          result += text[i];
        }
      }
      return result;
    }

    template <typename FontPolicy>
    inline std::string
    render_block_with(const model::ManBlock& block, int width = 0) {
      return std::visit(
        [width](const auto& b) -> std::string {
          using T = std::decay_t<decltype(b)>;
          if constexpr (std::is_same_v<T, model::ParagraphBlock>) {
            auto text =
              unescape_with<FontPolicy>(docstring_to_text(b.paragraph));
            return "       " + wrap_text(text, 7, width) + "\n";
          } else if constexpr (std::is_same_v<T, model::PreBlock>) {
            std::string result;
            for (const auto& line : b.pre) {
              result += "       " + line + "\n";
            }
            return result;
          } else if constexpr (std::is_same_v<T, model::LabelTextBlock>) {
            std::string result;
            result += "       " + unescape_with<FontPolicy>(b.label) + "\n";
            auto text = unescape_with<FontPolicy>(docstring_to_text(b.text));
            result += "           " + wrap_text(text, 11, width) + "\n";
            return result;
          } else if constexpr (std::is_same_v<T, model::NoBlankBlock>) {
            return "";
          }
        },
        block);
    }

    template <typename FontPolicy>
    inline std::string
    render_section_with(const model::ManSection& section, int width = 0) {
      std::string result = FontPolicy::section_header(section.name);
      for (const auto& block : section.blocks) {
        result += render_block_with<FontPolicy>(block, width);
      }
      result += "\n";
      return result;
    }

    template <typename FontPolicy>
    inline std::string
    render_page_with(
      const std::string& /*name*/,
      const std::vector<model::ManSection>& sections,
      int width = 0) {
      std::string result;
      for (const auto& section : sections) {
        result += render_section_with<FontPolicy>(section, width);
      }
      return result;
    }

  } // namespace detail

  // -------------------------------------------------------------------------
  // Groff rendering
  // -------------------------------------------------------------------------

  namespace groff {

    inline std::string
    escape(const std::string& text) {
      std::string result;
      result.reserve(text.size());
      for (std::size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        if (c == '\\') {
          result += "\\\\";
        } else if (i == 0 && c == '.') {
          result += "\\&.";
        } else if (i == 0 && c == '\'') {
          result += "\\&'";
        } else {
          result += c;
        }
      }
      return result;
    }

    inline std::string
    render_block(const model::ManBlock& block) {
      return std::visit(
        [](const auto& b) -> std::string {
          using T = std::decay_t<decltype(b)>;
          if constexpr (std::is_same_v<T, model::ParagraphBlock>) {
            return ".PP\n" + detail::docstring_to_text(b.paragraph) + "\n";
          } else if constexpr (std::is_same_v<T, model::PreBlock>) {
            std::string result = ".nf\n";
            for (const auto& line : b.pre) {
              result += line + "\n";
            }
            result += ".fi\n";
            return result;
          } else if constexpr (std::is_same_v<T, model::LabelTextBlock>) {
            return ".TP\n\\fB" + b.label + "\\fR\n" +
                   escape(detail::docstring_to_text(b.text)) + "\n";
          } else if constexpr (std::is_same_v<T, model::NoBlankBlock>) {
            return "";
          }
        },
        block);
    }

    inline std::string
    render_section(const model::ManSection& section) {
      std::string result = ".SH " + section.name + "\n";
      for (const auto& block : section.blocks) {
        result += render_block(block);
      }
      return result;
    }

    inline std::string
    render_page(
      const std::string& name,
      int man_section,
      const std::string& version,
      const std::vector<model::ManSection>& sections) {
      std::string upper_name = name;
      std::transform(
        upper_name.begin(),
        upper_name.end(),
        upper_name.begin(),
        [](unsigned char c) { return std::toupper(c); });
      std::string result = ".TH " + upper_name + " " +
                           std::to_string(man_section) + " \"\" \"" + version +
                           "\"\n";
      for (const auto& section : sections) {
        result += render_section(section);
      }
      return result;
    }

  } // namespace groff

  // -------------------------------------------------------------------------
  // Plain-text rendering
  // -------------------------------------------------------------------------

  namespace plain {

    inline std::string
    unescape(const std::string& text) {
      return detail::unescape_with<detail::StripFont>(text);
    }

    inline std::string
    render_block(const model::ManBlock& block, int width = 0) {
      return detail::render_block_with<detail::StripFont>(block, width);
    }

    inline std::string
    render_section(const model::ManSection& section, int width = 0) {
      return detail::render_section_with<detail::StripFont>(section, width);
    }

    inline std::string
    render_page(
      const std::string& name,
      const std::vector<model::ManSection>& sections,
      int width = 0) {
      return detail::render_page_with<detail::StripFont>(name, sections, width);
    }

  } // namespace plain

  // -------------------------------------------------------------------------
  // ANSI-text rendering (for interactive terminals)
  // -------------------------------------------------------------------------

  namespace ansi {

    inline std::string
    unescape(const std::string& text) {
      return detail::unescape_with<detail::AnsiFont>(text);
    }

    inline std::string
    render_block(const model::ManBlock& block, int width = 0) {
      return detail::render_block_with<detail::AnsiFont>(block, width);
    }

    inline std::string
    render_section(const model::ManSection& section, int width = 0) {
      return detail::render_section_with<detail::AnsiFont>(section, width);
    }

    inline std::string
    render_page(
      const std::string& name,
      const std::vector<model::ManSection>& sections,
      int width = 0) {
      return detail::render_page_with<detail::AnsiFont>(name, sections, width);
    }

  } // namespace ansi

  // -------------------------------------------------------------------------
  // Argument section generation
  // -------------------------------------------------------------------------

  namespace detail {

    inline std::string
    arg_section_name(const model::Argument& arg) {
      return std::visit(
        [](const auto& a) -> std::string {
          using T = std::decay_t<decltype(a)>;
          if constexpr (std::is_same_v<T, model::Positional>) {
            return a.docs.value_or(s_arguments);
          } else if constexpr (std::is_same_v<T, model::Flag>) {
            return a.docs.value_or(s_options);
          } else if constexpr (std::is_same_v<T, model::Option>) {
            return a.docs.value_or(s_options);
          } else if constexpr (std::is_same_v<T, model::FlagGroup>) {
            return a.docs.value_or(s_options);
          }
        },
        arg);
    }

    inline std::vector<model::ManBlock>
    arg_blocks(const model::Argument& arg) {
      return std::visit(
        [](const auto& a) -> std::vector<model::ManBlock> {
          using T = std::decay_t<decltype(a)>;
          if constexpr (std::is_same_v<T, model::Flag>) {
            return {model::LabelTextBlock{format_names(a.names), a.doc}};
          } else if constexpr (std::is_same_v<T, model::Option>) {
            return {model::LabelTextBlock{format_option_label(a), a.doc}};
          } else if constexpr (std::is_same_v<T, model::Positional>) {
            return {model::LabelTextBlock{format_positional_label(a), a.doc}};
          } else if constexpr (std::is_same_v<T, model::FlagGroup>) {
            std::vector<model::ManBlock> blocks;
            for (const auto& entry : a.flags) {
              blocks.push_back(
                model::LabelTextBlock{
                  format_flag_group_entry_label(entry), entry.doc});
            }
            return blocks;
          }
        },
        arg);
    }

  } // namespace detail

  inline std::vector<model::ManSection>
  make_arg_sections(const std::vector<model::Argument>& args) {
    // Collect blocks grouped by section name, preserving insertion order
    std::vector<std::string> order;
    std::map<std::string, std::vector<model::ManBlock>> groups;

    for (const auto& arg : args) {
      std::string name = detail::arg_section_name(arg);
      if (groups.find(name) == groups.end()) { order.push_back(name); }
      auto blocks = detail::arg_blocks(arg);
      auto& dest = groups[name];
      dest.insert(dest.end(), blocks.begin(), blocks.end());
    }

    std::vector<model::ManSection> sections;
    for (const auto& name : order) {
      sections.push_back({name, groups[name]});
    }
    return sections;
  }

  // -------------------------------------------------------------------------
  // Auto-generated sections
  // -------------------------------------------------------------------------

  inline model::ManSection
  make_name_section(const std::string& name, const model::DocString& doc) {
    std::string first_line = doc.empty() ? "" : doc[0];
    return {s_name, {model::ParagraphBlock{{name + " \\- " + first_line}}}};
  }

  inline model::ManSection
  make_synopsis_section(
    const std::string& name,
    const std::vector<model::Argument>& args,
    bool has_commands) {
    std::string synopsis = "\\fB" + name + "\\fR";

    bool has_options = false;
    std::vector<std::string> positionals;
    for (const auto& arg : args) {
      std::visit(
        [&](const auto& a) {
          using T = std::decay_t<decltype(a)>;
          if constexpr (std::is_same_v<T, model::Positional>) {
            std::string docv;
            if (a.docv.has_value()) {
              docv = *a.docv;
            } else {
              docv = a.name;
              std::transform(
                docv.begin(), docv.end(), docv.begin(), [](unsigned char c) {
                  return std::toupper(c);
                });
            }
            if (a.required.value_or(false)) {
              positionals.push_back("\\fI" + docv + "\\fR");
            } else {
              positionals.push_back("[\\fI" + docv + "\\fR]");
            }
          } else {
            has_options = true;
          }
        },
        arg);
    }

    if (has_options) { synopsis += " [OPTIONS]"; }
    for (const auto& p : positionals) {
      synopsis += " " + p;
    }
    if (has_commands) { synopsis += " COMMAND"; }

    return {s_synopsis, {model::ParagraphBlock{{synopsis}}}};
  }

  inline model::ManSection
  make_commands_section(const std::vector<model::Command>& commands) {
    std::vector<model::ManBlock> blocks;
    for (const auto& cmd : commands) {
      blocks.push_back(
        model::LabelTextBlock{"\\fB" + cmd.name + "\\fR", cmd.doc});
    }
    return {s_commands, blocks};
  }

  inline model::ManSection
  make_exit_status_section(const std::vector<model::ExitInfo>& exits) {
    std::vector<model::ManBlock> blocks;
    for (const auto& e : exits) {
      std::string label = std::to_string(e.code);
      if (e.max.has_value()) {
        label += "-";
        label += std::to_string(*e.max);
      }
      blocks.push_back(model::LabelTextBlock{label, e.doc});
    }
    return {s_exit_status, blocks};
  }

  inline model::ManSection
  make_environment_section(const std::vector<model::EnvInfo>& envs) {
    std::vector<model::ManBlock> blocks;
    for (const auto& e : envs) {
      model::DocString doc = e.doc.value_or(model::DocString{});
      blocks.push_back(model::LabelTextBlock{"\\fB" + e.var + "\\fR", doc});
    }
    return {s_environment, blocks};
  }

  inline model::ManSection
  make_see_also_section(const std::vector<model::ManXref>& xrefs) {
    std::string text;
    for (std::size_t i = 0; i < xrefs.size(); ++i) {
      if (i > 0) { text += ", "; }
      text += "\\fB" + xrefs[i].name + "\\fR(" +
              std::to_string(xrefs[i].section) + ")";
    }
    return {s_see_also, {model::ParagraphBlock{{text}}}};
  }

  // -------------------------------------------------------------------------
  // Section ordering
  // -------------------------------------------------------------------------

  inline int
  section_order(const std::string& name) {
    static const std::vector<std::string> order = {
      s_name,
      s_synopsis,
      s_description,
      s_commands,
      s_arguments,
      s_options,
      s_exit_status,
      s_environment,
      s_see_also,
    };
    for (int i = 0; i < static_cast<int>(order.size()); ++i) {
      if (order[i] == name) { return i; }
    }
    return static_cast<int>(order.size());
  }

  // -------------------------------------------------------------------------
  // Assembly
  // -------------------------------------------------------------------------

  template <typename T>
  std::vector<model::ManSection>
  assemble(
    const T& root,
    const std::string& display_name,
    const std::string& synopsis_name = "") {
    std::map<std::string, model::ManSection> section_map;
    std::vector<std::string> section_names;

    auto add_section = [&](const model::ManSection& s) {
      if (section_map.find(s.name) == section_map.end()) {
        section_names.push_back(s.name);
        section_map[s.name] = s;
      } else {
        auto& existing = section_map[s.name];
        existing.blocks.insert(
          existing.blocks.end(), s.blocks.begin(), s.blocks.end());
      }
    };

    // NAME (always hyphenated per man convention)
    add_section(make_name_section(display_name, root.doc));

    // SYNOPSIS (space-separated for subcommands)
    const std::string& syn_name =
      synopsis_name.empty() ? display_name : synopsis_name;
    bool has_commands = root.commands.has_value() && !root.commands->empty();
    add_section(make_synopsis_section(
      syn_name,
      root.args.value_or(std::vector<model::Argument>{}),
      has_commands));

    // User-provided sections
    if (root.man.has_value() && root.man->sections.has_value()) {
      for (const auto& s : *root.man->sections) {
        add_section(s);
      }
    }

    // COMMANDS
    if (has_commands) { add_section(make_commands_section(*root.commands)); }

    // Argument sections (OPTIONS, ARGUMENTS, custom)
    if (root.args.has_value()) {
      auto arg_secs = make_arg_sections(*root.args);
      for (const auto& s : arg_secs) {
        add_section(s);
      }
    }

    // EXIT STATUS
    if (root.exits.has_value() && !root.exits->empty()) {
      add_section(make_exit_status_section(*root.exits));
    }

    // ENVIRONMENT
    if (root.envs.has_value() && !root.envs->empty()) {
      add_section(make_environment_section(*root.envs));
    }

    // SEE ALSO
    if (
      root.man.has_value() && root.man->xrefs.has_value() &&
      !root.man->xrefs->empty()) {
      add_section(make_see_also_section(*root.man->xrefs));
    }

    // Sort by standard ordering
    std::sort(
      section_names.begin(),
      section_names.end(),
      [](const std::string& a, const std::string& b) {
        return section_order(a) < section_order(b);
      });

    std::vector<model::ManSection> result;
    for (const auto& name : section_names) {
      result.push_back(section_map[name]);
    }
    return result;
  }

  // -------------------------------------------------------------------------
  // Subcommand lookup
  // -------------------------------------------------------------------------

  inline const model::Command&
  find_command(const model::Root& root, const std::vector<std::string>& path) {
    if (path.empty()) { throw std::runtime_error("find_command: empty path"); }

    const std::vector<model::Command>* commands =
      root.commands.has_value() ? &*root.commands : nullptr;
    const model::Command* current = nullptr;

    for (const auto& segment : path) {
      if (!commands) {
        throw std::runtime_error("subcommand not found: " + segment);
      }
      bool found = false;
      for (const auto& cmd : *commands) {
        if (cmd.name == segment) {
          current = &cmd;
          commands = cmd.commands.has_value() ? &*cmd.commands : nullptr;
          found = true;
          break;
        }
      }
      if (!found) {
        throw std::runtime_error("subcommand not found: " + segment);
      }
    }

    return *current;
  }

  // -------------------------------------------------------------------------
  // Convenience: assemble + render
  // -------------------------------------------------------------------------

  inline std::string
  to_groff(const model::Root& root) {
    int man_section = 1;
    if (root.man.has_value() && root.man->section.has_value()) {
      man_section = *root.man->section;
    }
    std::string version = root.version.value_or("");
    auto sections = assemble(root, root.name);
    return groff::render_page(root.name, man_section, version, sections);
  }

  inline std::string
  to_groff(
    const model::Command& cmd,
    const std::string& full_name,
    const std::string& version = "",
    const std::string& synopsis_name = "") {
    int man_section = 1;
    if (cmd.man.has_value() && cmd.man->section.has_value()) {
      man_section = *cmd.man->section;
    }
    auto sections = assemble(cmd, full_name, synopsis_name);
    return groff::render_page(full_name, man_section, version, sections);
  }

  inline std::string
  to_groff(
    const model::Root& root, const std::vector<std::string>& command_path) {
    if (command_path.empty()) { return to_groff(root); }

    std::string version = root.version.value_or("");
    const auto& cmd = find_command(root, command_path);

    std::string full_name = root.name;
    std::string syn_name = root.name;
    for (const auto& segment : command_path) {
      full_name += "-" + segment;
      syn_name += " " + segment;
    }

    return to_groff(cmd, full_name, version, syn_name);
  }

  // -------------------------------------------------------------------------
  // Convenience: assemble + plain-text render
  // -------------------------------------------------------------------------

  inline std::string
  to_plain_text(const model::Root& root, int width = 0) {
    auto sections = assemble(root, root.name);
    return plain::render_page(root.name, sections, width);
  }

  inline std::string
  to_plain_text(
    const model::Command& cmd,
    const std::string& full_name,
    const std::string& synopsis_name = "",
    int width = 0) {
    auto sections = assemble(cmd, full_name, synopsis_name);
    return plain::render_page(full_name, sections, width);
  }

  inline std::string
  to_plain_text(
    const model::Root& root,
    const std::vector<std::string>& command_path,
    int width = 0) {
    if (command_path.empty()) { return to_plain_text(root, width); }

    const auto& cmd = find_command(root, command_path);

    std::string full_name = root.name;
    std::string syn_name = root.name;
    for (const auto& segment : command_path) {
      full_name += "-" + segment;
      syn_name += " " + segment;
    }

    return to_plain_text(cmd, full_name, syn_name, width);
  }

  // -------------------------------------------------------------------------
  // Convenience: assemble + ANSI-text render
  // -------------------------------------------------------------------------

  inline std::string
  to_ansi_text(const model::Root& root, int width = 0) {
    auto sections = assemble(root, root.name);
    return ansi::render_page(root.name, sections, width);
  }

  inline std::string
  to_ansi_text(
    const model::Command& cmd,
    const std::string& full_name,
    const std::string& synopsis_name = "",
    int width = 0) {
    auto sections = assemble(cmd, full_name, synopsis_name);
    return ansi::render_page(full_name, sections, width);
  }

  inline std::string
  to_ansi_text(
    const model::Root& root,
    const std::vector<std::string>& command_path,
    int width = 0) {
    if (command_path.empty()) { return to_ansi_text(root, width); }

    const auto& cmd = find_command(root, command_path);

    std::string full_name = root.name;
    std::string syn_name = root.name;
    for (const auto& segment : command_path) {
      full_name += "-" + segment;
      syn_name += " " + segment;
    }

    return to_ansi_text(cmd, full_name, syn_name, width);
  }

} // namespace json_commander::manpage
