#pragma once

#include "ga/chromosome.hpp"

#include <vector>

struct Problem;
class Rng;

class Population {
  public:
    Population() = default;                         // empty population
    Population(int n, const Problem& pb, Rng& rng); // random init
    explicit Population(std::vector<Chromosome> v);

    int size() const;
    Chromosome& operator[](int i);
    const Chromosome& operator[](int i) const;

    int argmax() const;
    double maxFitness() const;
    double meanFitness() const;
    double totalFitness() const;

    std::vector<double> selectionProbs() const;
    std::vector<double> cumulativeProbs() const;

  private:
    std::vector<Chromosome> chromos_;
};
