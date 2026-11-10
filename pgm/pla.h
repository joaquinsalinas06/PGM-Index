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

using namespace std;

struct PLA {
  int eps;
  vector<Point> low, up;
  int low_pos = 0, up_pos = 0;
  Key first = 0;
  Point rec[4];

  PLA(int eps_) : eps(eps_) {
    if (eps < 1) throw invalid_argument("eps debe ser >= 1");
  }

  bool add_point(Key x, int y) {
    Point up_pt{x, (__int128)y + eps};
    Point low_pt{x, y <= eps ? 0 : (__int128)y - eps};

    if (low.empty()) {
      first = x;
      rec[0] = up_pt;
      rec[1] = low_pt;
      up.clear();
      low.clear();
      up.push_back(up_pt);
      low.push_back(low_pt);
      up_pos = low_pos = 0;
      return true;
    }
    if (low.size() == 1) {
      rec[2] = low_pt;
      rec[3] = up_pt;
      up.push_back(up_pt);
      low.push_back(low_pt);
      return true;
    }

    Slope max_m = rec[2] - rec[0];
    Slope min_m = rec[3] - rec[1];
    if (up_pt - rec[2] < max_m || low_pt - rec[3] > min_m) {
      low.clear();
      up.clear();
      return false;
    }
    if (up_pt - rec[1] < min_m)
      fit_max(up_pt);
    if (low_pt - rec[0] > max_m)
      fit_min(low_pt);
    return true;
  }

  Segment get_seg() {
    Point r[4] = {rec[0], rec[1], rec[2], rec[3]};
    if (low.size() == 1) {
      r[2] = r[0];
      r[3] = r[1];
    }
    bool single = r[0].x == r[2].x && r[0].y == r[2].y && r[1].x == r[3].x && r[1].y == r[3].y;
    Segment seg;
    seg.key = first;
    if (single) {
      seg.m = 0;
      seg.b = (r[0].y + r[1].y) / 2;
    } else {
      Slope m = r[3] - r[1];
      __int128 num = m.dy * (first - r[1].x);
      __int128 round = m.dx / 2;
      if (num < 0)
        round = -round;
      seg.b = (num + round) / m.dx + r[1].y;
      long double dy = m.dy;
      seg.m = dy / m.dx;
    }
    return seg;
  }

  int make_segs(vector<Key>& keys, Key last_key, vector<Segment>& out) {
    if (keys.empty()) return 0;
    if (keys.size() > INT_MAX || !is_sorted(keys.begin(), keys.end()) || keys.back() == ULLONG_MAX)
      throw invalid_argument("se requieren llaves ordenadas, sin ULLONG_MAX, y n <= INT_MAX");

    low.clear();
    up.clear();
    int count = 0;
    auto add = [&](Key x, int y) {
      if (!add_point(x, y)) {
        out.push_back(get_seg());
        ++count;
        add_point(x, y);
      }
    };

    int n = keys.size();
    add(keys[0], 0);
    for (int i = 1; i + 1 < n; ++i) {
      if (keys[i] != keys[i - 1]) add(keys[i], i);
      else if (keys[i] + 1 < keys[i + 1]) add(keys[i] + 1, i);
    }
    if (n >= 2 && keys[n - 1] != keys[n - 2]) add(keys[n - 1], n - 1);
    add(keys.back() + 1, n);
    out.push_back(get_seg());
    ++count;

    Segment last = out.back();
    if (last.key == ULLONG_MAX) return count - 1;
    if (last.predict(ULLONG_MAX - 1) < n)
      out.push_back({last_key + 1, 0, n});
    out.push_back({ULLONG_MAX, 0, n});
    return count;
  }

 private:
  void fit_max(Point up_pt) {
    Slope best = low[low_pos] - up_pt;
    int best_pos = low_pos;
    int size = low.size();
    for (int i = low_pos + 1; i < size; i++) {
      Slope candidate = low[i] - up_pt;
      if (candidate > best)
        break;
      best = candidate;
      best_pos = i;
    }
    rec[1] = low[best_pos];
    rec[3] = up_pt;
    low_pos = best_pos;

    int end = up.size();
    while (end >= up_pos + 2 && up[end - 2].cross(up[end - 1], up_pt) <= 0)
      end--;
    up.resize(end);
    up.push_back(up_pt);
  }

  void fit_min(Point low_pt) {
    Slope best = up[up_pos] - low_pt;
    int best_pos = up_pos;
    int size = up.size();
    for (int i = up_pos + 1; i < size; i++) {
      Slope candidate = up[i] - low_pt;
      if (candidate < best)
        break;
      best = candidate;
      best_pos = i;
    }
    rec[0] = up[best_pos];
    rec[2] = low_pt;
    up_pos = best_pos;

    int end = low.size();
    while (end >= low_pos + 2 && low[end - 2].cross(low[end - 1], low_pt) >= 0)
      end--;
    low.resize(end);
    low.push_back(low_pt);
  }
};
