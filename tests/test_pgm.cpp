#include <cassert>
#include <iostream>
#include <random>
#include <type_traits>
#include "../pgm/static.h"

void check_node(std::vector<Key> keys) {
  for (int eps : {1, 4, 16, 64, 256}) {
    PGMIndex pgm(keys, eps);
    auto probe = [&](Key x) {
      int want = std::lower_bound(keys.begin(), keys.end(), x) - keys.begin();
      assert(pgm.lookup(x) == want);
      auto window = pgm.approx(x);
      assert(0 <= window.lo && window.lo <= window.hi && window.hi <= int(keys.size()));
      assert(window.lo <= want && want <= window.hi);
      if (want < int(keys.size()) && keys[want] == x) assert(want < window.hi);
    };
    for (Key x : {Key(0), Key(1), ULLONG_MAX - 1, ULLONG_MAX}) probe(x);
    for (Key x : keys) {
      probe(x);
      if (x) probe(x - 1);
      probe(x + 1);
    }
    std::mt19937_64 rng(42);
    for (int i = 0; i < 2000; ++i) probe(rng());
  }
}

int main() {
  assert((!std::is_constructible_v<PGMIndex, std::vector<Key>&&, int>));

  check_node({}); check_node({0}); check_node({10}); check_node({1, 1, 1});
  check_node({0, 0, 1, 1, 2, 100, 100});
  check_node({ULLONG_MAX - 5, ULLONG_MAX - 2, ULLONG_MAX - 1});
  std::mt19937_64 rng(42);
  for (int kind = 0; kind < 4; ++kind) {
    std::vector<Key> keys;
    Key cur = 0;
    for (Key i = 1; i <= 10000; ++i) {
      Key low = i & -i;
      cur += kind == 0 ? 1 : kind == 1 ? 1000 : kind == 2 ? low * low : 1 + rng() % 100000;
      keys.push_back(cur);
    }
    check_node(keys);
    PGMIndex p(keys, 8);
    if (kind < 2) assert(p.seg_count() == 1);
    if (kind == 2) { assert(p.seg_count() > 10); assert(p.height() > 1); }
  }
  std::vector<Key> uniform(10000), duplicates(10000);
  for (int i = 0; i < 10000; ++i) {
    uniform[i] = rng() >> 1;
    duplicates[i] = rng() % 32;
  }
  std::sort(uniform.begin(), uniform.end());
  std::sort(duplicates.begin(), duplicates.end());
  check_node(uniform); check_node(duplicates);
  check_node(std::vector<Key>(1000, ULLONG_MAX - 1));
  for (auto keys : {std::vector<Key>{2, 1}, std::vector<Key>{1, ULLONG_MAX}}) {
    bool rejected = false;
    try { PGMIndex p(keys, 4); } catch (std::invalid_argument&) { rejected = true; }
    assert(rejected);
  }
  std::vector<Key> keys{1};
  bool rejected = false;
  try { PGMIndex p(keys, 0); } catch (std::invalid_argument&) { rejected = true; }
  assert(rejected);
  std::cout << "OK: PGM frente a lower_bound y ventanas\n";
}
