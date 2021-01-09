//
// Created by TYTY on 2021-01-08 008.
//

#include <iostream>
#include <iomanip>
#include <fstream>
#include "parser.h"
#include "cassert"

using namespace bbcode::lexer;
using namespace bbcode::parser;

static auto NodeTypeNames = std::array {
    "Invalid",
    "Omission",
    "Simple",
    "Parametric",
    "Greedy",
    "Verbatim",
    "Literal",
    "Constant",
    "Newline",
    "End"
};

static auto NodeTypeNamesBase = &NodeTypeNames[1];

namespace Color {
enum Code {
  FG_RED      = 91,
  FG_MAGENTA  = 95,
  FG_CYAN     = 96,
  FG_DEFAULT  = 39,
};

class Modifier {
  Code code;
 public:
  Modifier() noexcept : Modifier(FG_DEFAULT) {}
  explicit Modifier(Code pCode) noexcept : code(pCode) {}
  friend std::ostream&
  operator<<(std::ostream& os, const Modifier& mod) {
    return os << "\033[" << mod.code << "m";
  }
};

static Color::Modifier red(Color::FG_RED);
static Color::Modifier magenta(Color::FG_MAGENTA);
static Color::Modifier cyan(Color::FG_CYAN);
static Color::Modifier def(Color::FG_DEFAULT);

}

static void write_escape(std::string_view s, std::ostream* o) {
  for (const auto& c : s) {
    switch (c) {
      case '"':
        *o << "\\\"";
        break;
      case '\\':
        *o << "\\\\";
        break;
      case '\r':
        *o << "\\r";
        break;
      case '\n':
        *o << "\\n";
        break;
      default:
        *o << c;
        break;
    }
  }
}

void write_node_as_json(const Node& node, std::ostream* o) {
  *o << R"({"type":")" << *(NodeTypeNamesBase + node.type)
     << R"(","line":)" << node.line << R"(,"chr":)" << node.chr
     << R"(,"offset":)" << node.offset << R"(,"span":)" << node.span;
  switch (node.type) {
    case bbcode::grammar::Literal:
    case bbcode::grammar::Constant: {
      *o << R"(,"content":")";
      write_escape(node.data, o);
      *o << "\"}";
      break;
    }
    case bbcode::grammar::Omission:
      *o << R"(,"name":")" << node.name << "\"}";
      break;
    case bbcode::grammar::Simple:
    case bbcode::grammar::Greedy:
    case bbcode::grammar::Verbatim: {
      *o << R"(,"name":")" << node.name << "\"";
      *o << R"(,"content":[)";
      for (auto it = node.children.begin(); it != node.children.end(); ++it) {
        write_node_as_json(*it, o);
        if (it + 1 != node.children.end()) {
          *o << ",";
        }
      }
      *o << "]}";
      break;
    }
    case bbcode::grammar::Parametric: {
      *o << R"(,"name":")" << node.name << "\"";
      *o << R"(,"parameter":")" << node.data << "\"";
      *o << R"(,"content":[)";
      for (auto it = node.children.begin(); it != node.children.end(); ++it) {
        write_node_as_json(*it, o);
        if (it + 1 != node.children.end()) {
          *o << ",";
        }
      }
      *o << "]}";
      break;
    }
    case bbcode::grammar::Newline:
      *o << "}";
      break;
    case bbcode::grammar::Invalid: {
      *o << R"(,"name":")" << node.name << "\"";
      if (!node.data.empty()) {
        *o << R"(,"content":")";
        write_escape(node.data, o);
        *o << "\"";
      } else if (!node.children.empty()) {
        *o << R"(,"content":[)";
        for (auto it = node.children.begin(); it != node.children.end(); ++it) {
          write_node_as_json(*it, o);
          if (it + 1 != node.children.end()) {
            *o << ",";
          }
        }
        *o << "]";
      }
      *o << "}";
      break;
    }
    case bbcode::grammar::End:
      assert(false);
  }
}

int main(int argc, char ** argv) {
  std::istream* input = &std::cin;
  std::ostream* output = &std::cout;

  std::ifstream file_in;
  std::ofstream file_out;

  std::vector<std::string> source {{}};
  usize error = 0;
  usize warning = 0;
  usize note = 0;

  if (argc >= 2) {
    auto i = std::string_view(argv[1]);
    if (i != "-") {
      file_in = std::ifstream(i.data(), std::ios::binary);
      if (file_in.is_open()) {
        input = &file_in;
      } else {
        std::cerr << "Failed opening file: " << i << " for read." << std::endl;
        exit(1);
      }
    }
  }

  if (argc >= 3) {
    auto o = std::string_view(argv[2]);
    if (o != "-") {
      file_out = std::ofstream(o.data(), std::ios::binary | std::ios::trunc);
      if (file_out.is_open()) {
        output = &file_out;
      } else {
        std::cerr << "Failed opening file: " << o << " for write." << std::endl;
        exit(1);
      }
    }
  }

  *output << "[";

  Parser parser([output, first = true](Node&& node) mutable {
    if (node.type == NodeType::End) {
      *output << "]";
    } else {
      if (!first) {
        *output << ",";
      }
      write_node_as_json(node, output);
    }

    if (first) {
      first = false;
    }
  }, [&](Message&& message) {
    std::cerr << "L" << message.line + 1 << ":" << message.chr + 1 << ":\t";
    Color::Modifier cur;
    switch (message.severity) {
      case Tidy:
        ++note;
        cur = Color::cyan;
        std::cerr << cur << "Note: " << Color::def;
        break;
      case Warning:
        ++warning;
        cur = Color::magenta;
        std::cerr << cur << "Warning: " << Color::def;
        break;
      case Error:
        ++error;
        cur = Color::red;
        std::cerr << cur << "Error: " << Color::def;
        break;
    }

    std::cerr << message.message << " [" << cur << "-W" << message.name << Color::def << "]" << std::endl;
    std::cerr << std::setw(5) << std::setfill(' ') << message.line + 1 << std::setw(0);
    std::cerr << " | " << source[message.line] << std::endl;
    std::cerr << "      | " << std::string(message.chr, ' ') << cur << '^';
    if (message.span > 1) {
      std::cerr << std::string(message.span - 1, '~');
    }
    std::cerr << Color::def << std::endl;
  });
  Lexer lexer([&parser](LexItem&& item) {
    parser.put(std::move(item));
  });

  while (input->good()) {
    auto next = input->get();
    if (!input->eof()) {
      if (next == '\n') {
        source.emplace_back();
      } else {
        source.back() += char(next);
      }
      lexer.put(static_cast<i8>(next));
    } else {
      break;
    }
  }

  lexer.finish();

  if (error || warning || note) {
    std::cerr << error << " error";
    if (error > 1) {
      std::cerr << "s";
    }
    std::cerr << ", ";
    std::cerr << warning << " warning";
    if (warning > 1) {
      std::cerr << "s";
    }
    std::cerr << " and ";
    std::cerr << note << " note";
    if (note > 1) {
      std::cerr << "s";
    }
    std::cerr << " generated." << std::endl;
  }

  if (file_in.is_open()) {
    file_in.close();
  }

  if (file_out.is_open()) {
    file_out.close();
  }

  return 0;
}
