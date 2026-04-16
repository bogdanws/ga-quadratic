#pragma once

#include "ga/ga.hpp"

#include <iosfwd>

// the first generation gets the full output: initial pop, selection, crossover, mutation
void writeGen1Trace(std::ostream& out, const GaParams& params, const GenerationTrace& trace);

// subsequent generations only add one line each
void writeEvolutionLine(std::ostream& out, double maxFitness, double meanFitness);
