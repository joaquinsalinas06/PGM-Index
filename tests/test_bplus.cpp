#include <cassert>
#include <iostream>
#include <random>
#include <type_traits>
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
  assert((!std::is_constructible_v<StaticBPlusTree, std::vector<Key>&&, int>));
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
  std::mt19937_64 rng(42);
  for (int capacity : {4, 5, 16, 64}) {
    for (int pattern = 0; pattern < 4; ++pattern) {
      DynamicBPlusTree tree(capacity);
      std::vector<Key> oracle;
      for (Key i = 0; i < 3000; ++i) {
        Key x = pattern == 0 ? i : pattern == 1 ? 3000 - i : pattern == 2 ? rng() % 30 : rng();
        if (x == ULLONG_MAX) --x;
        tree.insert(x);
        oracle.insert(std::lower_bound(oracle.begin(), oracle.end(), x), x);
        assert(tree.size() == int(oracle.size()) && tree.check());
        for (Key q : {Key(0), x, x + 1, ULLONG_MAX - 1, ULLONG_MAX})
          assert(tree.lookup(q) == std::lower_bound(oracle.begin(), oracle.end(), q) - oracle.begin());
      }
      StaticBPlusTree fixed(oracle, capacity);
      for (Key x : oracle) assert(fixed.lookup(x) == tree.lookup(x));
      bool rejected = false;
      try { tree.insert(ULLONG_MAX); } catch (std::invalid_argument&) { rejected = true; }
      assert(rejected && tree.size() == 3000 && tree.check());
    }
  }
  {
    std::vector<Key> oracle(1000);
    for (int i = 0; i < 1000; ++i) oracle[i] = i / 3;
    DynamicBPlusTree tree(oracle, 4);
    for (int i = 0; i < 500; ++i) {
      Key x = rng() % 500;
      tree.insert(x);
      oracle.insert(std::lower_bound(oracle.begin(), oracle.end(), x), x);
      assert(tree.check() && tree.size() == int(oracle.size()));
      for (Key q : {Key(0), x, x + 1, ULLONG_MAX - 1, ULLONG_MAX})
        assert(tree.lookup(q) == std::lower_bound(oracle.begin(), oracle.end(), q) - oracle.begin());
    }
  }
  for (auto keys : {std::vector<Key>{2, 1}, std::vector<Key>{ULLONG_MAX}}) {
    bool rejected = false;
    try { DynamicBPlusTree invalid(keys, 4); } catch (std::invalid_argument&) { rejected = true; }
    assert(rejected);
    rejected = false;
    try { StaticBPlusTree invalid(keys, 4); } catch (std::invalid_argument&) { rejected = true; }
    assert(rejected);
  }
  bool invalid_capacity = false;
  try { DynamicBPlusTree invalid(1); } catch (std::invalid_argument&) { invalid_capacity = true; }
  assert(invalid_capacity);
  bool rejected = false;
  std::vector<Key> empty;
  try { StaticBPlusTree invalid(empty, 1); } catch (std::invalid_argument&) { rejected = true; }
  assert(rejected);
  std::cout << "OK: construcción y búsqueda B+ estático y dinámico\n";
}
