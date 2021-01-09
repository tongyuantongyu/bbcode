//
// Created by TYTY on 2020-12-31 031.
//

#include "trie.h"
#include "grammar.h"

#include <utility>

namespace bbcode::trie {

Trie Trie::instance {};
std::once_flag Trie::inited {};

TrieCursor Trie::get_cursor() {
  std::call_once(Trie::inited, []() {
    for (const auto& c : bbcode::grammar::CONSTANTS) {
      if (!Trie::instance.insert(c)) {
        throw std::runtime_error("Bad constant value.");
      }
    }
  });
  return TrieCursor(Trie::instance.root);
}

bool Trie::insert(const std::string_view &s) {
  if (s.length() == 0) {
    return false;
  }

  auto s_vec = str_to_hex_vec(s);
  auto current = this->root;
  usize position = 0;
  usize index = 0;

  while (position < s_vec.size()) {
    switch (current->type) {
      case Empty:goto INSERT;

      case Single:return false;

      case Forward: {
        const auto &route = std::get<Forward>(current->d).route;
        index = 0;

        while (true) {
          if (index == route.size()) {
            position += index;
            current = std::get<Forward>(current->d).node;
            break;
          }

          if (index + position == s_vec.size()
              || route[index] != s_vec[index + position]) {
            position += index;
            goto INSERT;
          }

          index += 1;
        }
      }

      break;
      case Branch: {
        auto node = (*std::get<Branch>(current->d).children)[s_vec[position]];
        position += 1;
        switch (node->type) {
          case Empty:current = node;
            goto INSERT;
          case Single:return false;
          default:current = node;
        }
      }
    }
  }

  INSERT:
  switch (current->type) {
    case Empty:
      if (position + 1 == s_vec.size()) {
        current->type = Single;
      }
      else {
        current->type = Forward;
        TrieNodeForwardDetail d;
        d.node->type = Single;
        std::copy(s_vec.begin() + position,
                  s_vec.end(),
                  std::back_inserter(d.route));
        current->d = std::move(d);
      }

      break;
    case Single:return false;
    case Forward: {
      auto &route = std::get<Forward>(current->d).route;
      auto next = std::get<Forward>(current->d).node;

      auto branch = std::make_shared<TrieNode>();
      branch->type = Branch;
      TrieNodeBranchDetail d;

      if (position + 1 == s_vec.size()) {
        (*d.children)[s_vec[position]]->type = Single;
      }
      else {
        (*d.children)[s_vec[position]]->type = Forward;
        TrieNodeForwardDetail di;
        std::copy(s_vec.begin() + position + 1,
                  s_vec.end(),
                  std::back_inserter(di.route));
        di.node->type = Single;
        (*d.children)[s_vec[position]]->d = std::move(di);
      }

      if (index + 1 >= route.size()) {
        (*d.children)[route[index]] = next;
      }
      else {
        (*d.children)[route[index]]->type = Forward;
        TrieNodeForwardDetail di;
        std::copy(route.begin() + index + 1,
                  route.end(),
                  std::back_inserter(di.route));
        di.node = next;
        (*d.children)[route[index]]->d = std::move(di);
      }
      route.resize(index);

      if (index == 0) {
        current->type = Branch;
        current->d = std::move(d);
      } else {
        branch->d = std::move(d);
        std::get<Forward>(current->d).node = branch;
      }
    }

      break;
    case Branch:
      if (position + 1 >= s_vec.size()) {
        (*std::get<Branch>(current->d).children)[s_vec[position]]->type =
            Single;
      }
      else {
        auto &target =
            (*std::get<Branch>(current->d).children)[s_vec[position]];
        target->type = Forward;
        TrieNodeForwardDetail d;
        std::copy(s_vec.begin() + position,
                  s_vec.end(),
                  std::back_inserter(d.route));
        target->d = std::move(d);
      }

      break;
  }

  return true;
}

Trie::Trie() noexcept {
  this->root = std::make_shared<TrieNode>();
}

CursorState TrieCursor::walk_step(u8 step) {
  switch (this->node->type) {
    case Empty:return NotFound;

    case Single:return Found;

    case Forward: {
      const auto &route = std::get<Forward>(this->node->d).route;
      const usize r_len = route.size();
      const usize v_len = this->verified.size();

      if (r_len <= v_len) {
        return NotFound;
      }

      if (route[v_len] == step) {
        if (v_len + 1 == r_len) {
          this->verified.clear();
          this->node = std::get<Forward>(this->node->d).node;
          switch (this->node->type) {
            case Empty:return NotFound;
            case Single:return Found;
            default:return Walking;
          }
        }
        else {
          this->verified.push_back(step);
          return Walking;
        }
      }
      else {
        return NotFound;
      }
    }

    case Branch: {
      this->node = (*std::get<Branch>(this->node->d).children)[step];
      switch (this->node->type) {
        case Empty:return NotFound;
        case Single:return Found;
        default:return Walking;
      }
    }
  }

  return NotFound;
}

std::pair<CursorState, bool> TrieCursor::walk(i8 c) {
  if (this->state != Walking) {
    return std::make_pair(this->state, false);
  }

  const u8 high = u8(c) >> 4;
  const u8 low = u8(c) & 0xf;

  this->state = this->walk_step(high);
  if (this->state != Walking) {
    return std::make_pair(this->state, true);
  }

  this->state = this->walk_step(low);
  if (this->state != NotFound) {
    this->_step++;
  }
  return std::make_pair(this->state, true);
}

TrieCursor::TrieCursor(std::shared_ptr<TrieNode> node) : root(std::move(node)), node(root), _step(0) {
  switch (this->node->type) {
    case Empty:
      this->state = NotFound;
      break;
    case Single:
      this->state = Found;
      break;
    default:
      this->state = Walking;
      break;
  }
}

void TrieCursor::reset() {
  this->node = this->root;
  this->verified.clear();
  this->_step = 0;

  switch (this->node->type) {
    case Empty:
      this->state = NotFound;
      break;
    case Single:
      this->state = Found;
      break;
    default:
      this->state = Walking;
      break;
  }
}

}
