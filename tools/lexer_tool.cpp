//
// Created by TYTY on 2021-01-01 001.
//

#include <iostream>
#include <fstream>
#include <variant>
#include "lexer.h"

using namespace bbcode::lexer;

auto LexItemTypeNames = std::array {
  "Newline",
  "OpenTagLeft",
  "CloseTagLeft",
  "TagRight",
  "Equal",
  "Constant",
  "Literal",
  "End"
};

int main(int argc, char ** argv) {
  std::istream* input = &std::cin;
  std::ostream* output = &std::cout;

  std::ifstream file_in;
  std::ofstream file_out;

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

  Lexer lexer([output](LexItem&& item) {
    if (item.type == Invalid) {
      std::cerr << "We are sorry but we encountered a problem." << std::endl;
      exit(2);
    }

    *output << item.line << ':' << item.chr << ' ';
    *output << '[' << LexItemTypeNames[item.type] << ']';
    if (item.type == Constant) {
      *output << ": `" << std::get<Constant>(item.d).content << "`";
    } else if(item.type == Literal) {
      *output << ": `" << std::get<Literal>(item.d).content << "`";
    }

    *output << std::endl;

    if (!output->good()) {
      std::cerr << "Failed writing output." << std::endl;
      exit(1);
    }
  });

  while (input->good()) {
    auto next = input->get();
    if (!input->eof()) {
      lexer.put(next);
    } else {
      break;
    }
  }

  lexer.finish();

  if (file_in.is_open()) {
    file_in.close();
  }

  if (file_out.is_open()) {
    file_out.close();
  }

  return 0;
}

