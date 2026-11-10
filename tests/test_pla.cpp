#include <cassert>
#include <cmath>
#include <iostream>
#include "../pgm/pla.h"

int main() {
  Point a{0, 0}, b{2, 2}, c{3, 5};
  assert(a.cross(b, c) > 0);
  Slope low{4, 2}, high{4, 3};
  assert(low < high && high > low);
  Segment s{100, 0.5f, 0};
  assert(s.predict(110) == 5);
  for (int gap : {1, 1000}) {
    std::vector<Key> keys(10000);
    for (int i = 0; i < 10000; ++i) keys[i] = 100 + Key(i) * gap;
    std::vector<Segment> segments;
    PLA pla(8);
    assert(pla.make_segs(keys, keys.back(), segments) == 1);
    for (int i = 0; i < 10000; ++i)
      assert(std::abs(segments[0].predict(keys[i]) - i) <= 9);
  }
  std::vector<Segment> empty;
  PLA pla(4);
  std::vector<Key> keys;
  assert(pla.make_segs(keys, 0, empty) == 0);
  bool rejected = false;
  try { PLA invalid(0); } catch (std::invalid_argument&) { rejected = true; }
  assert(rejected);
  std::cout << "OK: geometría y segmentos\n";
}
