#include "golden_demosaic_model.hpp"
#include "golden_demosaic_fixed.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace golden_demosaic_model {
namespace {
int mirror101(int x, int limit) {
    if (x < 0) return 1;
    if (x >= limit) return limit - 2;
    return x;
}
std::size_t at(int r, int c, int w) {
    return static_cast<std::size_t>(r) * w + c;
}
double clip(double x) { return std::max(0.0, std::min(4095.0, x)); }
void check(const std::vector<std::uint16_t> &raw, int w, int h) {
    if (w < 2 || h < 2 || raw.size() != static_cast<std::size_t>(w * h))
        throw std::invalid_argument("Demosaic: invalid frame size");
    for (std::size_t i = 0; i < raw.size(); ++i)
        if (raw[i] > 4095) throw std::invalid_argument("Demosaic: input is not RAW12");
}
}

std::vector<RgbFloat> demosaic_float(
    const std::vector<std::uint16_t> &raw, int w, int h,
    double edge_threshold, double edge_mag) {
    check(raw, w, h);
    std::vector<double> green(raw.size(), 0.0);
    std::vector<std::uint8_t> direction(raw.size(), 0);

    // Stage 1: interpolate G and build the direction map in raster order.
    for (int r = 0; r < h; ++r) {
        std::uint8_t dir_left = 0;
        for (int c = 0; c < w; ++c) {
            const std::size_t center = at(r, c, w);
            if ((r & 1) != (c & 1)) {
                green[center] = raw[center];
                direction[center] = dir_left;
                continue;
            }
            const int cl = mirror101(c - 1, w), cr = mirror101(c + 1, w);
            const int rt = mirror101(r - 1, h), rb = mirror101(r + 1, h);
            const double left = raw[at(r, cl, w)], right = raw[at(r, cr, w)];
            const double top = raw[at(rt, c, w)], bottom = raw[at(rb, c, w)];
            const double gh = (left + right) * 0.5;
            const double gv = (top + bottom) * 0.5;
            const double grad_h = std::fabs(left - right);
            const double grad_v = std::fabs(top - bottom);
            const double diff = std::fabs(grad_h - grad_v);
            const double grad_max = std::max(grad_h, grad_v);
            std::uint8_t chosen;
            if (grad_max < edge_mag) {
                green[center] = (gh + gv) * 0.5;
                chosen = dir_left;
            } else if (diff > edge_threshold) {
                chosen = grad_h > grad_v ? 1 : 0;
                green[center] = chosen ? gv : gh;
            } else {
                int vote = dir_left;
                if (r != 0) {
                    vote += direction[at(r - 1, mirror101(c - 1, w), w)];
                    vote += direction[at(r - 1, c, w)];
                    vote += direction[at(r - 1, mirror101(c + 1, w), w)];
                }
                chosen = vote > 2 ? 1 : 0;
                green[center] = chosen ? gv : gh;
            }
            direction[center] = chosen;
            dir_left = chosen;
        }
    }

    // Stage 2: color-difference interpolation for the missing R/B channels.
    std::vector<RgbFloat> out(raw.size());
    for (int r = 0; r < h; ++r) for (int c = 0; c < w; ++c) {
        const int cl = mirror101(c - 1, w), cr = mirror101(c + 1, w);
        const int rt = mirror101(r - 1, h), rb = mirror101(r + 1, h);
        const std::size_t center = at(r, c, w);
        const double gc = green[center];
        double rr, bb;
        if (!(r & 1) && !(c & 1)) {
            const double ab = (raw[at(rt,cl,w)] + raw[at(rt,cr,w)]
                             + raw[at(rb,cl,w)] + raw[at(rb,cr,w)]) * 0.25;
            const double ag = (green[at(rt,cl,w)] + green[at(rt,cr,w)]
                             + green[at(rb,cl,w)] + green[at(rb,cr,w)]) * 0.25;
            rr = raw[center]; bb = gc + ab - ag;
        } else if (!(r & 1) && (c & 1)) {
            const double ar = (raw[at(r,cl,w)] + raw[at(r,cr,w)]) * 0.5;
            const double agr = (green[at(r,cl,w)] + green[at(r,cr,w)]) * 0.5;
            const double ab = (raw[at(rt,c,w)] + raw[at(rb,c,w)]) * 0.5;
            const double agb = (green[at(rt,c,w)] + green[at(rb,c,w)]) * 0.5;
            rr = gc + ar - agr; bb = gc + ab - agb;
        } else if ((r & 1) && !(c & 1)) {
            const double ar = (raw[at(rt,c,w)] + raw[at(rb,c,w)]) * 0.5;
            const double agr = (green[at(rt,c,w)] + green[at(rb,c,w)]) * 0.5;
            const double ab = (raw[at(r,cl,w)] + raw[at(r,cr,w)]) * 0.5;
            const double agb = (green[at(r,cl,w)] + green[at(r,cr,w)]) * 0.5;
            rr = gc + ar - agr; bb = gc + ab - agb;
        } else {
            const double ar = (raw[at(rt,cl,w)] + raw[at(rt,cr,w)]
                             + raw[at(rb,cl,w)] + raw[at(rb,cr,w)]) * 0.25;
            const double ag = (green[at(rt,cl,w)] + green[at(rt,cr,w)]
                             + green[at(rb,cl,w)] + green[at(rb,cr,w)]) * 0.25;
            rr = gc + ar - ag; bb = raw[center];
        }
        out[center] = {clip(rr), clip(gc), clip(bb)};
    }
    return out;
}

std::vector<Rgb12> demosaic_fixed(
    const std::vector<std::uint16_t> &raw, int w, int h,
    int edge_threshold, int edge_mag) {
    check(raw, w, h);
    // Reuse the standalone integer implementation; it contains no HLS types
    // and models the Q12.4 shifts/rounding independently from the DUT.
    const std::vector<golden_demosaic::Rgb12> fixed =
        golden_demosaic::frame(raw, w, h, edge_threshold, edge_mag);
    std::vector<Rgb12> out(fixed.size());
    for (std::size_t i = 0; i < fixed.size(); ++i)
        out[i] = {fixed[i].r, fixed[i].g, fixed[i].b};
    return out;
}

} // namespace golden_demosaic_model