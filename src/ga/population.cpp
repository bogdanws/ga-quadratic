#include "ga/population.hpp"

#include "ga/chromosome.hpp"
#include "ga/problem.hpp"
#include "ga/rng.hpp"

#include <algorithm>
#include <numeric>

// constructors

Population::Population(int n, const Problem& pb, Rng& rng) {
    chromos_.reserve(n);
    for (int i = 0; i < n; ++i) {
        chromos_.push_back(Chromosome::random(pb, rng));
    }
}

Population::Population(std::vector<Chromosome> v) : chromos_(std::move(v)) {}

// getters

int Population::size() const {
    return static_cast<int>(chromos_.size());
}

Chromosome& Population::operator[](int i) {
    return chromos_[i];
}

const Chromosome& Population::operator[](int i) const {
    return chromos_[i];
}

// statistics

int Population::argmax() const {
    int best = 0;
    for (int i = 1; i < size(); ++i) {
        if (chromos_[i].fitness > chromos_[best].fitness) {
            best = i;
        }
    }
    return best;
}

double Population::maxFitness() const {
    return chromos_[argmax()].fitness;
}

double Population::meanFitness() const {
    return totalFitness() / static_cast<double>(size());
}

double Population::totalFitness() const {
    double sum = 0.0;
    for (const auto& c : chromos_) {
        sum += c.fitness;
    }
    return sum;
}

std::vector<double> Population::selectionProbs() const {
    double total = totalFitness();
    std::vector<double> probs(chromos_.size());
    for (int i = 0; i < size(); ++i) {
        probs[i] = chromos_[i].fitness / total;
    }
    return probs;
}

std::vector<double> Population::cumulativeProbs() const {
    std::vector<double> probs = selectionProbs();
    int n = size();
    // returns n+1 values: [0, q_1, q_2, ..., q_n = 1.0]
    std::vector<double> q(n + 1);
    q[0] = 0.0;
    for (int i = 0; i < n; ++i) {
        q[i + 1] = q[i] + probs[i];
    }
    // clamp last element to 1.0 so upper_bound never returns cumul.end() for u near 1.0
    q[n] = 1.0;
    return q;
}
