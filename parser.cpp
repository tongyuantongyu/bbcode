//
// Created by TYTY on 2021-01-03 003.
//

#include "parser.h"
#include <cassert>

namespace bbcode::parser {

void Parser::as_invalid(Node& node) {
  this->emitter(Message {
      .severity = Warning,
      .line = node.line,
      .chr = node.chr,
      .offset = node.offset,
      .span = node.span,
      .name = "unexpected-node",
      .message = "Unexpected node marked as Invalid.",
  });

  switch (node.type) {
    case grammar::Newline:
      node.data = "\n";
      [[fallthrough]];
    case grammar::Omission:
    case grammar::Greedy:
    case grammar::Verbatim:
    case grammar::Literal:
    case grammar::Constant:
    case grammar::Simple:
      node.type = grammar::Invalid;
      break;
    case grammar::Parametric:
      node.name = node.name + "=" + node.data;
      node.data.clear();
      node.type = grammar::Invalid;
      break;
    case grammar::End:
    case grammar::Invalid:
      break;
  }
}

void Parser::finish(Node&& node) {
  if (node.type == grammar::Literal && node.data.empty()) {
    return;
  }

  if (this->stack.empty()) {
    if (node.type >= 0 && node.type <= grammar::MAX_NODE) {
      auto descriptor = grammar::get_descriptor(node.name, node.type);
      assert(descriptor != grammar::NODE_MAP.end());
      if (!descriptor->second.parents.empty()) {
        as_invalid(node);
      }
    }
    this->callback(std::move(node));
  } else {
    const auto& parent = this->stack.back();
    auto parent_descriptor = grammar::get_descriptor(parent.name, parent.type);
    assert(parent_descriptor != grammar::NODE_MAP.end());

    if (!parent_descriptor->second.children.empty() &&
        !parent_descriptor->second.children.contains(node.name)) {
      as_invalid(node);
    } else if (node.type >= 0 && node.type <= grammar::MAX_NODE) {
      auto child_descriptor = grammar::get_descriptor(node.name, node.type);
      assert(child_descriptor != grammar::NODE_MAP.end());

      if (!child_descriptor->second.parents.empty() &&
          !child_descriptor->second.parents.contains(parent.name)) {
        as_invalid(node);
      }
    }

    this->stack.back().span += node.span;
    this->stack.back().children.push_back(std::move(node));
  }
}

void Parser::finish_back() {
  if (this->stack.empty()) {
    return;
  }

  auto node = std::move(this->stack.back());
  this->stack.pop_back();

  this->finish(std::move(node));
}

void Parser::push_literal(std::string &&s) {
  if (this->state == Parameter) {
    this->parameter += s;
    return;
  }

  assert(this->state == Literal || this->state == Verbatim);

  if (this->stack.back().type == grammar::Literal) {
    this->stack.back().data += s;
    this->stack.back().span = this->stack.back().data.size();
  } else {
    auto back = std::move(this->stack.back());
    this->stack.pop_back();
    auto literal = Node {
      .type = NodeType::Literal,
      .offset = back.offset + back.span,
      .span = s.size()
    };
    if (back.type == grammar::Newline) {
      literal.line = back.line + 1;
      literal.chr = 0;
    } else {
      literal.line = back.line;
      literal.chr = back.chr + back.span;
    }
    literal.data = s;
    this->finish(std::move(back));
    this->stack.push_back(std::move(literal));
  }
}

void Parser::open_node(lexer::LexItem&& item) {
  bool matched = false;
  if (this->state == TagOpenKnown) {
    auto match = grammar::NODE_MAP.equal_range(this->name);
    for (auto it = match.first; it != match.second; ++it) {
      if (it->second.type != grammar::Parametric) {
        this->finish_back();
        if (!this->stack.empty() && this->stack.back().type == grammar::Greedy &&
            it->second.type == grammar::Greedy) {
          auto parent = ++this->stack.rbegin();
          auto parent_descriptor = grammar::get_descriptor(parent->name, parent->type);
          if (parent_descriptor->second.children.contains(this->name)) {
            this->finish_back();
          }
        }

        this->stack.push_back(Node {
            .type = it->second.type,
            .name = this->name,
            .line = item.line,
            .chr = item.chr - this->name.size() - 1,
            .offset = item.offset - this->name.size() - 1,
            .span = this->name.size() + 2,
        });

        if (it->second.type == grammar::Omission) {
          this->push_node(Node {
              .type = NodeType::Literal,
              .line = item.line,
              .chr = item.chr + 1,
              .offset = item.offset + 1,
              .span = 0,
              .data = "",
          });
        } else {
          this->stack.push_back(Node {
              .type = NodeType::Literal,
              .line = item.line,
              .chr = item.chr + 1,
              .offset = item.offset + 1,
              .span = 0,
              .data = "",
          });
        }


        switch (it->second.type) {
          case grammar::Omission:
          case grammar::Simple:
          case grammar::Greedy:
            this->state = Literal;
            break;
          case grammar::Verbatim:
            this->state = Verbatim;
            break;
          case grammar::Literal:
          case grammar::Constant:
          case grammar::Newline:
          case grammar::End:
          case grammar::Invalid:
          case grammar::Parametric:
            assert(false);
        }

        this->name.clear();
        this->parameter.clear();
        return;
      } else {
        matched = true;
      }
    }

    // unknown tag
    std::stringstream ss;
    std::string literal = "[" + this->name + "]";
    if (matched) {
      ss << "Tag with unmatched type `" << this->name << "` interpreted as normal text.";
      this->emitter(Message {
          .severity = Warning,
          .line = item.line,
          .chr = item.chr - this->name.size() - 1,
          .offset = item.offset - this->name.size() - 1,
          .span = literal.size(),
          .name = "unmatched-tag-type",
          .message = ss.str(),
      });
    } else {
      ss << "Unknown open tag `" << this->name << "` interpreted as normal text.";
      this->emitter(Message {
          .severity = Tidy,
          .line = item.line,
          .chr = item.chr - this->name.size() - 1,
          .offset = item.offset - this->name.size() - 1,
          .span = literal.size(),
          .name = "unknown-open-tag",
          .message = ss.str(),
      });
    }

    this->state = this->before_tag;
    this->push_literal(std::move(literal));
  } else if (this->state == Parameter) {
    auto match = grammar::NODE_MAP.equal_range(this->name);
    for (auto it = match.first; it != match.second; ++it) {
      if (it->second.type == grammar::Parametric) {
        auto result = std::get<NodeType::Parametric>(it->second.d)
            .validator(this->parameter, this->emitter);
        if (result.result) {
          this->finish_back();
          this->stack.push_back(Node {
              .type = NodeType::Parametric,
              .name = this->name,
              .line = item.line,
              .chr = item.chr - this->name.size() - this->parameter.size() - 2,
              .offset = item.offset - this->name.size() - this->parameter.size() - 2,
              .span = this->name.size() + this->parameter.size() + 3,
              .data = std::move(result.content)
          });

          this->stack.push_back(Node {
              .type = NodeType::Literal,
              .line = item.line,
              .chr = item.chr + 1,
              .offset = item.offset + 1,
              .span = 0,
              .data = "",
          });
          this->state = Literal;
          this->name.clear();
          this->parameter.clear();
          return;
        } else {
          std::stringstream ss;
          ss << "`" << this->parameter << "` is not valid parameter of " << this->name << " tag. This tag will be decayed to normal text.";
          this->emitter(Message {
            .severity = Error,
            .line = item.line,
            .chr = item.chr - this->parameter.size(),
            .offset = item.offset - this->parameter.size(),
            .span = this->parameter.size(),
            .name = "bad-parameter",
            .message = ss.str(),
          });

          this->state = this->before_tag;
          this->push_literal("[" + this->name + "=" + this->parameter + "]");
          this->name.clear();
          this->parameter.clear();
          return;
        }
      } else {
        matched = true;
      }
    }

    // unknown tag
    std::stringstream ss;
    std::string literal = "[" + this->name + "=" + this->parameter + "]";

    if (matched) {
      ss << "Tag with unmatched type `" << this->name << "` interpreted as normal text.";
      this->emitter(Message {
          .severity = Warning,
          .line = item.line,
          .chr = item.chr - this->name.size() - this->parameter.size() - 2,
          .offset = item.offset - this->name.size() - this->parameter.size() - 2,
          .span = literal.size(),
          .name = "unknown-tag-type",
          .message = ss.str(),
      });
    } else {
      ss << "Unknown open tag `" << this->name << "` interpreted as normal text.";
      this->emitter(Message {
          .severity = Tidy,
          .line = item.line,
          .chr = item.chr - this->name.size() - this->parameter.size() - 2,
          .offset = item.offset - this->name.size() - this->parameter.size() - 2,
          .span = literal.size(),
          .name = "unknown-open-tag",
          .message = ss.str(),
      });
    }

    this->state = this->before_tag;
    this->push_literal(std::move(literal));
  }

  this->name.clear();
  this->parameter.clear();
}

void Parser::close_node(lexer::LexItem &&item) {
  if (this->stack.size() > 1 && (++this->stack.rbegin())->type == grammar::Verbatim) {
    if (this->name != (++this->stack.rbegin())->name) {
      this->state = Verbatim;
      this->push_literal("[/" + this->name + "]");
    } else {
      this->stack.back().span += this->name.size() + 3;
      this->finish_back();
      this->finish_back();
      this->state = Literal;
      this->stack.push_back(Node {
          .type = NodeType::Literal,
          .line = item.line,
          .chr = item.chr + 1,
          .offset = item.offset + 1,
          .span = 0,
          .data = "",
      });
    }

    this->name.clear();
    this->parameter.clear();
    return;
  }

  auto match_cur = grammar::NODE_MAP.equal_range(this->name);
  if (match_cur.first == match_cur.second ||
  std::all_of(match_cur.first, match_cur.second, [](const auto& a) {
    return a.second.type == grammar::Omission || a.second.type == grammar::Greedy;
  })) {
    std::stringstream ss;
    ss << "Unknown close tag `" << this->name << "` ignored.";
    this->emitter(Message {
        .severity = Tidy,
        .line = item.line,
        .chr = item.chr - this->name.size() - 2,
        .offset = item.offset - this->name.size() - 2,
        .span = this->name.size() + 3,
        .name = "unknown-close-tag",
        .message = ss.str(),
    });

    this->state = this->before_tag;
    this->name.clear();
    this->parameter.clear();
    return;
  }

  bool hold_back = false;
  Node back;
  switch (this->stack.back().type) {
    case grammar::Literal:
    case grammar::Constant:
    case grammar::Newline:
      back = this->stack.back();
      this->stack.pop_back();
      hold_back = true;
      break;
    default:
      break;
  }

  if (this->stack.empty()) {
    std::stringstream ss;
    ss << "Unpaired close tag `" << this->name << "` ignored.";
    this->emitter(Message {
        .severity = Warning,
        .line = item.line,
        .chr = item.chr - this->name.size() - 2,
        .offset = item.offset - this->name.size() - 2,
        .span = this->name.size() + 3,
        .name = "unpaired-close-tag",
        .message = ss.str(),
    });

    this->state = this->before_tag;
    if (hold_back) {
      this->stack.push_back(std::move(back));
    } else {
      this->stack.push_back(Node {
          .type = NodeType::Literal,
          .line = item.line,
          .chr = item.chr + 1,
          .offset = item.offset + 1,
          .span = 0,
          .data = "",
      });
    }

    this->name.clear();
    this->parameter.clear();
    return;
  }

  auto it = this->stack.rbegin();
  const auto end = this->stack.size() > 8 ? it + 8 : this->stack.rend();
  for (; it < end; ++it) {
    if (it->name == this->name) {
      break;
    }

    if (it->type == grammar::Greedy) {
      auto descriptor = grammar::get_descriptor(it->name, grammar::Greedy);
      assert(descriptor != grammar::NODE_MAP.end());
      if (!std::get<NodeType::Greedy>(descriptor->second.d).terminator.contains(this->name)) {
        break;
      }
    }
  }

  if (it == end) {
    std::stringstream ss;
    ss << "Unpaired close tag `" << this->name << "` ignored.";
    this->emitter(Message {
        .severity = Warning,
        .line = item.line,
        .chr = item.chr - this->name.size() - 2,
        .offset = item.offset - this->name.size() - 2,
        .span = this->name.size() + 3,
        .name = "unpaired-close-tag",
        .message = ss.str(),
    });

    this->state = this->before_tag;
    if (hold_back) {
      this->stack.push_back(std::move(back));
    } else {
      this->stack.push_back(Node {
          .type = NodeType::Literal,
          .line = item.line,
          .chr = item.chr + 1,
          .offset = item.offset + 1,
          .span = 0,
          .data = "",
      });
    }

    this->name.clear();
    this->parameter.clear();
    return;
  }

  this->finish(std::move(back));
  ++it;

  for (auto it2 = this->stack.rbegin(); it2 != it; ++it2) {
    if (it2->type == grammar::Greedy) {
      auto descriptor = grammar::get_descriptor(it2->name, it2->type);
      if (std::get<grammar::Greedy>(descriptor->second.d).terminator.contains(this->name)) {
        goto FINISH;
      }
    }

    if (it2 + 1 != it) {
      std::stringstream ss;
      ss << "Missing close tag for `" << it2->name << "` node.";
      this->emitter(Message {
          .severity = Warning,
          .line = it2->line,
          .chr = it2->chr,
          .offset = it2->offset,
          .span = it2->span,
          .name = "missing-close-tag",
          .message = ss.str(),
      });
    } else {
      this->stack.back().span += this->name.size() + 3;
    }

    FINISH:
    this->finish_back();
  }

  if (this->stack.empty()) {
    this->state = Literal;
  } else {
    if (this->stack.back().type == grammar::Verbatim) {
      this->state = Verbatim;
    } else {
      this->state = Literal;
    }
  }

  this->stack.push_back(Node {
      .type = NodeType::Literal,
      .line = item.line,
      .chr = item.chr + 1,
      .offset = item.offset + 1,
      .span = 0,
      .data = "",
  });

  this->name.clear();
  this->parameter.clear();
}

void Parser::flush_state() {
  auto old_state = this->state;
  this->state = Literal;
  std::string literal;
  switch (old_state) {
    case TagOpen: {
      literal = "[";
      break;
    }
    case TagOpenKnown: {
      literal = "[" + this->name;
      break;
    }
    case Parameter: {
      literal = "[" + this->name + "=" + this->parameter;
      break;
    }
    case TagClose: {
      literal = "[/";
      break;
    }
    case TagCloseKnown: {
      literal = "[/" + this->name;
      break;
    }
    default:
      assert(false);
  }

  std::stringstream ss;
  ss << "Incomplete tag `" << literal << "` interpreted as normal text.";
  auto& back = this->stack.back();
  auto message = Message {
      .severity = Warning,
      .offset = back.offset - this->parameter.size(),
      .span = literal.size(),
      .name = "incomplete-tag",
      .message = ss.str(),
  };

  if (back.type == grammar::Newline) {
    message.line = back.line + 1;
    message.chr = 0;
  } else {
    message.line = back.line;
    message.chr = back.chr + back.span;
  }

  this->emitter(std::move(message));
  this->push_literal(std::move(literal));
}

void Parser::push_node(Node&& node) {
  if (this->stack.back().type == grammar::Literal && node.type == grammar::Literal) {
    this->stack.back().data += node.data;
    this->stack.back().span = this->stack.back().data.size();
  } else {
    this->finish_back();
    this->stack.push_back(std::move(node));
  }
}

void Parser::close() {
  if (this->state == Done) {
    return;
  }

  std::stringstream ss;
  while (!this->stack.empty()) {
    auto& back = this->stack.back();

    if (back.type >= 0 && back.type <= grammar::MAX_NODE) {
      ss << "Missing close tag for `" << back.name << "` node.";
      this->emitter(Message {
          .severity = Warning,
          .line = back.line,
          .chr = back.chr,
          .offset = back.offset,
          .span = back.span,
          .name = "missing-close-tag",
          .message = ss.str(),
      });

      ss.str("");
    }

    this->finish_back();
  }

  this->callback(Node {
    .type = NodeType::End,
  });

  this->state = Done;
}

void Parser::put(LexItem &&item) {
  if (this->state == Done) {
    return;
  }

  switch (item.type) {
    case lexer::Newline: {
      switch (this->state) {
        case TagOpen:
        case TagOpenKnown:
        case Parameter:
        case TagClose:
        case TagCloseKnown:
          this->flush_state();
          [[fallthrough]];
        case Literal:
        case Verbatim: {
          this->push_node(Node {
              .type = grammar::Newline,
              .line = item.line,
              .chr = item.chr,
              .offset = item.offset,
              .span = 1,
              .data = "\n"
          });
          break;
        }
        case Done:
          assert(false);
          return;
      }
      break;
    }
    case lexer::OpenTagLeft: {
      switch (this->state) {
        default:
          this->flush_state();
          [[fallthrough]];
        case Literal: {
          this->before_tag = Literal;
          this->state = TagOpen;
          break;
        }
        case Verbatim: {
          this->push_literal("[");
          return;
        }
        case Done:
          assert(false);
          return;
      }
      break;
    }
    case lexer::CloseTagLeft: {
      switch (this->state) {
        default:
          this->flush_state();
          [[fallthrough]];
        case Literal:
        case Verbatim: {
          this->before_tag = this->state;
          this->state = TagClose;
          break;
        }
        case Done:
          assert(false);
          return;
      }
      break;
    }
    case lexer::TagRight: {
      switch (this->state) {
        case TagOpen:
        case TagClose:
          this->flush_state();
          [[fallthrough]];
        case Literal:
        case Verbatim: {
          this->push_literal("]");
          break;
        }
        case TagOpenKnown:
        case Parameter: {
          this->open_node(std::move(item));
          break;
        }
        case TagCloseKnown: {
          this->close_node(std::move(item));
          break;
        }
        case Done:
          assert(false);
          return;
      }
      break;
    }
    case lexer::Equal: {
      switch (this->state) {
        case TagOpenKnown: {
          this->state = Parameter;
          break;
        }
        case Literal:
        case Verbatim:
        case TagOpen:
        case Parameter: {
          this->push_literal("=");
          break;
        }
        case TagClose:
        case TagCloseKnown: {
          this->flush_state();
          this->push_literal("=");
          break;
        }
        case Done:
          assert(false);
          return;
      }
      break;
    }
    case lexer::Constant: {
      auto content = std::move(std::get<lexer::Constant>(item.d).content);
      switch (this->state) {
        case TagOpen:
        case TagOpenKnown:
        case Parameter:
        case TagClose:
        case TagCloseKnown:
          this->flush_state();
        [[fallthrough]];
        case Literal: {
          this->push_node(Node {
            .type = NodeType::Constant,
            .line = item.line,
            .chr = item.chr,
            .offset = item.offset,
            .span = content.size(),
            .data = content
          });
          break;
        }
        case Verbatim: {
          this->push_literal(std::move(content));
          break;
        }
        case Done:
          assert(false);
          return;
      }
      break;
    }
    case lexer::Literal: {
      auto content = std::move(std::get<lexer::Literal>(item.d).content);
      switch (this->state) {
        case Literal:
        case Verbatim: {
          this->push_literal(std::move(content));
          break;
        }
        case TagOpen: {
          this->name = content;
          this->state = TagOpenKnown;
          break;
        }
        case TagClose: {
          this->name = content;
          this->state = TagCloseKnown;
          break;
        }
        case TagOpenKnown:
        case TagCloseKnown: {
          this->name += content;
          break;
        }
        case Parameter: {
          this->parameter += content;
          break;
        }
        case Done:
          assert(false);
          return;
      }
      break;
    }
    case lexer::End: {
      this->close();
      break;
    }
    case lexer::Invalid:
      assert(false);
      return;
  }
}

}
