#include <cassert>
#include <iostream>
#include "../bplus/static.h"

int count_leaves(StaticBPlusNode* node) {
  if (!node) return 0;
  if (node->leaf) return 1;
  int total = 0;
  assert(node->keys.size() + 1 == node->kids.size());
  for (auto child : node->kids) total += count_leaves(child);
  return total;
}

int main() {
  for (int n : {0, 1, 4, 5, 16, 17, 1000}) {
    std::vector<Key> keys(n);
    for (int i = 0; i < n; ++i) keys[i] = i / 3;
    StaticBPlusTree tree(keys, 4);
    assert(count_leaves(tree.root) == (n + 3) / 4);
    assert(tree.ram_bytes >= tree.bytes);
  }
  bool rejected = false;
  std::vector<Key> empty;
  try { StaticBPlusTree invalid(empty, 1); } catch (std::invalid_argument&) { rejected = true; }
  assert(rejected);
  std::cout << "OK: construcción B+ estático\n";
}
