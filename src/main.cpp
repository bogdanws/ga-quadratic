#include "ga/chromosome.hpp"
#include "ga/problem.hpp"
#include "ga/rng.hpp"

#include <iostream>
#include <random>

int main() {
    // f(x) = -x^2 + x + 2 pe [-1, 2], precizie 6
    Problem pb = Problem::make(-1.0, 2.0, -1.0, 1.0, 2.0, 6);
    std::cout << "bitLength = " << pb.bitLength << "\n";

    std::cout << "decode(0) = " << pb.decode(0) << "\n";
    std::cout << "decode(max) = " << pb.decode(pb.maxEncoded()) << "\n\n";

    Rng rng(std::random_device{}());
    for (int i = 0; i < 5; ++i) {
        Chromosome c = Chromosome::random(pb, rng);
        std::cout << "  " << c.bitString() << "  x=" << c.x << "  f=" << c.fitness << "\n";
    }
    return 0;
}
