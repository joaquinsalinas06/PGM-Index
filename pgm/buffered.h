#pragma once

#include <memory>
#include <iterator>
#include "static.h"

struct BufferedPGM {
  int eps, cap;
  std::unique_ptr<std::vector<Key>> base;
  std::unique_ptr<PGMIndex> index;
  std::vector<Key> buffer;
  long long rebuilds = 0, moved_keys = 0;

  BufferedPGM(std::vector<Key> keys = {}, int eps_ = 64, int capacity = 256)
      : eps(eps_), cap(capacity) {
    if (capacity < 2 || capacity > INT_MAX / 4)
      throw std::invalid_argument("capacidad debe estar entre 2 e INT_MAX/4");
    base = std::make_unique<std::vector<Key>>(std::move(keys));
    index = std::make_unique<PGMIndex>(*base, eps);
  }


  int size() { 
    return base->size() + buffer.size(); 
  }


  int lookup(Key x) {
    return index->lookup(x) +
           (std::lower_bound(buffer.begin(), buffer.end(), x) - buffer.begin());
  }

  void insert(Key x) {
    if (x == ULLONG_MAX) throw std::invalid_argument("ULLONG_MAX esta reservado");
    if (size() == INT_MAX) throw std::length_error("demasiadas llaves");
    buffer.insert(std::lower_bound(buffer.begin(), buffer.end(), x), x);
    if (buffer.size() >= std::size_t(cap)) rebuild();
  }


  int seg_count() { 
    return index->seg_count(); 
  }
  int height() { 
    return index->height(); 
  }
  long long size_in_bytes() { 
    return index->size_in_bytes(); 
  }


  long long ram_bytes() {
    return sizeof(*this) + sizeof(*base) + base->capacity() * sizeof(Key) +
           buffer.capacity() * sizeof(Key) + index->ram_bytes();
  }

  bool check() {
    return std::is_sorted(base->begin(), base->end()) &&
           std::is_sorted(buffer.begin(), buffer.end()) && index->data == base.get() &&
           index->n == int(base->size());
  }


 private:
  void rebuild() {
    auto merged = std::make_unique<std::vector<Key>>();
    merged->reserve(size());
    std::merge(base->begin(), base->end(), buffer.begin(), buffer.end(), std::back_inserter(*merged));
    auto next_pgm = std::make_unique<PGMIndex>(*merged, eps);

    base.swap(merged);
    index.swap(next_pgm);
    buffer.clear();
    ++rebuilds;
    moved_keys += base->size();
  }

};
