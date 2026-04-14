#include "ga/rng.hpp"

#include <cstdint>

Rng::Rng(uint64_t seed) : engine_(seed) {}

double Rng::uniform01() {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(engine_);
}

int Rng::uniformInt(int lo, int hi) {
    std::uniform_int_distribution<int> dist(lo, hi);
    return dist(engine_);
}

std::vector<bool> Rng::randomBits(int n) {
    std::vector<bool> bits(n);
    for (int i = 0; i < n;) {
        uint64_t w = engine_();
        for (int b = 0; b < 64 && i < n; ++b, ++i) {
            bits[i] = static_cast<bool>((w >> b) & 1u);
        }
    }
    return bits;
}
