#pragma once

#include "pla.h"

struct ApproxPos {
  int pos, lo, hi;
};

using namespace std;

struct PGMIndex {
  vector<Key>* data;
  int base_segs = 0;
  int n, eps, eps_rec;
  Key first = 0;
  vector<Segment> segments;
  vector<int> offsets;

  PGMIndex(vector<Key>& keys, int eps_, int eps_rec_ = 4) {
    data = &keys;
    n = keys.size();
    eps = eps_;
    eps_rec = eps_rec_;
    if (eps < 1 || eps_rec < 1)
      throw invalid_argument("eps y eps_rec deben ser >= 1");
    if (keys.size() > INT_MAX || !is_sorted(keys.begin(), keys.end()))
      throw invalid_argument("se requieren llaves ordenadas y n <= INT_MAX");
    if (eps > INT_MAX / 4 || eps_rec > INT_MAX / 4)
      throw invalid_argument("eps demasiado grande");
    if (n == 0)
      return;
    if (keys.back() == ULLONG_MAX)
      throw invalid_argument("ULLONG_MAX esta reservado como centinela");
    first = keys[0];
    build_levels(keys);
  }

  PGMIndex(PGMIndex&) = delete;
  PGMIndex& operator=(PGMIndex&) = delete;

  int lookup(Key x) {
    if (n == 0 || x == ULLONG_MAX)
      return n;
    ApproxPos window = approx(x);
    return lower_bound(data->begin() + window.lo, data->begin() + window.hi, x) -
           data->begin();
  }

  ApproxPos approx(Key key) {
    if (n == 0) return {0, 0, 0};
    if (key == ULLONG_MAX) return {n, n, n};
    Key k = max(first, key);
    int pos = predict(find_seg(k), k);
    int lo = pos - eps, hi = pos + eps + 2;
    if (lo < 0)
      lo = 0;
    if (hi > n)
      hi = n;
    return {pos, lo, hi};
  }

  int find_seg(Key key) {
    auto end = segments.begin() + offsets[1];
    return upper_bound(segments.begin(), end, key,
                       [](Key x, Segment& s) { return x < s.key; }) - segments.begin() - 1;
  }

  int predict(int seg_id, Key key) {
    double pos = segments[seg_id].predict(key);
    if (pos > segments[seg_id + 1].b)
      pos = segments[seg_id + 1].b;
    if (pos < 0)
      pos = 0;
    return pos;
  }

  long long size_in_bytes() {
    long long n_segs = segments.size(), n_offsets = offsets.size();
    return n_segs * sizeof(Segment) + n_offsets * sizeof(int);
  }

  int height() { return offsets.empty() ? 0 : offsets.size() - 1; }

  int seg_count() { return base_segs; }

  long long ram_bytes() {
    return sizeof(*this) + segments.capacity() * sizeof(Segment) + offsets.capacity() * sizeof(int);
  }

 private:
  void build_levels(vector<Key>& keys) {
    offsets.push_back(0);
    PLA pla(eps);
    int len = pla.make_segs(keys, data->back(), segments);
    base_segs = len;
    offsets.push_back(segments.size());

  }
};
