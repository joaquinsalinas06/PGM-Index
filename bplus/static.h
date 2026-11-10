#pragma once

#include <algorithm>
#include <climits>
#include <stdexcept>
#include <vector>

using Key = unsigned long long;

using namespace std;

struct StaticBPlusNode {
  bool leaf;
  int start;
  vector<Key> keys;
  vector<StaticBPlusNode*> kids;
};

struct StaticBPlusTree {
  vector<Key>* data;
  int cap;
  StaticBPlusNode* root = nullptr;
  int height = 0;
  long long bytes = 0;
  long long ram_bytes = 0;

  StaticBPlusTree(vector<Key>& keys, int cap_) {
    if (cap_ < 2 || cap_ > INT_MAX / 4)
      throw invalid_argument("capacidad debe estar entre 2 e INT_MAX/4");
    if (keys.size() > INT_MAX || !is_sorted(keys.begin(), keys.end()) || (!keys.empty() && keys.back() == ULLONG_MAX))
      throw invalid_argument("llaves ordenadas, n <= INT_MAX y sin ULLONG_MAX");
    data = &keys;
    cap = cap_;
    if (keys.empty())
      return;
    vector<Key> mins;
    vector<StaticBPlusNode*> level = build_leaves(keys, mins);
    while (level.size() > 1) {
      level = build_parents(level, mins);
      height++;
    }
    root = level[0];
  }

  StaticBPlusTree(StaticBPlusTree&) = delete;
  StaticBPlusTree& operator=(StaticBPlusTree&) = delete;

  ~StaticBPlusTree() { destroy(root); }

 private:
  vector<StaticBPlusNode*> build_leaves(vector<Key>& keys, vector<Key>& mins) {
    vector<StaticBPlusNode*> leaves;
    for (int start = 0; start < (int)keys.size(); start += cap) {
      StaticBPlusNode* leaf = new StaticBPlusNode;
      leaf->leaf = true;
      leaf->start = start;
      leaves.push_back(leaf);
      mins.push_back(keys[start]);
      ram_bytes += sizeof(StaticBPlusNode);
    }
    return leaves;
  }

  vector<StaticBPlusNode*> build_parents(vector<StaticBPlusNode*>& level, vector<Key>& mins) {
    vector<StaticBPlusNode*> parents;
    vector<Key> next_mins;
    int size = level.size();
    for (int first = 0; first < size; first += cap) {
      StaticBPlusNode* parent = new StaticBPlusNode;
      parent->leaf = false;
      parent->start = 0;
      for (int j = first; j < first + cap && j < size; j++) {
        parent->kids.push_back(level[j]);
        if (j > first)
          parent->keys.push_back(mins[j]);
      }
      count_node(parent);
      parents.push_back(parent);
      next_mins.push_back(mins[first]);
    }
    mins = next_mins;
    return parents;
  }

  void count_node(StaticBPlusNode* node) {
    long long n = node->keys.size(), n_kids = node->kids.size();
    long long key_cap = node->keys.capacity(), kid_cap = node->kids.capacity();
    bytes += n * 8 + n_kids * 8;
    ram_bytes += sizeof(StaticBPlusNode) + key_cap * 8 + kid_cap * 8;
  }

  void destroy(StaticBPlusNode* node) {
    if (!node)
      return;
    for (StaticBPlusNode* kid : node->kids)
      destroy(kid);
    delete node;
  }
};
