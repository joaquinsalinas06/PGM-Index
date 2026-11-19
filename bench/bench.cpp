#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <memory>
#include <random>
#include <string>
#include "../bplus/dynamic.h"
#include "../bplus/static.h"
#include "../pgm/buffered.h"
#include "../pgm/logarithmic.h"
#include "../pgm/static.h"

using namespace std;

using Clock = chrono::steady_clock;

const int REPS = 5;
const size_t Q = 1000000;
const int EPS[] = {4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048};
const int CAPS[] = {8, 16, 32, 64, 128, 256, 512, 1024};

volatile long long sink = 0;

double secs(Clock::time_point a, Clock::time_point b) {
  return chrono::duration<double>(b - a).count();
}

vector<Key> load(const string& path) {
  ifstream in(path, ios::binary);
  Key n;
  in.read((char*)&n, 8);
  vector<Key> keys(n);
  in.read((char*)keys.data(), n * 8);
  return keys;
}

string name_of(const string& path) {
  string s = path.substr(path.find_last_of('/') + 1);
  return s.substr(0, s.find('.'));
}

vector<Key> sample(const vector<Key>& keys, size_t n) {
  if (n >= keys.size()) return keys;
  vector<Key> out;
  size_t step = keys.size() / n;
  for (size_t i = 0; i < keys.size() && out.size() < n; i += step) out.push_back(keys[i]);
  return out;
}

vector<Key> queries(const vector<Key>& keys, size_t q, int seed) {
  mt19937_64 rng(seed);
  vector<Key> out(q);
  for (auto& x : out) x = keys[rng() % keys.size()];
  return out;
}

template <class F>
double time_lookups(const vector<Key>& qs, F lookup) {
  long long s = 0;
  auto t0 = Clock::now();
  for (Key x : qs) s += lookup(x);
  auto t1 = Clock::now();
  sink += s;
  return secs(t0, t1) * 1e9 / qs.size();
}

long long maint(BufferedPGM& x) { return x.rebuilds; }
long long maint(LogarithmicPGM& x) { return x.merges; }
long long maint(DynamicBPlusTree& x) { return 0; }

long long moved(BufferedPGM& x) { return x.moved_keys; }
long long moved(LogarithmicPGM& x) { return x.moved_keys; }
long long moved(DynamicBPlusTree& x) { return 0; }

int segs(BufferedPGM& x) { return x.seg_count(); }
int segs(LogarithmicPGM& x) { return x.seg_count(); }
int segs(DynamicBPlusTree& x) { return 0; }

void mode_segments(const string& path) {
  auto keys = load(path);
  auto name = name_of(path);
  for (int eps : EPS) {
    vector<double> build;
    for (int r = 0; r < 3; r++) {
      auto t0 = Clock::now();
      PGMIndex pgm(keys, eps);
      build.push_back(secs(t0, Clock::now()) * 1e3);
    }
    sort(build.begin(), build.end());
    PGMIndex pgm(keys, eps);
    string levels;
    for (size_t l = 0; l + 1 < pgm.offsets.size(); l++) {
      if (l) levels += ";";
      levels += to_string(pgm.offsets[l + 1] - pgm.offsets[l]);
    }
    printf("%s,%zu,%d,%d,%.1f,%d,%lld,%.2f,%s\n", name.c_str(), keys.size(), eps, pgm.seg_count(),
           keys.size() / (2.0 * eps), pgm.height(), pgm.size_in_bytes(), build[1], levels.c_str());
  }
}

void mode_lookup(vector<Key>& keys, const string& name, bool full) {
  auto qs = queries(keys, Q, 42);
  size_t n = keys.size();
  for (int r = 0; r < REPS; r++) {
    double ns = time_lookups(qs, [&](Key x) { return int(lower_bound(keys.begin(), keys.end(), x) - keys.begin()); });
    printf("%s,%zu,binaria,0,0,%d,%.2f\n", name.c_str(), n, r, ns);
  }
  for (int eps : EPS) {
    if (!full && eps != 64) continue;
    PGMIndex pgm(keys, eps);
    for (int r = 0; r < REPS; r++) {
      double ns = time_lookups(qs, [&](Key x) { return pgm.lookup(x); });
      printf("%s,%zu,pgm,%d,%lld,%d,%.2f\n", name.c_str(), n, eps, pgm.size_in_bytes(), r, ns);
    }
  }
  for (int cap : CAPS) {
    if (!full && cap != 64) continue;
    StaticBPlusTree bp(keys, cap);
    for (int r = 0; r < REPS; r++) {
      double ns = time_lookups(qs, [&](Key x) { return bp.lookup(x); });
      printf("%s,%zu,bplus,%d,%lld,%d,%.2f\n", name.c_str(), n, cap, bp.bytes, r, ns);
    }
  }
}

