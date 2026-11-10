#pragma once

#include <algorithm>
#include <climits>
#include <stdexcept>
#include <vector>

using Key = unsigned long long;

struct Slope {
  __int128 dx, dy;
  bool operator<(Slope other) { return dy * other.dx < dx * other.dy; }
  bool operator>(Slope other) { return dy * other.dx > dx * other.dy; }
};

struct Point {
  __int128 x, y;
  Slope operator-(Point other) { return {x - other.x, y - other.y}; }
  __int128 cross(Point a, Point b) {
    Slope oa = a - *this, ob = b - *this;
    return oa.dx * ob.dy - oa.dy * ob.dx;
  }
};

struct Segment {
  Key key;
  float m;
  int b;

  double predict(Key k) {
    Key dist = k - key;
    return m * dist + b;
  }
};
