#include <iostream>
#include "trie.h"

int main() {
  Trie trie;
  trie.insert("aaa");
  trie.insert("aba");

  auto cursor = trie.get_cursor();
  std::cout << cursor.walk('a').first << " "
            << cursor.walk('b').first << " "
            << cursor.walk('a').first;
}
