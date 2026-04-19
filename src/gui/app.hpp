#pragma once

#include "ga/ga.hpp" // GaParams

#include <cstdint>

namespace gui {

int runGui(const GaParams& initialParams, uint64_t seed);

} // namespace gui
