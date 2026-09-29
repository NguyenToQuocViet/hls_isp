#include "isp_demosaic_top.hpp"
#include "golden_demosaic_model.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

using video_in_t  = ap_axiu<16, 1, 0, 0>;
using video_out_t = ap_axiu<40, 1, 0, 0>;

static std::uint16_t make_raw_pixel(int frame, int row, int col) {
    return static_cast<std::uint16_t>(
        (frame * 509 + row * 97 + col * 53
         + ((row * col * 11) ^ (col * 29))) & 0xfff);
}

int main() {
    // The current DUT is fixed at 1920x1080.
    const int W = FRAME_WIDTH;
    const int H = FRAME_HEIGHT;
    const int F = 2;
    const int N = W * H;
    const int DELAY = 2 * (W + 1);
    const int T = THRESHOLD_T;
    const int M = EDGE_MAG_T;

    std::vector<golden_demosaic_model::RgbFloat> float_ref_all;
    std::vector<golden_demosaic_model::Rgb12> fixed_ref_all;
    float_ref_all.reserve(static_cast<std::size_t>(F) * N);
    fixed_ref_all.reserve(static_cast<std::size_t>(F) * N);

    for (int frame = 0; frame < F; ++frame) {
        std::vector<std::uint16_t> raw(N);
        for (int row = 0; row < H; ++row) {
            for (int col = 0; col < W; ++col) {
                raw[row * W + col] = make_raw_pixel(frame, row, col);
            }
        }

        const std::vector<golden_demosaic_model::RgbFloat> float_ref =
            golden_demosaic_model::demosaic_float(raw, W, H, T, M);
        const std::vector<golden_demosaic_model::Rgb12> fixed_ref =
            golden_demosaic_model::demosaic_fixed(raw, W, H, T, M);

        float_ref_all.insert(
            float_ref_all.end(), float_ref.begin(), float_ref.end());
        fixed_ref_all.insert(
            fixed_ref_all.end(), fixed_ref.begin(), fixed_ref.end());
    }

    hls::stream<video_in_t> stream_in;
    hls::stream<video_out_t> stream_out;

    // hls::task uses blocking stream reads in C simulation.  Queue all real
    // pixels plus the exact two-window flush before starting the task network,
    // then read exactly F*N valid RGB packets.  This avoids guessing an extra
    // task/pipeline lead and leaves no padding RGB packet unread.
    const int total_inputs = F * N + DELAY;
    for (int input_index = 0; input_index < total_inputs; ++input_index) {
        int frame;
        int frame_index;
        if (input_index < F * N) {
            frame = input_index / N;
            frame_index = input_index % N;
        } else {
            frame = F;
            frame_index = input_index - F * N;
        }

        const int row = frame_index / W;
        const int col = frame_index % W;

        video_in_t input_pixel;
        input_pixel.data = 0;
        input_pixel.data.range(11, 0) = make_raw_pixel(frame, row, col);
        input_pixel.keep = -1;
        input_pixel.strb = -1;
        input_pixel.user = (frame_index == 0);
        input_pixel.last = (col == W - 1);
        stream_in.write(input_pixel);
    }

    // A pure hls::task top is instantiated exactly once.
    isp_demosaic_top(stream_in, stream_out);

    std::uint64_t data_mismatches = 0;
    std::uint64_t sideband_mismatches = 0;
    std::uint64_t samples = 0;
    double sae = 0.0, sse = 0.0, maxe = 0.0;
    int output_index = 0;

    for (; output_index < F * N; ++output_index) {
        const video_out_t output_pixel = stream_out.read();
        const unsigned dut[3] = {
            output_pixel.data.range(11, 0).to_uint(),
            output_pixel.data.range(23, 12).to_uint(),
            output_pixel.data.range(35, 24).to_uint()
        };
        const unsigned ref[3] = {
            fixed_ref_all[output_index].r,
            fixed_ref_all[output_index].g,
            fixed_ref_all[output_index].b
        };
        const double float_values[3] = {
            float_ref_all[output_index].r,
            float_ref_all[output_index].g,
            float_ref_all[output_index].b
        };

        for (int channel = 0; channel < 3; ++channel) {
            if (dut[channel] != ref[channel]) {
                if (data_mismatches < 10) {
                    std::cerr << "Demosaic mismatch output="
                              << output_index << " ch=" << channel
                              << " DUT=" << dut[channel]
                              << " FIX=" << ref[channel] << '\n';
                }
                ++data_mismatches;
            }
            const double error =
                static_cast<double>(ref[channel]) - float_values[channel];
            sae += std::fabs(error);
            sse += error * error;
            maxe = std::max(maxe, std::fabs(error));
            ++samples;
        }

        const bool expected_user = (output_index % N == 0);
        const bool expected_last = (output_index % W == W - 1);
        const bool sideband_ok =
            (output_pixel.user.to_uint() == expected_user) &&
            (output_pixel.last.to_uint() == expected_last) &&
            (output_pixel.keep == ap_uint<5>(-1)) &&
            (output_pixel.strb == ap_uint<5>(-1));
        if (!sideband_ok) {
            if (sideband_mismatches < 10) {
                std::cerr << "Demosaic sideband mismatch output="
                          << output_index << '\n';
            }
            ++sideband_mismatches;
        }
    }

    if (output_index != F * N) {
        std::cerr << "[FAIL] Demosaic output count=" << output_index
                  << " expected=" << F * N << '\n';
        return 1;
    }

    std::cout << "Demosaic HLS-fixed mismatches="
              << data_mismatches << '/' << samples << '\n'
              << "Demosaic sideband mismatches="
              << sideband_mismatches << '/' << output_index << '\n'
              << "Demosaic fixed-float MAE=" << sae / samples
              << " MSE=" << sse / samples
              << " MAX=" << maxe << '\n';
}