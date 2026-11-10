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
  std::cout << "OK: geometría y segmentos\n";
}
