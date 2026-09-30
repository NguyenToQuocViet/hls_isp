#include "golden_wb_model.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace golden_wb_model {
namespace {
void check(const std::vector<std::uint16_t> &raw, int w, int h) {
    if (w <= 0 || h <= 0 || raw.size() != static_cast<std::size_t>(w * h))
        throw std::invalid_argument("WB: invalid frame size");
}
}

std::uint16_t quantize_gain_q4_12(double gain) {
    // gain_t in the DUT is ap_ufixed<16,4> with the default AP_TRN mode.
    // For positive gains AP_TRN is equivalent to floor at Q4.12.
    const double q = std::floor(gain * 4096.0);
    return static_cast<std::uint16_t>(std::max(0.0, std::min(65535.0, q)));
}

std::vector<double> wb_float(
    const std::vector<std::uint16_t> &raw, int w, int h,
    const FloatGains &g) {
    check(raw, w, h);
    std::vector<double> out(raw.size());
    for (int r = 0; r < h; ++r) for (int c = 0; c < w; ++c) {
        const double gain = !(r & 1) ? (!(c & 1) ? g.r : g.gr)
                                         : (!(c & 1) ? g.gb : g.b);
        out[r * w + c] = std::max(0.0, std::min(4095.0, raw[r * w + c] * 4.0 * gain));
    }
    return out;
}

std::vector<std::uint16_t> wb_fixed(
    const std::vector<std::uint16_t> &raw, int w, int h,
    const FixedGainsQ12 &g) {
    check(raw, w, h);
    std::vector<std::uint16_t> out(raw.size());
    for (int r = 0; r < h; ++r) for (int c = 0; c < w; ++c) {
        if (raw[r * w + c] > 1023) throw std::invalid_argument("WB: input is not RAW10");
        const std::uint16_t gain = !(r & 1) ? (!(c & 1) ? g.r : g.gr)
                                                : (!(c & 1) ? g.gb : g.b);
        
        const std::uint64_t product_q12 =
            static_cast<std::uint64_t>(raw[r * w + c]) * gain;

        // RAW10 -> mien 12bit bang cach nhan 4
        const std::uint64_t scaled_product_q12 = product_q12 << 2;

        // lam tron Q4.12 roi saturation ve 12bit
        const std::uint64_t rounded = (scaled_product_q12 + 2048) >> 12;
        out[r * w + c] = static_cast<std::uint16_t>(std::min<std::uint64_t>(rounded, 4095));
    }
    return out;
}

} // namespace golden_wb_model
