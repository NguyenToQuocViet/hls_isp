#include "isp_golden_fixed.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace isp_golden {
namespace {

const std::int64_t Q12_HALF = INT64_C(1) << 11;
const std::int64_t Q4_HALF = INT64_C(1) << 3;

std::uint16_t round_sat_q12_to_u12(std::int64_t value_q12) {
    if (value_q12 <= 0) {
        return 0;
    }
    const std::int64_t rounded = (value_q12 + Q12_HALF) >> 12;
    return static_cast<std::uint16_t>(std::min<std::int64_t>(rounded, 4095));
}

std::uint16_t round_sat_q4_to_u12(std::int64_t value_q4) {
    if (value_q4 <= 0) {
        return 0;
    }
    const std::int64_t rounded = (value_q4 + Q4_HALF) >> 4;
    return static_cast<std::uint16_t>(std::min<std::int64_t>(rounded, 4095));
}

int mirror101(int coordinate, int limit) {
    if (coordinate < 0) {
        return 1;
    }
    if (coordinate >= limit) {
        return limit - 2;
    }
    return coordinate;
}

std::size_t index_of(int row, int col, int width) {
    return static_cast<std::size_t>(row) * static_cast<std::size_t>(width)
         + static_cast<std::size_t>(col);
}

void check_frame_size(std::size_t size, int width, int height) {
    if (width < 2 || height < 2) {
        throw std::invalid_argument("golden model requires width,height >= 2");
    }
    const std::size_t expected =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    if (size != expected) {
        throw std::invalid_argument("input vector size does not match width*height");
    }
}

} // namespace

std::uint16_t encode_ufixed_q12(double value) {
    if (value <= 0.0) {
        return 0;
    }
    const double scaled = std::floor(value * 4096.0 + 0.5);
    return static_cast<std::uint16_t>(std::min(scaled, 65535.0));
}

std::int16_t encode_fixed_q12(double value) {
    const double scaled = std::floor(value * 4096.0 + 0.5);
    const double clipped = std::max(-32768.0, std::min(32767.0, scaled));
    return static_cast<std::int16_t>(clipped);
}

std::uint16_t wb_pixel(std::uint16_t raw10, std::uint16_t gain_q12) {
    const std::int64_t product_q12 =
        (static_cast<std::int64_t>(raw10 & 0x03ffu) << 2) * gain_q12;
    return round_sat_q12_to_u12(product_q12);
}

std::vector<std::uint16_t> wb_frame(
    const std::vector<std::uint16_t> &raw10,
    int width,
    int height,
    const WbConfigQ12 &cfg
) {
    check_frame_size(raw10.size(), width, height);
    std::vector<std::uint16_t> out(raw10.size());

    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            const bool row_even = (row & 1) == 0;
            const bool col_even = (col & 1) == 0;
            const std::uint16_t gain = row_even
                ? (col_even ? cfg.r : cfg.gr)
                : (col_even ? cfg.gb : cfg.b);
            const std::size_t i = index_of(row, col, width);
            out[i] = wb_pixel(raw10[i], gain);
        }
    }
    return out;
}

