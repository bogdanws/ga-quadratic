#include "ga/problem.hpp"

#include <cmath>
#include <stdexcept>

Problem Problem::make(double a, double b, double A, double B, double C, int precision) {
    if (a >= b) {
        throw std::invalid_argument("problem: requires a < b");
    }
    if (precision < 0) {
        throw std::invalid_argument("problem: precision must be >= 0");
    }

    Problem p;
    p.a = a;
    p.b = b;
    p.A = A;
    p.B = B;
    p.C = C;
    p.precision = precision;

    // l = ceil( log2( (b - a) * 10^p ) )
    double range = (b - a) * std::pow(10.0, static_cast<double>(precision));
    p.bitLength = static_cast<int>(std::ceil(std::log2(range)));

    if (p.bitLength < 1) {
        throw std::invalid_argument("problem: domain too small for precision; cannot encode.");
    }

    if (p.bitLength > 63) {
        throw std::overflow_error("problem: bitLength=" + std::to_string(p.bitLength) +
                                  " exceeds 63; reduce the domain range or precision.");
    }

    return p;
}

// evaluation

double Problem::decode(uint64_t X10) const {
    // x = a + (X10 / (2^l - 1)) * (b - a)
    uint64_t maxVal = maxEncoded();
    return a + (static_cast<double>(X10) / static_cast<double>(maxVal)) * (b - a);
}

double Problem::fitness(double x) const {
    return A * x * x + B * x + C;
}
