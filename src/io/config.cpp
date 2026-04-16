#include "io/config.hpp"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

void printUsage(std::ostream& os) {
    os << "Usage: ga [--in <path>] [--out <path>] [--gui] [--seed <uint>] [--help]\n";
}

// helpers

// returns all whitespace-separated tokens from `in`, skipping comments
static std::vector<std::string> tokenize(std::istream& in) {
    std::vector<std::string> tokens;
    std::string line;
    while (std::getline(in, line)) {
        auto hash = line.find('#');
        if (hash != std::string::npos)
            line.erase(hash);
        std::istringstream ss(line);
        std::string tok;
        while (ss >> tok)
            tokens.push_back(tok);
    }
    return tokens;
}

static int parseInt(const std::string& s, const char* name) {
    try {
        std::size_t pos = 0;
        int v = std::stoi(s, &pos);
        if (pos == s.size())
            return v;
    } catch (...) {
    }
    throw std::invalid_argument(std::string("config: invalid value for ") + name + ": \"" + s + "\"");
}

static double parseDouble(const std::string& s, const char* name) {
    try {
        std::size_t pos = 0;
        double v = std::stod(s, &pos);
        if (pos == s.size())
            return v;
    } catch (...) {
    }
    throw std::invalid_argument(std::string("config: invalid value for ") + name + ": \"" + s + "\"");
}

static uint64_t parseUint64(const std::string& s, const char* name) {
    try {
        std::size_t pos = 0;
        unsigned long long v = std::stoull(s, &pos);
        if (pos == s.size())
            return static_cast<uint64_t>(v);
    } catch (...) {
    }
    throw std::invalid_argument(std::string("config: invalid value for ") + name + ": \"" + s + "\"");
}

static GaParams paramsFromTokens(const std::vector<std::string>& toks) {
    if (toks.size() < 10)
        throw std::invalid_argument("config: not enough parameters (need n  a b  A B C  p  p_c  p_m  T)");

    GaParams p;
    int i = 0;
    p.populationSize = parseInt(toks[i++], "n");
    p.a = parseDouble(toks[i++], "a");
    p.b = parseDouble(toks[i++], "b");
    p.A = parseDouble(toks[i++], "A");
    p.B = parseDouble(toks[i++], "B");
    p.C = parseDouble(toks[i++], "C");
    p.precision = parseInt(toks[i++], "precision");
    p.crossoverProb = parseDouble(toks[i++], "crossover probability");
    p.mutationProb = parseDouble(toks[i++], "mutation probability");
    p.generations = parseInt(toks[i++], "generations");
    return p;
}

// input

GaParams readGaParamsFromStream(std::istream& in) {
    return paramsFromTokens(tokenize(in));
}

// prompts go to stderr so they don't contaminate the token stream on stdin
static GaParams readInteractive() {
    auto prompt = [](const char* msg) -> std::string {
        std::cerr << msg;
        std::cerr.flush();
        std::string s;
        std::getline(std::cin, s);
        return s;
    };

    std::vector<std::string> toks = {
        prompt("Population size n: "),
        prompt("Domain a: "),
        prompt("Domain b: "),
        prompt("Coefficient A (f(x) = A*x^2 + B*x + C): "),
        prompt("Coefficient B: "),
        prompt("Coefficient C: "),
        prompt("Precision (decimal digits): "),
        prompt("Crossover probability p_c [0,1]: "),
        prompt("Mutation probability p_m [0,1]: "),
        prompt("Generations T: "),
    };
    return paramsFromTokens(toks);
}

Config parseCli(int argc, char** argv) {
    Config cfg;
    std::string inPath;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            printUsage(std::cout);
            std::exit(0);
        } else if (std::strcmp(argv[i], "--in") == 0) {
            if (i + 1 >= argc)
                throw std::invalid_argument("config: --in requires a path argument");
            inPath = argv[++i];
        } else if (std::strcmp(argv[i], "--out") == 0) {
            if (i + 1 >= argc)
                throw std::invalid_argument("config: --out requires a path argument");
            cfg.outputPath = argv[++i];
        } else if (std::strcmp(argv[i], "--gui") == 0) {
            cfg.gui = true;
        } else if (std::strcmp(argv[i], "--seed") == 0) {
            if (i + 1 >= argc)
                throw std::invalid_argument("config: --seed requires a uint64 argument");
            cfg.seed = parseUint64(argv[++i], "seed");
        } else {
            throw std::invalid_argument(std::string("config: unknown option: ") + argv[i]);
        }
    }

    if (!inPath.empty()) {
        std::ifstream f(inPath);
        if (!f)
            throw std::invalid_argument("config: cannot open input file: " + inPath);
        cfg.params = readGaParamsFromStream(f);
    } else {
        cfg.params = readInteractive();
    }

    return cfg;
}
