#pragma once

#include "ga/ga.hpp"

#include <cstdint>
#include <iosfwd>
#include <optional>
#include <string>

struct Config {
    GaParams params;
    std::string outputPath = "evolution.txt";
    bool gui = false;
    std::optional<uint64_t> seed;
};

Config parseCli(int argc, char** argv);
GaParams readGaParamsFromStream(std::istream& in);
void printUsage(std::ostream& os);
