#include <cassert>
#include <iostream>
#include <random>
#include "../pgm/buffered.h"
#include "../pgm/logarithmic.h"
#include "../bplus/dynamic.h"

template <class Index>
void check_node(std::vector<Key> initial, int capacity, int pattern) {
  std::vector<Key> oracle = initial;
  Index index(initial, 4, capacity);
  DynamicBPlusTree bplus(initial, 4);
  auto probe = [&](Key x) {
    int want = std::lower_bound(oracle.begin(), oracle.end(), x) - oracle.begin();
    assert(index.lookup(x) == want && bplus.lookup(x) == want);
  };
  assert(index.check());
  for (Key x : {Key(0), Key(1), ULLONG_MAX - 1, ULLONG_MAX}) probe(x);
  std::mt19937_64 rng(42);
  for (Key i = 0; i < 1500; ++i) {
    Key x = pattern == 0 ? i : pattern == 1 ? 1500 - i : pattern == 2 ? rng() % 30 : rng();
    if (x == ULLONG_MAX) --x;
    index.insert(x);
    bplus.insert(x);
    oracle.insert(std::lower_bound(oracle.begin(), oracle.end(), x), x);
    assert(index.size() == int(oracle.size()) && index.check() && bplus.check());
    for (Key q : {Key(0), x, x + 1, ULLONG_MAX - 1, ULLONG_MAX, Key(rng())}) probe(q);
    if (i % 100 == 0) for (Key q = 0; q < 32; ++q) probe(q);
  }
  for (Key x : oracle) probe(x);
  assert(index.size_in_bytes() > 0 && index.ram_bytes() >= index.size_in_bytes());
  bool rejected = false;
  try { index.insert(ULLONG_MAX); } catch (std::invalid_argument&) { rejected = true; }
  assert(rejected && index.size() == int(oracle.size()) && index.check());
}

template <class Index>
void invalid_inputs() {
  for (auto keys : {std::vector<Key>{2, 1}, std::vector<Key>{ULLONG_MAX}}) {
    bool rejected = false;
    try { Index index(keys, 4, 4); } catch (std::invalid_argument&) { rejected = true; }
    assert(rejected);
  }
  for (auto params : {std::pair<int, int>{0, 4}, {4, 1}, {4, 0}}) {
    bool rejected = false;
    try { Index index({}, params.first, params.second); }
    catch (std::invalid_argument&) { rejected = true; }
    assert(rejected);
  }
}

int main() {
  for (int cap : {2, 3, 16, 256}) {
    for (int pattern = 0; pattern < 4; ++pattern) {
      for (auto initial : {std::vector<Key>{}, std::vector<Key>{0, 1, 1, 3, 5000, ULLONG_MAX - 1}}) {
        check_node<BufferedPGM>(initial, cap, pattern);
        check_node<LogarithmicPGM>(initial, cap, pattern);
      }
    }
  }
  invalid_inputs<BufferedPGM>();
  invalid_inputs<LogarithmicPGM>();
  BufferedPGM buffered({1, 2, 3, 5000}, 4, 2);
  LogarithmicPGM logarithmic({1, 2, 3, 5000}, 4, 2);
  buffered.insert(4); logarithmic.insert(4);
  assert(buffered.lookup(5000) == 4 && logarithmic.lookup(5000) == 4);
  assert(buffered.rebuilds == 0 && logarithmic.merges == 0);
  buffered.insert(5); logarithmic.insert(5);
  assert(buffered.rebuilds == 1 && logarithmic.merges == 1);
  assert(buffered.lookup(5000) == 5 && logarithmic.lookup(5000) == 5);
  assert(buffered.moved_keys == 6 && logarithmic.moved_keys == 2);
  std::cout << "OK: PGM con buffer y niveles frente a vector y B+ dinámico\n";
}
