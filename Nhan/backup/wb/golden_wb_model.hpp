#ifndef GOLDEN_WB_MODEL_H_
#define GOLDEN_WB_MODEL_H_

#include <cstdint>
#include <vector>

namespace golden_wb_model {

struct FloatGains { double r, gr, gb, b; };
struct FixedGainsQ12 { std::uint16_t r, gr, gb, b; };

std::uint16_t quantize_gain_q4_12(double gain);

std::vector<double> wb_float(
    const std::vector<std::uint16_t> &raw10,
    int width, int height,
    const FloatGains &gains);

std::vector<std::uint16_t> wb_fixed(
    const std::vector<std::uint16_t> &raw10,
    int width, int height,
    const FixedGainsQ12 &gains);

} // namespace golden_wb_model
#endif
