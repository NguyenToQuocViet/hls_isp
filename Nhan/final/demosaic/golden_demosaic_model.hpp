#ifndef GOLDEN_DEMOSAIC_MODEL_H_
#define GOLDEN_DEMOSAIC_MODEL_H_

#include <cstdint>
#include <vector>

namespace golden_demosaic_model {

struct RgbFloat { double r, g, b; };
struct Rgb12 { std::uint16_t r, g, b; };

std::vector<RgbFloat> demosaic_float(
    const std::vector<std::uint16_t> &raw12,
    int width, int height,
    double edge_threshold,
    double edge_mag);

std::vector<Rgb12> demosaic_fixed(
    const std::vector<std::uint16_t> &raw12,
    int width, int height,
    int edge_threshold,
    int edge_mag);

} // namespace golden_demosaic_model
#endif
