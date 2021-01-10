//
// Created by TYTY on 2021-01-01 001.
//

#ifndef BBCODE__LEXER_H_
#define BBCODE__LEXER_H_

#include "defs.h"
#include "trie.h"
#include <vector>
#include <memory>
#include <variant>
#include <sstream>
#include <functional>

namespace bbcode::lexer {

enum LexType {
  /// "\n"
  Newline = 0,

  /// "["
  OpenTagLeft,

  /// "[/"
  CloseTagLeft,

  /// "]"
  TagRight,

  /// "="
  Equal,

  /// ":)"
  Constant,

  /// "hello"
  Literal,

  /// End
  End,

  /// invalid
  Invalid = -1
};

struct LexItemConstantDetail {
  std::string content;
};

struct LexItemLiteralDetail {
  std::string content;
};

struct LexItem {
  LexType type;
  usize line;
  usize chr;
  usize offset;
  std::variant<std::monostate,
               std::monostate,
               std::monostate,
               std::monostate,
               std::monostate,
               LexItemConstantDetail,
               LexItemLiteralDetail> d;
};

class Lexer {
 private:
  std::stringstream buffer;
  bbcode::TrieCursor cursor;
  std::function<void(LexItem &&)> callback;
  bool left_tag;
  usize line;
  usize chr;
  usize offset;

 private:
  void send_buffer(LexType type);
 public:
  Lexer() : Lexer([](const auto &&) {}) {}
  template<class T>
  explicit Lexer(T callback)
      : cursor(Trie::get_cursor()),
        callback(callback),
        left_tag(false),
        line(0),
        chr(0),
        offset(0) {}
  void put(i8 c);
  void finish() {
    ++this->chr;
    this->send_buffer(Literal);
    this->callback(LexItem{
      .type = End,
      .line = this->line,
      .chr =this->chr - 1,
      .offset = this->offset - 1,
    });
  }
};

}

#endif //BBCODE__LEXER_H_