std::vector<Rgb12> demosaic_frame(
    const std::vector<std::uint16_t> &raw12,
    int width,
    int height,
    int edge_threshold,
    int edge_mag
) {
    check_frame_size(raw12.size(), width, height);
    const std::size_t count = raw12.size();

    std::vector<std::uint32_t> green_q4(count, 0);
    std::vector<std::uint8_t> direction(count, 0);

    for (int row = 0; row < height; ++row) {
        std::uint8_t dir_left = 0;
        for (int col = 0; col < width; ++col) {
            const std::size_t center = index_of(row, col, width);
            const bool row_even = (row & 1) == 0;
            const bool col_even = (col & 1) == 0;
            const bool rb_site = row_even == col_even;

            if (!rb_site) {
                green_q4[center] =
                    static_cast<std::uint32_t>(raw12[center] & 0x0fffu) << 4;
                direction[center] = dir_left;
                continue;
            }

            const int cl = mirror101(col - 1, width);
            const int cr = mirror101(col + 1, width);
            const int rt = mirror101(row - 1, height);
            const int rb = mirror101(row + 1, height);

            const std::uint16_t left = raw12[index_of(row, cl, width)] & 0x0fffu;
            const std::uint16_t right = raw12[index_of(row, cr, width)] & 0x0fffu;
            const std::uint16_t top = raw12[index_of(rt, col, width)] & 0x0fffu;
            const std::uint16_t bottom = raw12[index_of(rb, col, width)] & 0x0fffu;

            const std::uint32_t gh_q4 =
                static_cast<std::uint32_t>((left + right) >> 1) << 4;
            const std::uint32_t gv_q4 =
                static_cast<std::uint32_t>((top + bottom) >> 1) << 4;
            const int grad_h = std::abs(static_cast<int>(left) - static_cast<int>(right));
            const int grad_v = std::abs(static_cast<int>(top) - static_cast<int>(bottom));
            const int diff = std::abs(grad_h - grad_v);
            const int grad_max = std::max(grad_h, grad_v);

            std::uint8_t chosen = 0;
            std::uint32_t result_q4 = 0;
            if (grad_max < edge_mag) {
                result_q4 = (gh_q4 + gv_q4) >> 1;
                chosen = dir_left;
            } else if (diff > edge_threshold) {
                chosen = (grad_h > grad_v) ? 1 : 0;
                result_q4 = chosen ? gv_q4 : gh_q4;
            } else {
                int vote = dir_left;
                if (row != 0) {
                    const int prev_left_col = (col == 0) ? col : col - 1;
                    const int prev_right_col =
                        (col == width - 1) ? col : col + 1;
                    vote += direction[index_of(row - 1, prev_left_col, width)];
                    vote += direction[index_of(row - 1, col, width)];
                    vote += direction[index_of(row - 1, prev_right_col, width)];
                }
                chosen = (vote > 2) ? 1 : 0;
                result_q4 = chosen ? gv_q4 : gh_q4;
            }

            green_q4[center] = result_q4;
            direction[center] = chosen;
            dir_left = chosen;
        }
    }

    std::vector<Rgb12> out(count);
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            const std::size_t center = index_of(row, col, width);
            const bool row_even = (row & 1) == 0;
            const bool col_even = (col & 1) == 0;
            const int cl = mirror101(col - 1, width);
            const int cr = mirror101(col + 1, width);
            const int rt = mirror101(row - 1, height);
            const int rb = mirror101(row + 1, height);

            const std::int64_t gc = green_q4[center];
            std::int64_t r_q4 = 0;
            std::int64_t b_q4 = 0;

            if (row_even && col_even) {
                const std::int64_t avg_b = (
                    raw12[index_of(rt, cl, width)]
                  + raw12[index_of(rt, cr, width)]
                  + raw12[index_of(rb, cl, width)]
                  + raw12[index_of(rb, cr, width)]) >> 2;
                const std::int64_t avg_g_q4 = (
                    green_q4[index_of(rt, cl, width)]
                  + green_q4[index_of(rt, cr, width)]
                  + green_q4[index_of(rb, cl, width)]
                  + green_q4[index_of(rb, cr, width)]) >> 2;
                r_q4 = static_cast<std::int64_t>(raw12[center]) << 4;
                b_q4 = gc + (avg_b << 4) - avg_g_q4;
            } else if (row_even && !col_even) {
                const std::int64_t avg_r =
                    (raw12[index_of(row, cl, width)]
                   + raw12[index_of(row, cr, width)]) >> 1;
                const std::int64_t avg_gr_q4 =
                    (green_q4[index_of(row, cl, width)]
                   + green_q4[index_of(row, cr, width)]) >> 1;
                const std::int64_t avg_b =
                    (raw12[index_of(rt, col, width)]
                   + raw12[index_of(rb, col, width)]) >> 1;
                const std::int64_t avg_gb_q4 =
                    (green_q4[index_of(rt, col, width)]
                   + green_q4[index_of(rb, col, width)]) >> 1;
                r_q4 = gc + (avg_r << 4) - avg_gr_q4;
                b_q4 = gc + (avg_b << 4) - avg_gb_q4;
            } else if (!row_even && col_even) {
                const std::int64_t avg_r =
                    (raw12[index_of(rt, col, width)]
                   + raw12[index_of(rb, col, width)]) >> 1;
                const std::int64_t avg_gr_q4 =
                    (green_q4[index_of(rt, col, width)]
                   + green_q4[index_of(rb, col, width)]) >> 1;
                const std::int64_t avg_b =
                    (raw12[index_of(row, cl, width)]
                   + raw12[index_of(row, cr, width)]) >> 1;
                const std::int64_t avg_gb_q4 =
                    (green_q4[index_of(row, cl, width)]
                   + green_q4[index_of(row, cr, width)]) >> 1;
                r_q4 = gc + (avg_r << 4) - avg_gr_q4;
                b_q4 = gc + (avg_b << 4) - avg_gb_q4;
            } else {
                const std::int64_t avg_r = (
                    raw12[index_of(rt, cl, width)]
                  + raw12[index_of(rt, cr, width)]
                  + raw12[index_of(rb, cl, width)]
                  + raw12[index_of(rb, cr, width)]) >> 2;
                const std::int64_t avg_g_q4 = (
                    green_q4[index_of(rt, cl, width)]
                  + green_q4[index_of(rt, cr, width)]
                  + green_q4[index_of(rb, cl, width)]
                  + green_q4[index_of(rb, cr, width)]) >> 2;
                r_q4 = gc + (avg_r << 4) - avg_g_q4;
                b_q4 = static_cast<std::int64_t>(raw12[center]) << 4;
            }

            out[center].r = round_sat_q4_to_u12(r_q4);
            out[center].g = round_sat_q4_to_u12(gc);
            out[center].b = round_sat_q4_to_u12(b_q4);
        }
    }
    return out;
}

