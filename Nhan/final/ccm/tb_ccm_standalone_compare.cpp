#include "isp_ccm_top.hpp"
#include "golden_ccm_model.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

using video_in_t  = ap_axiu<40, 1, 0, 0>;
using video_out_t = ap_axiu<40, 1, 0, 0>;

int main() {
    const int W = FRAME_WIDTH;
    const int H = FRAME_HEIGHT;
    const int F = 2;
    const int N = W * H;

    // Frame 0 verifies exact pass-through. Frame 1 exercises positive and
    // negative Q4.12 coefficients, rounding, clipping and saturation.
    const double matrices[F][3][3] = {
        {
            {1.0, 0.0, 0.0},
            {0.0, 1.0, 0.0},
            {0.0, 0.0, 1.0}
        },
        {
            { 1.6, -0.3, -0.3},
            {-0.2,  1.4, -0.2},
            {-0.1, -0.6,  1.7}
        }
    };

    hls::stream<video_in_t> stream_in;
    hls::stream<video_out_t> stream_out;

    std::uint64_t data_mismatches = 0;
    std::uint64_t sideband_mismatches = 0;
    std::uint64_t samples = 0;
    double sae = 0.0;
    double sse = 0.0;
    double maxe = 0.0;

    for (int frame = 0; frame < F; ++frame) {
        golden_ccm_model::FloatMatrix float_matrix = {};
        golden_ccm_model::FixedMatrixQ12 fixed_matrix = {};
        gain_matrix dut_matrix[3][3];

        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 3; ++col) {
                const double value = matrices[frame][row][col];
                float_matrix.a[row][col] = value;
                fixed_matrix.a[row][col] =
                    golden_ccm_model::quantize_coeff_q4_12(value);
                dut_matrix[row][col] = value;
            }
        }

        std::vector<golden_ccm_model::Rgb12> input_rgb(N);
        std::vector<golden_ccm_model::RgbFloat> input_float(N);

        for (int row = 0; row < H; ++row) {
            for (int col = 0; col < W; ++col) {
                const int i = row * W + col;
                input_rgb[i] = {
                    static_cast<std::uint16_t>(
                        (frame * 211 + row * 41 + col * 17) & 0xfff),
                    static_cast<std::uint16_t>(
                        (frame * 73 + row * 19 + col * 37 + 511) & 0xfff),
                    static_cast<std::uint16_t>(
                        (frame * 151 + row * 31 + col * 13 + 997) & 0xfff)
                };
                input_float[i] = {
                    static_cast<double>(input_rgb[i].r),
                    static_cast<double>(input_rgb[i].g),
                    static_cast<double>(input_rgb[i].b)
                };
            }
        }

        const std::vector<golden_ccm_model::RgbFloat> float_ref =
            golden_ccm_model::ccm_float(input_float, float_matrix);
        const std::vector<golden_ccm_model::Rgb12> fixed_ref =
            golden_ccm_model::ccm_fixed(input_rgb, fixed_matrix);

        // Queue one complete frame before starting one ap_ctrl_hs
        // transaction. A gap between the two frame transactions is allowed.
        for (int i = 0; i < N; ++i) {
            video_in_t input_pixel;
            input_pixel.data = 0;
            input_pixel.data.range(11, 0) = input_rgb[i].r;
            input_pixel.data.range(23, 12) = input_rgb[i].g;
            input_pixel.data.range(35, 24) = input_rgb[i].b;
            input_pixel.keep = -1;
            input_pixel.strb = -1;
            input_pixel.user = (i == 0);
            input_pixel.last = (i % W == W - 1);
            stream_in.write(input_pixel);
        }

        // One top-level call processes exactly one complete frame.
        isp_ccm_top(
            stream_in,
            stream_out,
            dut_matrix[0][0], dut_matrix[0][1], dut_matrix[0][2],
            dut_matrix[1][0], dut_matrix[1][1], dut_matrix[1][2],
            dut_matrix[2][0], dut_matrix[2][1], dut_matrix[2][2]
        );

        for (int i = 0; i < N; ++i) {
            const video_out_t output_pixel = stream_out.read();
            const unsigned dut[3] = {
                output_pixel.data.range(11, 0).to_uint(),
                output_pixel.data.range(23, 12).to_uint(),
                output_pixel.data.range(35, 24).to_uint()
            };
            const unsigned ref[3] = {
                fixed_ref[i].r,
                fixed_ref[i].g,
                fixed_ref[i].b
            };
            const double float_values[3] = {
                float_ref[i].r,
                float_ref[i].g,
                float_ref[i].b
            };

            for (int channel = 0; channel < 3; ++channel) {
                if (dut[channel] != ref[channel]) {
                    if (data_mismatches < 10) {
                        std::cerr << "CCM mismatch frame=" << frame
                                  << " i=" << i
                                  << " ch=" << channel
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

            const bool expected_user = (i == 0);
            const bool expected_last = (i % W == W - 1);
            const bool sideband_ok =
                (output_pixel.user.to_uint() == expected_user) &&
                (output_pixel.last.to_uint() == expected_last) &&
                (output_pixel.keep == ap_uint<5>(-1)) &&
                (output_pixel.strb == ap_uint<5>(-1));

            if (!sideband_ok) {
                if (sideband_mismatches < 10) {
                    std::cerr << "CCM sideband mismatch frame=" << frame
                              << " i=" << i << '\n';
                }
                ++sideband_mismatches;
            }
        }

        if (!stream_in.empty() || !stream_out.empty()) {
            std::cerr << "[FAIL] CCM stream contains leftover data after frame "
                      << frame << '\n';
            return 1;
        }

        std::cout << "CCM frame " << frame
                  << (frame == 0 ? " (identity)" : " (typical)")
                  << " completed\n";
    }

    std::cout << "CCM HLS-fixed mismatches="
              << data_mismatches << '/' << samples << '\n'
              << "CCM sideband mismatches="
              << sideband_mismatches << '/' << (F * N) << '\n'
              << "CCM fixed-float MAE=" << sae / samples
              << " MSE=" << sse / samples
              << " MAX=" << maxe << '\n';

    return (data_mismatches || sideband_mismatches) ? 1 : 0;
}
