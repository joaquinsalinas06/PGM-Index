#pragma once

#include <algorithm>
#include <climits>
#include <stdexcept>
#include <vector>

using Key = unsigned long long;
using namespace std;

struct DynamicBPlusNode {
  bool leaf = true;
  int total = 0;
  vector<Key> keys;
  vector<DynamicBPlusNode*> kids;

  int size() { return leaf ? keys.size() : total; }

  int entries() { return leaf ? keys.size() : kids.size(); }

  Key max_key() {
    DynamicBPlusNode* node = this;
    while (!node->leaf)
      node = node->kids.back();
    return node->keys.back();
  }

  Key min_key() {
    DynamicBPlusNode* node = this;
    while (!node->leaf)
      node = node->kids.front();
    return node->keys.empty() ? 0 : node->keys.front();
  }
};

struct DynamicBPlusTree {
  int cap, min_cap;
  DynamicBPlusNode* root;
  int height = 1;
  int n = 0;

  DynamicBPlusTree(int cap_ = 64) : cap(cap_), min_cap(max(2, cap_ / 4)) {
    if (cap < 4 || cap > INT_MAX / 4)
      throw invalid_argument("capacidad debe estar entre 4 e INT_MAX/4");
    root = new DynamicBPlusNode;
  }

  DynamicBPlusTree(vector<Key>& keys, int cap_) : DynamicBPlusTree(cap_) {
    if (keys.size() > INT_MAX || !is_sorted(keys.begin(), keys.end()) || (!keys.empty() && keys.back() == ULLONG_MAX))
      throw invalid_argument("llaves ordenadas, n <= INT_MAX y sin ULLONG_MAX");
    n = keys.size();
    if (keys.empty())
      return;
    delete root;
    int fill = max(2, cap * 3 / 4);
    vector<DynamicBPlusNode*> level = build_leaves(keys, fill);
    while (level.size() > 1) {
      level = build_parents(level, fill);
      height++;
    }
    root = level[0];
  }

  ~DynamicBPlusTree() { destroy(root); }
  DynamicBPlusTree(DynamicBPlusTree&) = delete;
  DynamicBPlusTree& operator=(DynamicBPlusTree&) = delete;

  int size() { return n; }

  int lookup(Key x) {
    DynamicBPlusNode* node = root;
    int rank = 0;
    while (!node->leaf) {
      int child = lower_bound(node->keys.begin(), node->keys.end(), x) - node->keys.begin();
      for (int j = 0; j < child; j++)
        rank += node->kids[j]->size();
      node = node->kids[child];
    }
    return rank + (lower_bound(node->keys.begin(), node->keys.end(), x) - node->keys.begin());
  }

  void insert(Key x) {
    if (x == ULLONG_MAX)
      throw invalid_argument("ULLONG_MAX esta reservado como centinela");
    if (n == INT_MAX) throw length_error("demasiadas llaves");
    DynamicBPlusNode* node = root;
    while (!node->leaf) {
      if (int(node->kids.size()) == cap)
        throw length_error("esta etapa no divide nodos internos llenos");
      int child = lower_bound(node->keys.begin(), node->keys.end(), x) - node->keys.begin();
      node = node->kids[child];
    }
    Key sep;
    DynamicBPlusNode* right = insert_node(root, x, sep);
    n++;
    if (right)
      grow_root(right, sep);
  }

  long long size_in_bytes() { return measure(root, false); }

  long long ram_bytes() { return sizeof(*this) + measure(root, true); }

  bool check() {
    int leaf_level = -1, count = 0;
    Key prev = 0;
    return check_node(root, true, 0, leaf_level, count, prev) && count == n;
  }

 private:
  size_t groups_for(size_t total, size_t fill) { return (total + fill - 1) / fill; }

  vector<DynamicBPlusNode*> build_leaves(vector<Key>& keys, int fill) {
    vector<DynamicBPlusNode*> leaves;
    size_t groups = groups_for(keys.size(), fill), start = 0;
    for (size_t i = 0; i < groups; i++) {
      size_t size = keys.size() / groups + (i < keys.size() % groups);
      DynamicBPlusNode* leaf = new DynamicBPlusNode;
      leaf->keys.assign(keys.begin() + start, keys.begin() + start + size);
      start += size;
      leaves.push_back(leaf);
    }
    return leaves;
  }