void mode_sizes(const string& path) {
  auto keys = load(path);
  for (size_t n : {100000, 1000000}) {
    auto s = sample(keys, n);
    mode_lookup(s, name_of(path), false);
  }
}

template <class F>
void run_dynamic(const string& name, const char* kind, int cap, F make,
                 const vector<Key>& base, const vector<Key>& ins, const vector<Key>& qs) {
  for (int r = 0; r < 3; r++) {
    auto idx = make();
    vector<double> lat(ins.size());
    auto t0 = Clock::now();
    for (size_t i = 0; i < ins.size(); i++) {
      auto a = Clock::now();
      idx->insert(ins[i]);
      lat[i] = secs(a, Clock::now()) * 1e9;
    }
    double total = secs(t0, Clock::now());
    sort(lat.begin(), lat.end());
    vector<double> look;
    for (int q = 0; q < REPS; q++)
      look.push_back(time_lookups(qs, [&](Key x) { return idx->lookup(x); }));
    sort(look.begin(), look.end());
    printf("%s,%zu,%zu,%s,%d,%d,%.1f,%.1f,%.1f,%.1f,%.2f,%.2f,%.2f,%lld,%lld,%lld,%d\n", name.c_str(),
           base.size(), ins.size(), kind, cap, r, total * 1e9 / ins.size(), lat[ins.size() / 2],
           lat[ins.size() * 99 / 100], lat.back(), look[0], look[REPS / 2], look[REPS - 1],
           idx->ram_bytes(), maint(*idx), moved(*idx), segs(*idx));
  }
}

void mode_dynamic(const string& path) {
  const size_t N = 1200000, I = 200000;
  auto all = sample(load(path), N);
  auto name = name_of(path);
  mt19937_64 rng(1);
  shuffle(all.begin(), all.end(), rng);
  vector<Key> ins(all.begin(), all.begin() + I);
  vector<Key> base(all.begin() + I, all.end());
  sort(base.begin(), base.end());
  vector<Key> final_keys = all;
  sort(final_keys.begin(), final_keys.end());
  auto qs = queries(final_keys, Q, 42);

  for (int cap : {256, 4096})
    run_dynamic(name, "buffered", cap, [&] { return make_unique<BufferedPGM>(base, 64, cap); }, base, ins, qs);
  for (int cap : {256, 4096})
    run_dynamic(name, "logarithmic", cap, [&] { return make_unique<LogarithmicPGM>(base, 64, cap); }, base, ins, qs);
  run_dynamic(name, "bplus_dyn", 64, [&] { return make_unique<DynamicBPlusTree>(base, 64); }, base, ins, qs);

  PGMIndex pgm(final_keys, 64);
  vector<double> look;
  for (int q = 0; q < REPS; q++)
    look.push_back(time_lookups(qs, [&](Key x) { return pgm.lookup(x); }));
  sort(look.begin(), look.end());
  printf("%s,%zu,%zu,pgm_static,64,0,0,0,0,0,%.2f,%.2f,%.2f,%lld,0,0,%d\n", name.c_str(), base.size(),
         ins.size(), look[0], look[REPS / 2], look[REPS - 1],
         (long long)(final_keys.capacity() * 8) + pgm.ram_bytes(), pgm.seg_count());
}

int main(int argc, char** argv) {
  if (argc < 3) return 1;
  string mode = argv[1];
  if (mode == "segments") puts("dataset,n,eps,m,bound,height,bytes,build_ms,levels");
  if (mode == "lookup" || mode == "sizes") puts("dataset,n,index,param,bytes,rep,ns");
  if (mode == "dynamic")
    puts("dataset,n0,inserts,index,cap,rep,ins_avg_ns,ins_p50_ns,ins_p99_ns,ins_max_ns,"
         "look_min_ns,look_med_ns,look_max_ns,ram_bytes,maint,moved_keys,segs");
  for (int i = 2; i < argc; i++) {
    if (mode == "segments") mode_segments(argv[i]);
    if (mode == "lookup") {
      auto keys = load(argv[i]);
      mode_lookup(keys, name_of(argv[i]), true);
    }
    if (mode == "sizes") mode_sizes(argv[i]);
    if (mode == "dynamic") mode_dynamic(argv[i]);
  }
  return 0;
}
