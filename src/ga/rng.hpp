#pragma once

#include <algorithm>
#include <cstdint>
#include <random>
#include <vector>

class Rng {
  public:
    explicit Rng(uint64_t seed);

    double uniform01();
    int uniformInt(int lo, int hi);

    template <typename It> void shuffle(It first, It last) { std::shuffle(first, last, engine_); }

    std::vector<bool> randomBits(int n);

  private:
    std::mt19937_64 engine_;
};
