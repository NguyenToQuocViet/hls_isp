#include "isp_demosaic_top.hpp"
#include "golden_demosaic_model.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

using video_in_t = ap_axiu<16, 1, 0, 0>;
using video_out_t = ap_axiu<40, 1, 0, 0>;

static std::uint16_t make_raw_pixel(int frame, int row, int col) {
    // Hai frame khac nhau, phu het mien RAW12 va tao nhieu bien canh de
    // kiem tra direction, saturation va cac phep cong gan full scale.
    const std::uint32_t base =
        static_cast<std::uint32_t>(
            frame * 977 + row * 97 + col * 53
            + ((row * col * 11) ^ (col * 29)));
    const std::uint32_t edge =
        (frame == 1 && (((row / 8) ^ (col / 8)) & 1)) ? 3071u : 0u;
    return static_cast<std::uint16_t>((base + edge) & 0x0fffu);
}

int main() {
    const int W = FRAME_WIDTH;
    const int H = FRAME_HEIGHT;
    const int F = 2;
    const int N = W * H;
    const int T = THRESHOLD_T;
    const int M = EDGE_MAG_T;

    std::uint64_t data_mismatches = 0;
    std::uint64_t sideband_mismatches = 0;
    std::uint64_t samples = 0;
    double sae = 0.0;
    double sse = 0.0;
    double maxe = 0.0;

    hls::stream<video_in_t> stream_in;
    hls::stream<video_out_t> stream_out;

    for (int frame = 0; frame < F; ++frame) {
        std::vector<std::uint16_t> raw(N);
        for (int row = 0; row < H; ++row) {
            for (int col = 0; col < W; ++col) {
                const int index = row * W + col;
                raw[index] = make_raw_pixel(frame, row, col);

                video_in_t input_pixel;
                input_pixel.data = 0;
                input_pixel.data.range(11, 0) = raw[index];
                input_pixel.keep = -1;
                input_pixel.strb = -1;
                input_pixel.user = (index == 0) ? 1 : 0;
                input_pixel.last = (col == W - 1) ? 1 : 0;
                stream_in.write(input_pixel);
            }
        }

        const std::vector<golden_demosaic_model::RgbFloat> float_ref =
            golden_demosaic_model::demosaic_float(raw, W, H, T, M);
        const std::vector<golden_demosaic_model::Rgb12> fixed_ref =
            golden_demosaic_model::demosaic_fixed(raw, W, H, T, M);

        // Mot transaction ap_ctrl_hs xu ly dung mot frame. Pha flush cua hai
        // cua so 3x3 nam ben trong core, nen testbench khong gui pixel padding.
        isp_demosaic_top(stream_in, stream_out);

        if (!stream_in.empty()) {
            std::cerr << "[FAIL] Frame " << frame
                      << ": DUT did not consume all input pixels\n";
            return 1;
        }

        for (int index = 0; index < N; ++index) {
            if (stream_out.empty()) {
                std::cerr << "[FAIL] Frame " << frame
                          << ": output stopped at " << index
                          << ", expected " << N << " pixels\n";
                return 1;
            }

            const video_out_t output_pixel = stream_out.read();
            const unsigned dut[3] = {
                output_pixel.data.range(11, 0).to_uint(),
                output_pixel.data.range(23, 12).to_uint(),
                output_pixel.data.range(35, 24).to_uint()
            };
            const unsigned ref[3] = {
                fixed_ref[index].r,
                fixed_ref[index].g,
                fixed_ref[index].b
            };
             const double float_value[3] = {
                float_ref[index].r,
                float_ref[index].g,
                float_ref[index].b
            };

            for (int channel = 0; channel < 3; ++channel) {
                if (dut[channel] != ref[channel]) {
                    if (data_mismatches < 12) {
                        std::cerr << "Demosaic mismatch frame=" << frame
                                  << " index=" << index
                                  << " row=" << index / W
                                  << " col=" << index % W
                                  << " ch=" << channel
                                  << " DUT=" << dut[channel]
                                  << " FIX=" << ref[channel] << '\n';
                    }
                    ++data_mismatches;
                }

                const double error =
                    static_cast<double>(ref[channel]) - float_value[channel];
                sae += std::fabs(error);
                sse += error * error;
                maxe = std::max(maxe, std::fabs(error));
                ++samples;
            }

            const bool expected_user = (index == 0);
            const bool expected_last = (index % W == W - 1);
            const bool sideband_ok =
                (output_pixel.user.to_uint() == expected_user) &&
                (output_pixel.last.to_uint() == expected_last) &&
                (output_pixel.keep == ap_uint<5>(-1)) &&
                (output_pixel.strb == ap_uint<5>(-1));

            if (!sideband_ok) {
                if (sideband_mismatches < 12) {
                    std::cerr << "Sideband mismatch frame=" << frame
                              << " index=" << index
                              << " user=" << output_pixel.user.to_uint()
                              << " last=" << output_pixel.last.to_uint()
                              << '\n';
                }
                ++sideband_mismatches;
            }
        }

        if (!stream_out.empty()) {
            std::cerr << "[FAIL] Frame " << frame
                      << ": DUT produced more than " << N << " pixels\n";
            return 1;
        }
        std::cout << "Demosaic frame " << frame
                  << " completed\n";
    }

    std::cout << "Demosaic HLS-fixed mismatches="
              << data_mismatches << '/' << samples << '\n'
              << "Demosaic sideband mismatches="
              << sideband_mismatches << '/' << (F * N) << '\n'
              << "Demosaic fixed-float MAE=" << sae / samples
              << " MSE=" << sse / samples
              << " MAX=" << maxe << '\n';

    return (data_mismatches || sideband_mismatches) ? 1 : 0;
}