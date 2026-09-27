#ifndef ISP_GOLDEN_FIXED_H_
#define ISP_GOLDEN_FIXED_H_

#include <cstddef>
#include <cstdint>
#include <vector>

namespace isp_golden {

struct WbConfigQ12 {
    std::uint16_t r;
    std::uint16_t gr;
    std::uint16_t gb;
    std::uint16_t b;
};

struct CcmConfigQ12 {
    std::int16_t a00, a01, a02;
    std::int16_t a10, a11, a12;
    std::int16_t a20, a21, a22;
};

struct Rgb12 {
    std::uint16_t r;
    std::uint16_t g;
    std::uint16_t b;
};

std::uint16_t encode_ufixed_q12(double value);
std::int16_t encode_fixed_q12(double value);

std::uint16_t wb_pixel(
    std::uint16_t raw10,
    std::uint16_t gain_q12
);

std::vector<std::uint16_t> wb_frame(
    const std::vector<std::uint16_t> &raw10,
    int width,
    int height,
    const WbConfigQ12 &cfg
);

std::vector<Rgb12> demosaic_frame(
    const std::vector<std::uint16_t> &raw12,
    int width,
    int height,
    int edge_threshold,
    int edge_mag
);

Rgb12 ccm_pixel(
    const Rgb12 &in,
    const CcmConfigQ12 &cfg
);

std::vector<Rgb12> ccm_frame(
    const std::vector<Rgb12> &rgb,
    const CcmConfigQ12 &cfg
);

std::vector<Rgb12> pipeline_frame(
    const std::vector<std::uint16_t> &raw10,
    int width,
    int height,
    const WbConfigQ12 &wb_cfg,
    int edge_threshold,
    int edge_mag,
    const CcmConfigQ12 &ccm_cfg
);

} // namespace isp_golden

#endif