  vector<DynamicBPlusNode*> build_parents(vector<DynamicBPlusNode*>& level, int fill) {
    vector<DynamicBPlusNode*> parents;
    size_t groups = groups_for(level.size(), fill), start = 0;
    for (size_t i = 0; i < groups; i++) {
      size_t size = level.size() / groups + (i < level.size() % groups);
      DynamicBPlusNode* parent = new DynamicBPlusNode;
      parent->leaf = false;
      for (size_t j = start; j < start + size; j++) {
        if (j > start)
          parent->keys.push_back(level[j - 1]->max_key());
        parent->kids.push_back(level[j]);
        parent->total += level[j]->size();
      }
      start += size;
      parents.push_back(parent);
    }
    return parents;
  }

  DynamicBPlusNode* insert_node(DynamicBPlusNode* node, Key x, Key& sep) {
    if (node->leaf) {
      node->keys.insert(lower_bound(node->keys.begin(), node->keys.end(), x), x);
      return (int)node->keys.size() > cap ? split_leaf(node, sep) : nullptr;
    }
    int child = lower_bound(node->keys.begin(), node->keys.end(), x) - node->keys.begin();
    Key child_sep;
    DynamicBPlusNode* right = insert_node(node->kids[child], x, child_sep);
    node->total++;
    if (!right)
      return nullptr;
    node->keys.insert(node->keys.begin() + child, child_sep);
    node->kids.insert(node->kids.begin() + child + 1, right);
    return nullptr;
  }

  DynamicBPlusNode* split_leaf(DynamicBPlusNode* leaf, Key& sep) {
    DynamicBPlusNode* right = new DynamicBPlusNode;
    int half = leaf->keys.size() / 2;
    right->keys.assign(leaf->keys.begin() + half, leaf->keys.end());
    leaf->keys.resize(half);
    sep = leaf->keys.back();
    return right;
  }

  void grow_root(DynamicBPlusNode* right, Key sep) {
    DynamicBPlusNode* new_root = new DynamicBPlusNode;
    new_root->leaf = false;
    new_root->kids = {root, right};
    new_root->keys = {sep};
    new_root->total = root->size() + right->size();
    root = new_root;
    height++;
  }

  void destroy(DynamicBPlusNode* node) {
    if (!node)
      return;
    for (DynamicBPlusNode* kid : node->kids)
      destroy(kid);
    delete node;
  }

  long long measure(DynamicBPlusNode* node, bool with_leaves) {
    long long bytes = 0;
    if (node->leaf) {
      if (with_leaves)
        bytes += sizeof(DynamicBPlusNode) + node->keys.capacity() * 8;
      return bytes;
    }
    bytes += with_leaves ? sizeof(DynamicBPlusNode) + node->keys.capacity() * 8 + node->kids.capacity() * 8
                            : node->keys.size() * 8 + node->kids.size() * 8;
    for (DynamicBPlusNode* kid : node->kids)
      bytes += measure(kid, with_leaves);
    return bytes;
  }

  bool check_node(DynamicBPlusNode* node, bool is_root, int depth, int& leaf_level, int& count,
              Key& prev) {
    int entries = node->entries();
    if (entries > cap || (!is_root && entries < min_cap))
      return false;
    if (node->leaf)
      return check_leaf(node, depth, leaf_level, count, prev);
    if (node->keys.size() + 1 != node->kids.size())
      return false;
    int before = count;
    for (size_t i = 0; i < node->kids.size(); i++) {
      if (!check_node(node->kids[i], false, depth + 1, leaf_level, count, prev))
        return false;
      if (i < node->keys.size() && !fits(node, i, prev))
        return false;
    }
    return node->total == count - before;
  }

  bool check_leaf(DynamicBPlusNode* leaf, int depth, int& leaf_level, int& count,
                   Key& prev) {
    if (leaf_level < 0)
      leaf_level = depth;
    if (leaf_level != depth || !is_sorted(leaf->keys.begin(), leaf->keys.end()))
      return false;
    if (!leaf->keys.empty()) {
      if (leaf->keys[0] < prev)
        return false;
      prev = leaf->keys.back();
    }
    count += leaf->keys.size();
    return true;
  }

  bool fits(DynamicBPlusNode* node, size_t i, Key left_max) {
    return left_max <= node->keys[i] && node->kids[i + 1]->min_key() >= node->keys[i];
  }
};
