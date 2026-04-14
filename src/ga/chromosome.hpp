#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct Problem;
class Rng;

struct Chromosome {
    std::vector<bool> bits;
    double x = 0.0;
    double fitness = 0.0;

    static Chromosome random(const Problem& pb, Rng& rng);
    static Chromosome fromBits(std::vector<bool> bits, const Problem& pb);

    void evaluate(const Problem& pb);

    uint64_t toUInt() const;
    std::string bitString() const;
};
