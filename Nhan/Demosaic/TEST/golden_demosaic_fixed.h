#ifndef GOLDEN_DEMOSAIC_FIXED_H_
#define GOLDEN_DEMOSAIC_FIXED_H_

#include <cstdint>
#include <vector>

namespace golden_demosaic {

struct Rgb12 { std::uint16_t r, g, b; };

std::vector<Rgb12> frame(
    const std::vector<std::uint16_t> &raw12,
    int width,
    int height,
    int edge_threshold,
    int edge_mag
);

} // namespace golden_demosaic

#endif
