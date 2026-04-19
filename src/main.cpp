#include "ga/ga.hpp"
#include "ga/rng.hpp"
#include "io/config.hpp"
#include "io/logger.hpp"

#ifdef GA_BUILD_GUI
#include "gui/app.hpp"
#endif

#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>

static int runHeadless(const Config& cfg, Rng& rng) {
    std::ofstream out(cfg.outputPath, std::ios::trunc);
    if (!out)
        throw std::runtime_error("cannot open output file: " + cfg.outputPath);
    // match the x-precision requested in the problem so probabilities, x values
    // and fitness all get the same decimal rendering
    out << std::fixed << std::setprecision(cfg.params.precision);

    Ga ga(cfg.params, rng);

    GenerationTrace t;
    ga.step(&t);
    writeGen1Trace(out, cfg.params, t);

    for (int g = 2; g <= cfg.params.generations; ++g) {
        GenerationStats s = ga.step();
        writeEvolutionLine(out, s.maxFitness, s.meanFitness);
    }

    return 0;
}

int main(int argc, char** argv) {
    Config cfg;
    try {
        cfg = parseCli(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        printUsage(std::cerr);
        return 2;
    }

    uint64_t seed = cfg.seed.value_or(std::random_device{}());

    if (cfg.gui) {
#ifdef GA_BUILD_GUI
        try {
            return gui::runGui(cfg.params, seed);
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
            return 1;
        }
#else
        std::cerr << "Built without GUI support. Reconfigure with -DGA_BUILD_GUI=ON.\n";
        return 1;
#endif
    }

    Rng rng(seed);
    try {
        return runHeadless(cfg, rng);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