Rgb12 ccm_pixel(const Rgb12 &in, const CcmConfigQ12 &cfg) {
    const std::int64_t r_q12 =
        static_cast<std::int64_t>(in.r) * cfg.a00
      + static_cast<std::int64_t>(in.g) * cfg.a01
      + static_cast<std::int64_t>(in.b) * cfg.a02;
    const std::int64_t g_q12 =
        static_cast<std::int64_t>(in.r) * cfg.a10
      + static_cast<std::int64_t>(in.g) * cfg.a11
      + static_cast<std::int64_t>(in.b) * cfg.a12;
    const std::int64_t b_q12 =
        static_cast<std::int64_t>(in.r) * cfg.a20
      + static_cast<std::int64_t>(in.g) * cfg.a21
      + static_cast<std::int64_t>(in.b) * cfg.a22;

    Rgb12 out;
    out.r = round_sat_q12_to_u12(r_q12);
    out.g = round_sat_q12_to_u12(g_q12);
    out.b = round_sat_q12_to_u12(b_q12);
    return out;
}

std::vector<Rgb12> ccm_frame(
    const std::vector<Rgb12> &rgb,
    const CcmConfigQ12 &cfg
) {
    std::vector<Rgb12> out(rgb.size());
    for (std::size_t i = 0; i < rgb.size(); ++i) {
        out[i] = ccm_pixel(rgb[i], cfg);
    }
    return out;
}

std::vector<Rgb12> pipeline_frame(
    const std::vector<std::uint16_t> &raw10,
    int width,
    int height,
    const WbConfigQ12 &wb_cfg,
    int edge_threshold,
    int edge_mag,
    const CcmConfigQ12 &ccm_cfg
) {
    const std::vector<std::uint16_t> wb =
        wb_frame(raw10, width, height, wb_cfg);
    const std::vector<Rgb12> demosaic =
        demosaic_frame(wb, width, height, edge_threshold, edge_mag);
    return ccm_frame(demosaic, ccm_cfg);
}

} // namespace isp_golden
