//
// Created by TYTY on 2021-01-01 001.
//

#ifndef BBCODE__GRAMMAR_H_
#define BBCODE__GRAMMAR_H_

#include <vector>
#include <string>
#include <optional>
#include <variant>
#include <functional>
#include <unordered_set>
#include <unordered_map>

#include "defs.h"
#include "message.h"

namespace bbcode::grammar {

extern std::vector<const char *> CONSTANTS;

enum NodeType {
  /// node with only open tag present. e.g. `[hr]`
  Omission = 0,

  /// node with only content. e.g. `[b]bold text[/b]`
  Simple,

  /// node with both content and parameter. e.g. `[size=1]small text[/size]`
  Parametric,

  /// node with only open tag present, but consider following to be content,
  /// until encountered specific tags. e.g. `[*]` in `[list]`
  Greedy,

  /// node with only literal and newline children
  Verbatim,

  /// Not in tag form, but we considered it as a tag to unify type
  Literal,

  /// Not in tag form, but we considered it as a tag to unify type
  Constant,

  /// Not in tag form, but we considered it as a tag to unify type
  Newline,

  /// Not in tag form, but we considered it as a tag to unify type
  End,

  /// Bad strings. We will preserve their position in tree, but it
  /// should not be rendered
  Invalid = -1,
};

const NodeType MAX_NODE = Verbatim;

struct ValidateResult {
  enum {
    Error = 0,
    Ok = 1
  } result;

  std::string content;
};

typedef std::function<ValidateResult(std::string_view,
                                     const MessageEmitter &)>
    Validator;

struct NodeParametricDescriptor {
  Validator validator;
};

struct NodeGreedyDescriptor {
  std::unordered_set<std::string> terminator;
};

struct NodeDescriptor {
  NodeType type;
  std::string name;
  usize level;
  std::variant<std::monostate,
               std::monostate,
               NodeParametricDescriptor,
               NodeGreedyDescriptor> d;

  std::unordered_set<std::string> children;
  std::unordered_set<std::string> parents;
};

extern std::unordered_multimap<std::string, NodeDescriptor> NODE_MAP;

decltype(NODE_MAP)::iterator get_descriptor(const std::string& name, NodeType type);

}

#endif //BBCODE__GRAMMAR_H_
