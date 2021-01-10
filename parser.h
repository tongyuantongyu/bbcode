//
// Created by TYTY on 2021-01-03 003.
//

#ifndef BBCODE__PARSER_H_
#define BBCODE__PARSER_H_

#include <deque>

#include "grammar.h"
#include "lexer.h"

namespace bbcode::parser {

using bbcode::grammar::NodeType;
using namespace bbcode::lexer;

struct Node {
  NodeType type;
  std::string name;
  usize line;
  usize chr;
  usize offset;
  usize span;

  // for *Parametric, this is the parameter.
  // for Literal, this is text data.
  // for other, this should be empty and not used.
  std::string data;

  // for Literal and Newline, this should always been empty;
  std::vector<Node> children;
};

enum ParserState {
  Literal,
  Verbatim,
  TagOpen,
  TagOpenKnown,
  Parameter,
  TagClose,
  TagCloseKnown,
  Done,
};

class Parser {
 private:
  std::deque<Node> stack;
  std::string name;
  std::string parameter;
  ParserState state;
  ParserState before_tag;

  MessageEmitter emitter;
  std::function<void(Node &&)> callback;

 private:
  void finish(Node&& node);
  void finish_back();
  void push_literal(std::string &&s);
  void open_node(lexer::LexItem&& item);
  void close_node(LexItem&& item);
  void push_node(Node&& node);
  void flush_state();
  void as_invalid(Node& node);
  void close();
 public:
  Parser() : Parser([](const auto &&) {}, [](const auto &&) {}) {}
  template <class T1, class T2>
  Parser(T1 callback, T2 emitter) :
      stack{Node {
        .type = NodeType::Literal,
        .line = 0,
        .chr = 0,
        .offset = 0,
        .span = 0
      }},
      state(Literal),
      before_tag(Literal),
      emitter(emitter),
      callback(callback) {}
  void put(LexItem &&item);
};

}

#endif //BBCODE__PARSER_H_
