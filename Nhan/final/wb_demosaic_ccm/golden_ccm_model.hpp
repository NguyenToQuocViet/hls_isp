#ifndef GOLDEN_CCM_MODEL_H_
#define GOLDEN_CCM_MODEL_H_

#include <cstdint>
#include <vector>

namespace golden_ccm_model {

struct RgbFloat { double r, g, b; };
struct Rgb12 { std::uint16_t r, g, b; };
struct FloatMatrix { double a[3][3]; };
struct FixedMatrixQ12 { std::int16_t a[3][3]; };

std::int16_t quantize_coeff_q4_12(double value);
std::vector<RgbFloat> ccm_float(
    const std::vector<RgbFloat> &input, const FloatMatrix &matrix);
std::vector<Rgb12> ccm_fixed(
    const std::vector<Rgb12> &input, const FixedMatrixQ12 &matrix);

} // namespace golden_ccm_model
#endif
