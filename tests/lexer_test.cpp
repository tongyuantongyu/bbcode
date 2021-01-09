//
// Created by TYTY on 2021-01-01 001.
//

#include "lexer.h"
#include <vector>
#include <cassert>

using namespace bbcode::lexer;

std::vector<LexItem> get_output(const std::string& input) {
  std::vector<LexItem> items;

  Lexer lexer([&items](LexItem&& item) {
    items.push_back(item);
  });

  for (const auto& c : input) {
    lexer.put(c);
  }

  lexer.finish();
  return items;
}

int main() {
  /// standalone literal
  auto result1 = get_output("test");
  assert(result1.size() == 2);
  assert(result1[0].type == Literal);
  assert(std::get<Literal>(result1[0].d).content == "test");

  /// standalone constant
  auto result2 = get_output(":)");
  assert(result2.size() == 2);
  assert(result2[0].type == Constant);
  assert(std::get<Constant>(result2[0].d).content == ":)");

  /// full test
  auto result3 = get_output("[size=1]hello :)[/size]\n");
  assert(result3.size() == 12);
  assert(result3[0].type == OpenTagLeft);
  assert(result3[1].type == Literal);
  assert(std::get<Literal>(result3[1].d).content == "size");
  assert(result3[2].type == Equal);
  assert(result3[3].type == Literal);
  assert(std::get<Literal>(result3[3].d).content == "1");
  assert(result3[4].type == TagRight);
  assert(result3[5].type == Literal);
  assert(std::get<Literal>(result3[5].d).content == "hello ");
  assert(result3[6].type == Constant);
  assert(std::get<Constant>(result3[6].d).content == ":)");
  assert(result3[7].type == CloseTagLeft);
  assert(result3[8].type == Literal);
  assert(std::get<Literal>(result3[8].d).content == "size");
  assert(result3[9].type == TagRight);
  assert(result3[10].type == Newline);
  assert(result3[11].type == End);

  return 0;
}