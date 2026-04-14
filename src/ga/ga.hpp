#pragma once

#include "ga/population.hpp"
#include "ga/problem.hpp"

#include <string>
#include <vector>

struct GaParams {
    int populationSize;
    double a, b, A, B, C;
    int precision;
    double crossoverProb;
    double mutationProb;
    int generations;
};

struct GenerationStats {
    int generation;
    double maxFitness;
    double meanFitness;
    int bestIndex;
};

struct CrossoverEvent {
    int parentA, parentB;
    int cutPoint;
    std::string parentABits, parentBBits;
    std::string childABits, childBBits;
};

struct GenerationTrace {
    struct Draw {
        double u;
        int chosenIndex;
    };
    struct Participant {
        int idx;
        double u;
        bool participates;
    };

    Population initial;
    std::vector<double> selectionProbs;
    std::vector<double> cumulativeProbs;
    std::vector<Draw> draws;
    int eliteIndex;
    Population afterSelection;
    std::vector<Participant> crossoverDraws;
    std::vector<CrossoverEvent> crossovers;
    Population afterCrossover;
    std::vector<int> mutatedIndices;
    Population afterMutation;
    GenerationStats stats;
};

class Ga {
  public:
    Ga(const GaParams& params, Rng& rng);

    // pass a non-null trace to capture full per-step detail; null for the fast path
    GenerationStats step(GenerationTrace* trace = nullptr);

    const Population& current() const;
    const Problem& problem() const;
    int currentGeneration() const;

  private:
    Problem problem_;
    Rng& rng_;
    Population population_;
    double crossoverProb_;
    double mutationProb_;
    int gen_;
};
