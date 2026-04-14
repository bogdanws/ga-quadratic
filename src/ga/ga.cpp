#include "ga/ga.hpp"

#include "ga/chromosome.hpp"
#include "ga/population.hpp"
#include "ga/problem.hpp"
#include "ga/rng.hpp"

#include <algorithm>
#include <stdexcept>
#include <vector>

Ga::Ga(const GaParams& params, Rng& rng)
    : problem_(Problem::make(params.a, params.b, params.A, params.B, params.C, params.precision)),
      rng_(rng), population_(), crossoverProb_(params.crossoverProb),
      mutationProb_(params.mutationProb), gen_(0) {
    if (params.populationSize < 2)
        throw std::invalid_argument("ga: population size must be >= 2");
    if (params.generations < 1)
        throw std::invalid_argument("ga: number of generations must be >= 1");
    if (params.crossoverProb < 0.0 || params.crossoverProb > 1.0)
        throw std::invalid_argument("ga: crossover probability must be in [0, 1]");
    if (params.mutationProb < 0.0 || params.mutationProb > 1.0)
        throw std::invalid_argument("ga: mutation probability must be in [0, 1]");

    population_ = Population(params.populationSize, problem_, rng);
}

// getters

const Population& Ga::current() const {
    return population_;
}
const Problem& Ga::problem() const {
    return problem_;
}
int Ga::currentGeneration() const {
    return gen_;
}

// roulette-wheel selection with elitism: picks n-1 parents, then appends the elite to the last slot
// so it always survives
static Population doSelection(const Population& pop, int eliteIdx, Rng& rng,
                              std::vector<double>* outProbs, std::vector<double>* outCumul,
                              std::vector<GenerationTrace::Draw>* outDraws) {
    int n = pop.size();

    std::vector<double> probs = pop.selectionProbs();
    std::vector<double> cumul = pop.cumulativeProbs();

    if (outProbs)
        *outProbs = probs;
    if (outCumul)
        *outCumul = cumul;

    std::vector<Chromosome> selected;
    selected.reserve(n);

    // spin the wheel n-1 times; the elite takes the last slot
    for (int draw = 0; draw < n - 1; ++draw) {
        double u = rng.uniform01();
        auto it = std::upper_bound(cumul.begin(), cumul.end(), u);
        int idx = static_cast<int>(it - cumul.begin()) - 1;

        if (outDraws) {
            outDraws->push_back({u, idx});
        }

        selected.push_back(pop[idx]);
    }

    selected.push_back(pop[eliteIdx]);

    return Population(std::move(selected));
}

// one-point crossover, in place
// the elite (last slot) is never touched
static void doCrossover(Population& selected, int l, double p_c, Rng& rng, const Problem& pb,
                        std::vector<GenerationTrace::Participant>* outParts,
                        std::vector<CrossoverEvent>* outEvents) {
    if (l < 2) // a single bit has no interior cut point
        return;
    int n = selected.size();
    std::vector<int> participants;

    // roll for each non-elite chromosome: does it join the crossover pool?
    for (int i = 0; i < n - 1; ++i) {
        double u = rng.uniform01();
        bool participates = (u < p_c);
        if (outParts) {
            outParts->push_back({i, u, participates});
        }
        if (participates) {
            participants.push_back(i);
        }
    }

    rng.shuffle(participants.begin(), participants.end());

    // pick from the pool two at a time
    // if the count is odd the last one gets left out (no crossover, just copied)
    for (int k = 0; k + 1 < static_cast<int>(participants.size()); k += 2) {
        int idxA = participants[k];
        int idxB = participants[k + 1];

        int cut = rng.uniformInt(1, l - 1);

        std::string pABits = selected[idxA].bitString();
        std::string pBBits = selected[idxB].bitString();

        std::vector<bool> newA(l), newB(l);
        for (int b = 0; b < cut; ++b) {
            newA[b] = selected[idxB].bits[b];
            newB[b] = selected[idxA].bits[b];
        }
        for (int b = cut; b < l; ++b) {
            newA[b] = selected[idxA].bits[b];
            newB[b] = selected[idxB].bits[b];
        }
        selected[idxA] = Chromosome::fromBits(std::move(newA), pb);
        selected[idxB] = Chromosome::fromBits(std::move(newB), pb);

        if (outEvents) {
            CrossoverEvent ev;
            ev.parentA = idxA;
            ev.parentB = idxB;
            ev.cutPoint = cut;
            ev.parentABits = pABits;
            ev.parentBBits = pBBits;
            ev.childABits = selected[idxA].bitString();
            ev.childBBits = selected[idxB].bitString();
            outEvents->push_back(std::move(ev));
        }
    }
}

// per-bit mutation. flips each bit with probability p_m; elite is skipped
static void doMutation(Population& pop, int l, double p_m, Rng& rng, const Problem& pb,
                       std::vector<int>* outMutated) {
    int n = pop.size();
    std::vector<int> modified;

    for (int i = 0; i < n - 1; ++i) {
        bool changed = false;
        for (int bit = 0; bit < l; ++bit) {
            double u = rng.uniform01();
            if (u < p_m) {
                pop[i].bits[bit] = !pop[i].bits[bit];
                changed = true;
            }
        }
        // only re-evaluate fitness if we actually flipped something
        if (changed) {
            pop[i].evaluate(pb);
            modified.push_back(i);
        }
    }

    if (outMutated) {
        std::sort(modified.begin(), modified.end());
        *outMutated = std::move(modified);
    }
}

GenerationStats Ga::step(GenerationTrace* trace) {
    int nextGen = gen_ + 1;
    int l = problem_.bitLength;
    int eliteIdx = population_.argmax();

    if (trace) {
        // snapshot the input population so the caller can log the "before" state
        trace->initial = population_;
        trace->eliteIndex = eliteIdx;
    }

    Population working = doSelection(population_, eliteIdx, rng_,
                                     trace ? &trace->selectionProbs : nullptr,
                                     trace ? &trace->cumulativeProbs : nullptr,
                                     trace ? &trace->draws : nullptr);
    if (trace)
        trace->afterSelection = working;

    doCrossover(working, l, crossoverProb_, rng_, problem_,
                trace ? &trace->crossoverDraws : nullptr,
                trace ? &trace->crossovers : nullptr);
    if (trace)
        trace->afterCrossover = working;

    doMutation(working, l, mutationProb_, rng_, problem_,
               trace ? &trace->mutatedIndices : nullptr);
    if (trace)
        trace->afterMutation = working;

    population_ = std::move(working);
    gen_ = nextGen;

    GenerationStats stats;
    stats.generation = gen_;
    stats.maxFitness = population_.maxFitness();
    stats.meanFitness = population_.meanFitness();
    stats.bestIndex = population_.argmax();

    if (trace)
        trace->stats = stats;

    return stats;
}
