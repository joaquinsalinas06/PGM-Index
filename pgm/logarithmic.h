#pragma once

#include <iterator>
#include <memory>
#include "static.h"

struct LogarithmicPGM {
  struct Level {
    std::vector<Key> keys;
    PGMIndex index;

    Level(std::vector<Key> values, int eps)
        : keys(std::move(values)), index(keys, eps) {}
  };

  int eps, cap;
  int n = 0;
  std::vector<Key> buffer;
  std::vector<std::unique_ptr<Level>> levels;
  long long merges = 0, moved_keys = 0;

  LogarithmicPGM(std::vector<Key> keys = {}, int eps_ = 64, int capacity = 256)
      : eps(eps_), cap(capacity) {
    if (capacity < 2 || capacity > INT_MAX / 4 || eps < 1 || eps > INT_MAX / 4)
      throw std::invalid_argument("eps o capacidad invalidos");
    if (keys.size() > INT_MAX || !std::is_sorted(keys.begin(), keys.end()) ||
        (!keys.empty() && keys.back() == ULLONG_MAX))
      throw std::invalid_argument("llaves ordenadas, n <= INT_MAX y sin ULLONG_MAX");
    if (keys.empty()) return;
    std::size_t target = 0;
    while (level_cap(target) < keys.size()) ++target;
    levels.resize(target + 1);
    n = keys.size();
    levels[target] = std::make_unique<Level>(std::move(keys), eps);
  }

  int size() { return n; }

  int lookup(Key x) {
    int rank = std::lower_bound(buffer.begin(), buffer.end(), x) - buffer.begin();
    for (auto& level : levels)
      if (level) rank += level->index.lookup(x);
    return rank;
  }

  void insert(Key x) {
    if (x == ULLONG_MAX) throw std::invalid_argument("ULLONG_MAX esta reservado");
    if (n == INT_MAX) throw std::length_error("demasiadas llaves");
    buffer.insert(std::lower_bound(buffer.begin(), buffer.end(), x), x);
    ++n;
    if (buffer.size() >= std::size_t(cap)) flush();
  }

  int seg_count() {
    int count = 0;
    for (auto& level : levels)
      if (level) count += level->index.seg_count();
    return count;
  }

  int levels_used() {
    int count = 0;
    for (auto& level : levels) if (level) ++count;
    return count;
  }

  long long size_in_bytes() {
    long long bytes = 0;
    for (auto& level : levels) if (level) bytes += level->index.size_in_bytes();
    return bytes;
  }

  long long ram_bytes() {
    long long bytes = sizeof(*this) + buffer.capacity() * sizeof(Key) +
                      levels.capacity() * sizeof(std::unique_ptr<Level>);
    for (auto& level : levels)
      if (level) bytes += sizeof(Level) + level->keys.capacity() * sizeof(Key) +
                          level->index.ram_bytes() - sizeof(PGMIndex);
    return bytes;
  }

  bool check() {
    std::size_t total = buffer.size();
    if (!std::is_sorted(buffer.begin(), buffer.end())) return false;
    for (std::size_t k = 0; k < levels.size(); ++k) {
      if (!levels[k]) continue;
      auto& level = *levels[k];
      if (level.keys.size() > level_cap(k) ||
          !std::is_sorted(level.keys.begin(), level.keys.end()) ||
          level.index.data != &level.keys || level.index.n != int(level.keys.size())) return false;
      total += level.keys.size();
    }
    return total == std::size_t(n);
  }

 private:
  std::size_t level_cap(std::size_t k) { return std::size_t(cap) << k; }

  void flush() {
    std::size_t target = 0, needed = buffer.size();
    for (;;) {
      if (target < levels.size() && levels[target]) needed += levels[target]->keys.size();
      if (needed <= level_cap(target)) break;
      ++target;
    }
    std::vector<Key> merged = buffer;
    for (std::size_t k = 0; k <= target && k < levels.size(); ++k) {
      if (!levels[k]) continue;
      std::vector<Key> next;
      next.reserve(merged.size() + levels[k]->keys.size());
      std::merge(merged.begin(), merged.end(), levels[k]->keys.begin(), levels[k]->keys.end(),
                 std::back_inserter(next));
      merged.swap(next);
    }

    auto next_level = std::make_unique<Level>(std::move(merged), eps);
    if (target >= levels.size()) levels.resize(target + 1);
    moved_keys += next_level->keys.size();
    levels[target].swap(next_level);
    for (std::size_t k = 0; k < target; ++k) levels[k].reset();
    buffer.clear();
    ++merges;
  }
};
