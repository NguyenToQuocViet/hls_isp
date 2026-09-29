#include "isp_wb_top.hpp"
#include "golden_wb_model.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

using video_in_t  = ap_axiu<16, 1, 0, 0>;
using video_out_t = ap_axiu<16, 1, 0, 0>;

int main() {
    const int W = FRAME_WIDTH;
    const int H = FRAME_HEIGHT;
    const int F = 2;
    const int N = W * H;

    // AXI-Lite gain values used for both DUT and golden models.
    const golden_wb_model::FloatGains float_gains = {
        1.512804, 1.0, 1.0, 1.916974
    };
    const golden_wb_model::FixedGainsQ12 fixed_gains = {
        golden_wb_model::quantize_gain_q4_12(float_gains.r),
        golden_wb_model::quantize_gain_q4_12(float_gains.gr),
        golden_wb_model::quantize_gain_q4_12(float_gains.gb),
        golden_wb_model::quantize_gain_q4_12(float_gains.b)
    };

    const gain_t dut_gain_r  = float_gains.r;
    const gain_t dut_gain_gr = float_gains.gr;
    const gain_t dut_gain_gb = float_gains.gb;
    const gain_t dut_gain_b  = float_gains.b;

    hls::stream<video_in_t> stream_in;
    hls::stream<video_out_t> stream_out;

    std::uint64_t data_mismatches = 0;
    std::uint64_t sideband_mismatches = 0;
    std::uint64_t samples = 0;
    double sae = 0.0, sse = 0.0, maxe = 0.0;

    for (int frame = 0; frame < F; ++frame) {
        std::vector<std::uint16_t> raw(N);
        for (int row = 0; row < H; ++row) {
            for (int col = 0; col < W; ++col) {
                raw[row * W + col] =
                    (frame * 113 + row * 29 + col * 17
                     + ((row * col * 3) ^ (col * 11))) & 0x3ff;
            }
        }

        const std::vector<double> float_ref =
            golden_wb_model::wb_float(raw, W, H, float_gains);
        const std::vector<std::uint16_t> fixed_ref =
            golden_wb_model::wb_fixed(raw, W, H, fixed_gains);

        // Queue exactly one complete frame before starting one ap_ctrl_hs
        // transaction. A gap between two frame transactions is allowed.
        for (int i = 0; i < N; ++i) {
            video_in_t input_pixel;
            input_pixel.data = 0;
            input_pixel.data.range(9, 0) = raw[i];
            input_pixel.keep = -1;
            input_pixel.strb = -1;
            input_pixel.user = (i == 0);
            input_pixel.last = (i % W == W - 1);
            stream_in.write(input_pixel);
        }
    
        // One top-level call processes exactly one frame.
        isp_wb_top(
            stream_in,
            stream_out,
            dut_gain_r,
            dut_gain_gr,
            dut_gain_gb,
            dut_gain_b
        );

        for (int i = 0; i < N; ++i) {
            const video_out_t output_pixel = stream_out.read();
            const unsigned dut = output_pixel.data.range(11, 0).to_uint();

            if (dut != fixed_ref[i]) {
                if (data_mismatches < 10) {
                    std::cerr << "WB mismatch frame=" << frame
                              << " i=" << i << " DUT=" << dut
                              << " FIX=" << fixed_ref[i] << '\n';
                }
                ++data_mismatches;
            }

            const bool expected_user = (i == 0);
            const bool expected_last = (i % W == W - 1);
            const bool sideband_ok =
                (output_pixel.user.to_uint() == expected_user) &&
                (output_pixel.last.to_uint() == expected_last) &&
                (output_pixel.keep == ap_uint<2>(-1)) &&
                (output_pixel.strb == ap_uint<2>(-1));
            if (!sideband_ok) {
                if (sideband_mismatches < 10) {
                    std::cerr << "WB sideband mismatch frame=" << frame
                              << " i=" << i << '\n';
                }
                ++sideband_mismatches;
            }

            const double error =
                static_cast<double>(fixed_ref[i]) - float_ref[i];
            sae += std::fabs(error);
            sse += error * error;
            maxe = std::max(maxe, std::fabs(error));
            ++samples;
        }

        if (!stream_in.empty() || !stream_out.empty()) {
            std::cerr << "[FAIL] WB stream contains leftover data after frame "
                      << frame << '\n';
            return 1;
        }

        std::cout << "WB frame " << frame << " completed\n";
    }

    std::cout << "WB HLS-fixed mismatches="
              << data_mismatches << '/' << samples << '\n'
              << "WB sideband mismatches="
              << sideband_mismatches << '/' << samples << '\n'
              << "WB fixed-float MAE=" << sae / samples
              << " MSE=" << sse / samples
              << " MAX=" << maxe << '\n';

    return (data_mismatches || sideband_mismatches) ? 1 : 0;
}
