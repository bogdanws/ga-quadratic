#include "ga/chromosome.hpp"

#include "ga/problem.hpp"
#include "ga/rng.hpp"

#include <stdexcept>
#include <string>

// factory methods

Chromosome Chromosome::random(const Problem& pb, Rng& rng) {
    Chromosome c;
    c.bits = rng.randomBits(pb.bitLength);
    c.evaluate(pb);
    return c;
}

Chromosome Chromosome::fromBits(std::vector<bool> bits, const Problem& pb) {
    Chromosome c;
    c.bits = std::move(bits);
    c.evaluate(pb);
    return c;
}

// evaluation

void Chromosome::evaluate(const Problem& pb) {
    uint64_t val = toUInt();
    x = pb.decode(val);
    fitness = pb.fitness(x);

    // user picks A,B,C; if the quadratic goes negative on [a,b], roulette breaks
    // tolerate float noise on boundary zeros
    if (fitness < -1e-12) {
        throw std::runtime_error(
            "chromosome: negative fitness at x=" + std::to_string(x));
    }
    if (fitness < 0.0)
        fitness = 0.0;
}

// helpers

uint64_t Chromosome::toUInt() const {
    // bits[0] is the MSB
    uint64_t val = 0;
    for (bool bit : bits) {
        val = (val << 1u) | (bit ? 1ULL : 0ULL);
    }
    return val;
}

std::string Chromosome::bitString() const {
    std::string s;
    s.reserve(bits.size());
    for (bool bit : bits) {
        s += (bit ? '1' : '0');
    }
    return s;
}
