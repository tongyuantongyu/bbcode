//
// Created by TYTY on 2020-12-31 031.
//

#ifndef BBCODE__TRIE_H_
#define BBCODE__TRIE_H_

#include "defs.h"
#include <vector>
#include <memory>
#include <variant>
#include <mutex>

namespace bbcode {
namespace trie {

enum TrieNodeType {
  Empty = 0,
  Single,
  Forward,
  Branch
};

struct TrieNode;

struct TrieNodeForwardDetail {
  std::vector<u8> route;
  std::shared_ptr<TrieNode> node;

  TrieNodeForwardDetail() : node(std::make_shared<TrieNode>()) {}
};

struct TrieNodeBranchDetail {
  std::shared_ptr<std::vector<std::shared_ptr<TrieNode>>> children;

  TrieNodeBranchDetail() {
    children = std::make_shared<decltype(children)::element_type>(16);
    for (auto& child : *children) {
      child = std::make_shared<TrieNode>();
    }
  }
};

struct TrieNode {
  TrieNodeType type;
  std::variant<std::monostate, std::monostate, TrieNodeForwardDetail, TrieNodeBranchDetail> d;

  TrieNode(): type(Empty) {}
};

class TrieCursor;

class Trie {
 private:
  std::shared_ptr<TrieNode> root;
  static Trie instance;
  static std::once_flag inited;
  Trie() noexcept;

 public:
  bool insert(const std::string_view &s);
  static TrieCursor get_cursor();
};

inline std::vector<u8> str_to_hex_vec(const std::string_view &s) {
  std::vector<u8> v;
  v.reserve(s.size() * 2);
  for (const auto& c : s) {
    v.push_back(c >> 4);
    v.push_back(c & 0xf);
  }

  return v;
}

enum CursorState {
  Found,
  NotFound,
  Walking
};

class TrieCursor {
 private:
  std::shared_ptr<TrieNode> root;
  std::shared_ptr<TrieNode> node;
  std::vector<u8> verified;
  std::size_t _step;
 public:
  CursorState state;

 private:
  CursorState walk_step(u8 step);
  explicit TrieCursor(std::shared_ptr<TrieNode> node);
  friend TrieCursor Trie::get_cursor();

 public:
  std::pair<CursorState, bool> walk(i8 c);
  void reset();
  [[nodiscard]] std::size_t step() const noexcept { return this->_step; }
};

}

using Trie = trie::Trie;
using TrieCursor = trie::TrieCursor;

}

#endif //BBCODE__TRIE_H_
