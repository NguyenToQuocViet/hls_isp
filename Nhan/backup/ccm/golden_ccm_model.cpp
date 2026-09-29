#include "golden_ccm_model.hpp"

#include <algorithm>
#include <cmath>

namespace golden_ccm_model {
namespace {
double clip_float(double x) { return std::max(0.0, std::min(4095.0, x)); }
std::uint16_t round_sat_q12(std::int64_t x) {
    if (x <= 0) return 0;
    const std::int64_t y = (x + 2048) >> 12;
    return static_cast<std::uint16_t>(std::min<std::int64_t>(y, 4095));
}
}

std::int16_t quantize_coeff_q4_12(double value) {
    // gain_matrix in the DUT is ap_fixed<16,4> with the default AP_TRN mode,
    // which rounds toward minus infinity for both positive and negative values.
    const double q = std::floor(value * 4096.0);
    return static_cast<std::int16_t>(std::max(-32768.0, std::min(32767.0, q)));
}

std::vector<RgbFloat> ccm_float(
    const std::vector<RgbFloat> &in, const FloatMatrix &m) {
    std::vector<RgbFloat> out(in.size());
    for (std::size_t i = 0; i < in.size(); ++i) {
        out[i].r = clip_float(in[i].r*m.a[0][0] + in[i].g*m.a[0][1] + in[i].b*m.a[0][2]);
        out[i].g = clip_float(in[i].r*m.a[1][0] + in[i].g*m.a[1][1] + in[i].b*m.a[1][2]);
        out[i].b = clip_float(in[i].r*m.a[2][0] + in[i].g*m.a[2][1] + in[i].b*m.a[2][2]);
    }
    return out;
}

std::vector<Rgb12> ccm_fixed(
    const std::vector<Rgb12> &in, const FixedMatrixQ12 &m) {
    std::vector<Rgb12> out(in.size());
    for (std::size_t i = 0; i < in.size(); ++i) {
        const std::int64_t r = static_cast<std::int64_t>(in[i].r)*m.a[0][0]
                             + static_cast<std::int64_t>(in[i].g)*m.a[0][1]
                             + static_cast<std::int64_t>(in[i].b)*m.a[0][2];
        const std::int64_t g = static_cast<std::int64_t>(in[i].r)*m.a[1][0]
                             + static_cast<std::int64_t>(in[i].g)*m.a[1][1]
                             + static_cast<std::int64_t>(in[i].b)*m.a[1][2];
        const std::int64_t b = static_cast<std::int64_t>(in[i].r)*m.a[2][0]
                             + static_cast<std::int64_t>(in[i].g)*m.a[2][1]
                             + static_cast<std::int64_t>(in[i].b)*m.a[2][2];
        out[i] = {round_sat_q12(r), round_sat_q12(g), round_sat_q12(b)};
    }
    return out;
}

} // namespace golden_ccm_model
