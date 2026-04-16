#include "io/logger.hpp"

#include "ga/population.hpp"

#include <iomanip>
#include <ostream>

static void writePopulation(std::ostream& out, const std::string& title, const Population& pop) {
    out << title << "\n";
    for (int i = 0; i < pop.size(); ++i) {
        const auto& c = pop[i];
        out << std::setw(4) << (i + 1) << ": " << c.bitString() << " x=" << c.x
            << " f=" << c.fitness << "\n";
    }
    out << "\n";
}

static void writeSelection(std::ostream& out, const GenerationTrace& t) {
    out << "Selection probabilities\n";
    for (int i = 0; i < static_cast<int>(t.selectionProbs.size()); ++i)
        out << "  chromosome " << std::setw(4) << (i + 1) << " probability " << t.selectionProbs[i]
            << "\n";
    out << "\n";

    out << "Cumulative selection intervals\n";
    for (double v : t.cumulativeProbs)
        out << v << " ";
    out << "\n\n";

    out << "Selection draws\n";
    for (const auto& d : t.draws)
        out << "  u=" << d.u << " -> chromosome " << d.chosenIndex + 1 << "\n";
    out << "\n";
}

static void writeCrossover(std::ostream& out, double p_c, const GenerationTrace& t) {
    out << "Crossover probability " << p_c << "\n";
    for (const auto& p : t.crossoverDraws) {
        out << "  " << p.idx + 1 << ": u=" << p.u;
        if (p.participates)
            out << " participates";
        out << "\n";
    }
    out << "\n";

    if (t.crossovers.empty()) {
        out << "No crossover events\n\n";
        return;
    }
    out << "Crossover events\n";
    for (const auto& ev : t.crossovers) {
        out << "  chromosomes " << ev.parentA + 1 << " x " << ev.parentB + 1 << " cut=" << ev.cutPoint
            << "\n";
        out << "    " << ev.parentABits << " " << ev.parentBBits << "\n";
        out << "    " << ev.childABits << " " << ev.childBBits << "\n";
    }
    out << "\n";
}

static void writeMutation(std::ostream& out, double p_m, const GenerationTrace& t) {
    out << "Mutation probability " << p_m << "\n";
    out << "Mutated chromosomes:";
    if (t.mutatedIndices.empty()) {
        out << " none";
    } else {
        for (int idx : t.mutatedIndices)
            out << " " << idx + 1;
    }
    out << "\n\n";
}

void writeGen1Trace(std::ostream& out, const GaParams& params, const GenerationTrace& trace) {
    writePopulation(out, "Initial population", trace.initial);
    writeSelection(out, trace);
    writePopulation(out, "After selection", trace.afterSelection);
    writeCrossover(out, params.crossoverProb, trace);
    writePopulation(out, "After crossover", trace.afterCrossover);
    writeMutation(out, params.mutationProb, trace);
    writePopulation(out, "After mutation", trace.afterMutation);

    out << "Evolution (max mean)\n";
    writeEvolutionLine(out, trace.stats.maxFitness, trace.stats.meanFitness);
}

void writeEvolutionLine(std::ostream& out, double maxFitness, double meanFitness) {
    out << maxFitness << " " << meanFitness << "\n";
}
