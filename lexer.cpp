//
// Created by TYTY on 2021-01-01 001.
//

#include "lexer.h"
#include <cassert>

namespace bbcode::lexer {

void Lexer::send_buffer(LexType type) {
  if (this->buffer.rdbuf()->in_avail() != 0) {
    auto content = this->buffer.str();
    switch (type) {
      case Constant:
        this->callback(LexItem{
            .type = Constant,
            .line = this->line,
            .chr = this->chr - content.size(),
            .offset = this->offset - content.size(),
            .d = LexItemConstantDetail{
                .content = std::move(content)
            },
        });
        break;

      case Literal:
        this->callback(LexItem{
            .type = Literal,
            .line = this->line,
            .chr = this->chr - content.size() - 1,
            .offset = this->offset - content.size() - 1,
            .d = LexItemLiteralDetail{
                .content = std::move(content)
            }
        });
        break;

      default:break;
    }

    this->buffer.str("");
  }
}

void Lexer::put(i8 c) {
  ++this->chr;
  ++this->offset;

  if (this->left_tag) {
    this->left_tag = false;
    if (c == '/') {
      this->callback(LexItem{
        .type = CloseTagLeft,
        .line = this->line,
        .chr = this->chr - 2,
        .offset = this->offset - 2,
      });
      return;
    }
    else {
      this->callback(LexItem{
        .type = OpenTagLeft,
        .line = this->line,
        .chr = this->chr - 2,
        .offset = this->offset - 2,
      });
    }
  }

  LexType cur_type = Invalid;
  bool constant = false;

  switch (c) {
    case ']':cur_type = TagRight;
      break;
    case '=':cur_type = Equal;
      break;
    case '\n':cur_type = Newline;
      break;
    case '[':this->send_buffer(Literal);
      this->left_tag = true;
      break;
    default:this->buffer.put(c);
      auto[result, valid] = this->cursor.walk(c);
      assert(valid);
      constant = result == trie::Found;
      if (result == trie::NotFound) {
        cursor.reset();
      }
  }

  if (cur_type != Invalid) {
    this->send_buffer(Literal);
    this->callback(LexItem{
        .type = cur_type,
        .line = this->line,
        .chr =this->chr - 1,
        .offset = this->offset - 1,
    });

    if (cur_type == Newline) {
      ++this->line;
      this->chr = 0;
    }
  }
  else if (constant) {
    std::size_t curr = this->cursor.step();
    auto result = this->buffer.str();
    std::size_t extra = result.size() - curr;
    if (extra > 0) {
      this->callback(LexItem{
          .type = Literal,
          .line = this->line,
          .chr = this->chr - result.size(),
          .offset = this->offset - result.size(),
          .d = LexItemLiteralDetail{
              .content = result.substr(0, extra)
          }
      });

      this->callback(LexItem{
          .type = Constant,
          .line = this->line,
          .chr = this->chr - curr,
          .offset = this->offset - curr,
          .d = LexItemConstantDetail{
              .content = result.substr(extra)
          }
      });

      this->buffer.str("");
    }
    else {
      this->send_buffer(Constant);
    }

    this->cursor.reset();
  }
}

}


