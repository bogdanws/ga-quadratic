#pragma once

#include <cstdint>

struct Problem {
    double a, b;
    double A, B, C;
    int precision;
    int bitLength;

    static Problem make(double a, double b, double A, double B, double C, int precision);

    double decode(uint64_t X10) const;
    double fitness(double x) const;
    uint64_t maxEncoded() const { return (1ULL << bitLength) - 1; }
};
