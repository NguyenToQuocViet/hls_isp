#include "golden_demosaic_fixed.h"

#include <algorithm>
#include <cstdlib>
#include <stdexcept>

namespace golden_demosaic {
namespace {

int mirror101(int x, int limit) {
    if (x < 0) return 1;
    if (x >= limit) return limit - 2;
    return x;
}

std::size_t at(int row, int col, int width) {
    return static_cast<std::size_t>(row) * width + col;
}

std::uint16_t round_sat_q4(std::int64_t value) {
    if (value <= 0) return 0;
    const std::int64_t rounded = (value + 8) >> 4;
    return static_cast<std::uint16_t>(
        std::min<std::int64_t>(rounded, 4095));
}

} // namespace

std::vector<Rgb12> frame(
    const std::vector<std::uint16_t> &raw12,
    int width,
    int height,
    int edge_threshold,
    int edge_mag
) {
    if (width < 2 || height < 2 ||
        raw12.size() != static_cast<std::size_t>(width * height)) {
        throw std::invalid_argument("Demosaic golden: invalid frame size");
    }

    const std::size_t count = raw12.size();
    std::vector<std::uint32_t> green_q4(count, 0);
    std::vector<std::uint8_t> direction(count, 0);

    // Tang 1: RAW -> G(Q12.4). Direction duoc tao theo raster nhu DUT.
    for (int row = 0; row < height; ++row) {
        std::uint8_t dir_left = 0;
        for (int col = 0; col < width; ++col) {
            const std::size_t center = at(row, col, width);
            const bool rb_site = ((row & 1) == (col & 1));

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
            const int left = raw12[at(row, cl, width)] & 0x0fff;
            const int right = raw12[at(row, cr, width)] & 0x0fff;
            const int top = raw12[at(rt, col, width)] & 0x0fff;
            const int bottom = raw12[at(rb, col, width)] & 0x0fff;

            const std::uint32_t gh_q4 = ((left + right) >> 1) << 4;
            const std::uint32_t gv_q4 = ((top + bottom) >> 1) << 4;
            const int grad_h = std::abs(left - right);
            const int grad_v = std::abs(top - bottom);
            const int diff = std::abs(grad_h - grad_v);
            const int grad_max = std::max(grad_h, grad_v);
            std::uint8_t chosen;

            if (grad_max < edge_mag) {
                green_q4[center] = (gh_q4 + gv_q4) >> 1;
                chosen = dir_left;
            } else if (diff > edge_threshold) {
                chosen = grad_h > grad_v ? 1 : 0;
                green_q4[center] = chosen ? gv_q4 : gh_q4;
            } else {
                int vote = dir_left;
                if (row != 0) {
                    vote += direction[at(row - 1, mirror101(col - 1, width), width)];
                    vote += direction[at(row - 1, col, width)];
                    vote += direction[at(row - 1, mirror101(col + 1, width), width)];
                }
                chosen = vote > 2 ? 1 : 0;
                green_q4[center] = chosen ? gv_q4 : gh_q4;
            }
            direction[center] = chosen;
            dir_left = chosen;
        }
    }

    // Tang 2: {RAW,G} -> RGB. Tat ca G van o Q12.4.
    std::vector<Rgb12> out(count);
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            const std::size_t center = at(row, col, width);
            const bool re = (row & 1) == 0;
            const bool ce = (col & 1) == 0;
            const int cl = mirror101(col - 1, width);
            const int cr = mirror101(col + 1, width);
            const int rt = mirror101(row - 1, height);
            const int rb = mirror101(row + 1, height);
            const std::int64_t gc = green_q4[center];
            std::int64_t rq4 = 0;
            std::int64_t bq4 = 0;

            if (re && ce) {
                const std::int64_t avg_b =
                    (raw12[at(rt, cl, width)] + raw12[at(rt, cr, width)]
                   + raw12[at(rb, cl, width)] + raw12[at(rb, cr, width)]) >> 2;
                const std::int64_t avg_g =
                    (green_q4[at(rt, cl, width)] + green_q4[at(rt, cr, width)]
                   + green_q4[at(rb, cl, width)] + green_q4[at(rb, cr, width)]) >> 2;
                rq4 = static_cast<std::int64_t>(raw12[center]) << 4;
                bq4 = gc + (avg_b << 4) - avg_g;
            } else if (re && !ce) {
                const std::int64_t ar =
                    (raw12[at(row, cl, width)] + raw12[at(row, cr, width)]) >> 1;
                const std::int64_t agr =
                    (green_q4[at(row, cl, width)] + green_q4[at(row, cr, width)]) >> 1;
                const std::int64_t ab =
                    (raw12[at(rt, col, width)] + raw12[at(rb, col, width)]) >> 1;
                const std::int64_t agb =
                    (green_q4[at(rt, col, width)] + green_q4[at(rb, col, width)]) >> 1;
                rq4 = gc + (ar << 4) - agr;
                bq4 = gc + (ab << 4) - agb;
            } else if (!re && ce) {
                const std::int64_t ar =
                    (raw12[at(rt, col, width)] + raw12[at(rb, col, width)]) >> 1;
                const std::int64_t agr =
                    (green_q4[at(rt, col, width)] + green_q4[at(rb, col, width)]) >> 1;
                const std::int64_t ab =
                    (raw12[at(row, cl, width)] + raw12[at(row, cr, width)]) >> 1;
                const std::int64_t agb =
                    (green_q4[at(row, cl, width)] + green_q4[at(row, cr, width)]) >> 1;
                rq4 = gc + (ar << 4) - agr;
                bq4 = gc + (ab << 4) - agb;
            } else {
                const std::int64_t avg_r =
                    (raw12[at(rt, cl, width)] + raw12[at(rt, cr, width)]
                   + raw12[at(rb, cl, width)] + raw12[at(rb, cr, width)]) >> 2;
                const std::int64_t avg_g =
                    (green_q4[at(rt, cl, width)] + green_q4[at(rt, cr, width)]
                   + green_q4[at(rb, cl, width)] + green_q4[at(rb, cr, width)]) >> 2;
                rq4 = gc + (avg_r << 4) - avg_g;
                bq4 = static_cast<std::int64_t>(raw12[center]) << 4;
            }
            out[center] = {round_sat_q4(rq4), round_sat_q4(gc), round_sat_q4(bq4)};
        }
    }
    return out;
}

} // namespace golden_demosaic
