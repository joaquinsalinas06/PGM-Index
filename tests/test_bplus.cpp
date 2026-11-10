#include <cassert>
#include <iostream>
#include "../bplus/static.h"
#include "../bplus/dynamic.h"

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
    DynamicBPlusTree dynamic(keys, 4);
    assert(dynamic.size() == n && dynamic.check());
    for (Key x = 0; x < Key(n + 2); ++x)
      assert(dynamic.lookup(x) == std::lower_bound(keys.begin(), keys.end(), x) - keys.begin());
    assert(count_leaves(tree.root) == (n + 3) / 4);
    assert(tree.ram_bytes >= tree.bytes);
    for (Key x = 0; x < Key(n + 2); ++x)
      assert(tree.lookup(x) == std::lower_bound(keys.begin(), keys.end(), x) - keys.begin());
    assert(tree.lookup(ULLONG_MAX) == n);
  }
  {
    DynamicBPlusTree tree(4);
    std::vector<Key> oracle;
    for (Key x : {Key(9), Key(1), Key(5), Key(5), Key(3), Key(7), Key(0), Key(10)}) {
      tree.insert(x);
      oracle.insert(std::lower_bound(oracle.begin(), oracle.end(), x), x);
      assert(tree.check() && tree.size() == int(oracle.size()));
      for (Key q = 0; q < 12; ++q)
        assert(tree.lookup(q) == std::lower_bound(oracle.begin(), oracle.end(), q) - oracle.begin());
    }
    assert(tree.height == 2);
  }
  bool rejected = false;
  std::vector<Key> empty;
  try { StaticBPlusTree invalid(empty, 1); } catch (std::invalid_argument&) { rejected = true; }
  assert(rejected);
  std::cout << "OK: construcción y búsqueda B+ estático y dinámico\n";
}
