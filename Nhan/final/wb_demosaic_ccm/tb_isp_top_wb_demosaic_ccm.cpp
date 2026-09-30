#include "isp_top_wb_demosaic_ccm.hpp"
#include "golden_wb_model.hpp"
#include "golden_demosaic_model.hpp"
#include "golden_ccm_model.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

using video_in_t = ap_axiu<16, 1, 0, 0>;
using video_out_t = ap_axiu<40, 1, 0, 0>;

namespace {

const int FRAME_COUNT = 2;
const int PIXELS_PER_FRAME = FRAME_WIDTH * FRAME_HEIGHT;
const int MAX_PRINTED_ERRORS = 12;

struct FrameConfig {
    golden_wb_model::FloatGains wb;
    golden_ccm_model::FloatMatrix ccm;
    const char *name;
};

std::uint16_t make_raw(int frame, int row, int col) {
    const unsigned value =
        static_cast<unsigned>(frame * 113)
      + static_cast<unsigned>(row * 29)
      + static_cast<unsigned>(col * 17)
      + static_cast<unsigned>((row * col * 3) ^ (col * 11));
    return static_cast<std::uint16_t>(value & 0x03ffu);
}

FrameConfig make_config(int frame) {
    FrameConfig cfg = {};

    if (frame == 0) {
        cfg.wb = {1.0, 1.0, 1.0, 1.0};
        cfg.ccm.a[0][0] = 1.0;
        cfg.ccm.a[1][1] = 1.0;
        cfg.ccm.a[2][2] = 1.0;
        cfg.name = "identity";
    } else {
        cfg.wb = {1.512804, 1.0, 1.0, 1.916974};
        const double typical[3][3] = {
            { 1.6, -0.3, -0.3},
            {-0.2,  1.4, -0.2},
            {-0.1, -0.6,  1.7}
        };
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 3; ++col) {
                cfg.ccm.a[row][col] = typical[row][col];
            }
        }
        cfg.name = "typical";
    }
    return cfg;
}

golden_wb_model::FixedGainsQ12 fixed_wb_config(
    const golden_wb_model::FloatGains &g
) {
    return {
        golden_wb_model::quantize_gain_q4_12(g.r),
        golden_wb_model::quantize_gain_q4_12(g.gr),
        golden_wb_model::quantize_gain_q4_12(g.gb),
        golden_wb_model::quantize_gain_q4_12(g.b)
    };
}

golden_ccm_model::FixedMatrixQ12 fixed_ccm_config(
    const golden_ccm_model::FloatMatrix &m
) {
    golden_ccm_model::FixedMatrixQ12 fixed = {};
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            fixed.a[row][col] =
                golden_ccm_model::quantize_coeff_q4_12(m.a[row][col]);
        }
    }
    return fixed;
}

std::uint16_t round_sat_12(double value) {
    if (value <= 0.0) return 0;
    if (value >= 4095.0) return 4095;
    return static_cast<std::uint16_t>(std::floor(value + 0.5));
}

video_in_t make_input_pixel(std::uint16_t raw, int index) {
    video_in_t pixel;
    pixel.data = 0;
    pixel.data.range(9, 0) = raw;
    pixel.keep = -1;
    pixel.strb = -1;
    pixel.user = (index == 0) ? 1 : 0;
    pixel.last = (index % FRAME_WIDTH == FRAME_WIDTH - 1) ? 1 : 0;
    return pixel;
}

} // namespace

int main() {
    hls::stream<video_in_t> stream_in;
    hls::stream<video_out_t> stream_out;

    std::uint64_t data_mismatches = 0;
    std::uint64_t sideband_mismatches = 0;
    std::uint64_t channel_samples = 0;
    double absolute_error_sum = 0.0;
    double squared_error_sum = 0.0;
    double maximum_error = 0.0;

    for (int frame = 0; frame < FRAME_COUNT; ++frame) {
        const FrameConfig cfg = make_config(frame);
        const golden_wb_model::FixedGainsQ12 wb_fixed_cfg =
            fixed_wb_config(cfg.wb);
        const golden_ccm_model::FixedMatrixQ12 ccm_fixed_cfg =
            fixed_ccm_config(cfg.ccm);

        std::vector<std::uint16_t> raw(PIXELS_PER_FRAME);
        for (int row = 0; row < FRAME_HEIGHT; ++row) {
            for (int col = 0; col < FRAME_WIDTH; ++col) {
                const int index = row * FRAME_WIDTH + col;
                raw[index] = make_raw(frame, row, col);
                stream_in.write(make_input_pixel(raw[index], index));
            }
        }

        // Fixed-point, bit-exact reference for the complete pipeline.
        const std::vector<std::uint16_t> wb_fixed =
            golden_wb_model::wb_fixed(
                raw, FRAME_WIDTH, FRAME_HEIGHT, wb_fixed_cfg);
        const std::vector<golden_demosaic_model::Rgb12> demosaic_fixed =
            golden_demosaic_model::demosaic_fixed(
                wb_fixed,
                FRAME_WIDTH,
                FRAME_HEIGHT,
                THRESHOLD_T,
                EDGE_MAG_T);

        std::vector<golden_ccm_model::Rgb12> ccm_fixed_input(
            PIXELS_PER_FRAME);
        for (int i = 0; i < PIXELS_PER_FRAME; ++i) {
            ccm_fixed_input[i] = {
                demosaic_fixed[i].r,
                demosaic_fixed[i].g,
                demosaic_fixed[i].b
            };
        }
        const std::vector<golden_ccm_model::Rgb12> fixed_reference =
            golden_ccm_model::ccm_fixed(ccm_fixed_input, ccm_fixed_cfg);

        // Floating-point reference. The WB result is rounded at the RAW12
        // block boundary because the demosaic input port is an integer port.
        const std::vector<double> wb_float =
            golden_wb_model::wb_float(
                raw, FRAME_WIDTH, FRAME_HEIGHT, cfg.wb);
        std::vector<std::uint16_t> wb_float_at_interface(
            PIXELS_PER_FRAME);
        for (int i = 0; i < PIXELS_PER_FRAME; ++i) {
            wb_float_at_interface[i] = round_sat_12(wb_float[i]);
        }
        const std::vector<golden_demosaic_model::RgbFloat> demosaic_float =
            golden_demosaic_model::demosaic_float(
                wb_float_at_interface,
                FRAME_WIDTH,
                FRAME_HEIGHT,
                static_cast<double>(THRESHOLD_T),
                static_cast<double>(EDGE_MAG_T));

        std::vector<golden_ccm_model::RgbFloat> ccm_float_input(
            PIXELS_PER_FRAME);
        for (int i = 0; i < PIXELS_PER_FRAME; ++i) {
            ccm_float_input[i] = {
                demosaic_float[i].r,
                demosaic_float[i].g,
                demosaic_float[i].b
            };
        }
        const std::vector<golden_ccm_model::RgbFloat> float_reference =
            golden_ccm_model::ccm_float(ccm_float_input, cfg.ccm);

        gain_matrix dut_ccm[3][3];
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 3; ++col) {
                dut_ccm[row][col] = cfg.ccm.a[row][col];
            }
        }

        // One call/ap_start handles one complete frame. Calling it twice
        // verifies that demosaic state is reset correctly for frame 1.
        isp_top_wb_demosaic_ccm(
            stream_in,
            stream_out,
            static_cast<gain_t>(cfg.wb.r),
            static_cast<gain_t>(cfg.wb.gr),
            static_cast<gain_t>(cfg.wb.gb),
            static_cast<gain_t>(cfg.wb.b),
            dut_ccm[0][0], dut_ccm[0][1], dut_ccm[0][2],
            dut_ccm[1][0], dut_ccm[1][1], dut_ccm[1][2],
            dut_ccm[2][0], dut_ccm[2][1], dut_ccm[2][2]);

        for (int i = 0; i < PIXELS_PER_FRAME; ++i) {
            if (stream_out.empty()) {
                std::cerr << "[FAIL] Missing output frame=" << frame
                          << " index=" << i << '\n';
                return 1;
            }

            const video_out_t output_pixel = stream_out.read();
            const unsigned dut[3] = {
                static_cast<unsigned>(output_pixel.data.range(11, 0)),
                static_cast<unsigned>(output_pixel.data.range(23, 12)),
                static_cast<unsigned>(output_pixel.data.range(35, 24))
            };
            const unsigned fixed[3] = {
                fixed_reference[i].r,
                fixed_reference[i].g,
                fixed_reference[i].b
            };
            const double floating[3] = {
                float_reference[i].r,
                float_reference[i].g,
                float_reference[i].b
            };

            for (int channel = 0; channel < 3; ++channel) {
                if (dut[channel] != fixed[channel]) {
                    if (data_mismatches < MAX_PRINTED_ERRORS) {
                        std::cerr << "[FAIL-DATA] frame=" << frame
                                  << " index=" << i
                                  << " channel=" << channel
                                  << " DUT=" << dut[channel]
                                  << " FIX=" << fixed[channel] << '\n';
                    }
                    ++data_mismatches;
                }

                const double error =
                    static_cast<double>(fixed[channel]) - floating[channel];
                absolute_error_sum += std::fabs(error);
                squared_error_sum += error * error;
                maximum_error = std::max(maximum_error, std::fabs(error));
                ++channel_samples;
            }

            const unsigned expected_user = (i == 0) ? 1u : 0u;
            const unsigned expected_last =
                (i % FRAME_WIDTH == FRAME_WIDTH - 1) ? 1u : 0u;
            const bool sideband_ok =
                output_pixel.user.to_uint() == expected_user &&
                output_pixel.last.to_uint() == expected_last &&
                output_pixel.keep == ap_uint<5>(-1) &&
                output_pixel.strb == ap_uint<5>(-1) &&
                output_pixel.data.range(39, 36) == ap_uint<4>(0);

            if (!sideband_ok) {
                if (sideband_mismatches < MAX_PRINTED_ERRORS) {
                    std::cerr << "[FAIL-SIDEBAND] frame=" << frame
                              << " index=" << i
                              << " user=" << output_pixel.user.to_uint()
                              << " last=" << output_pixel.last.to_uint()
                              << " keep=0x" << std::hex
                              << output_pixel.keep.to_uint()
                              << " strb=0x" << output_pixel.strb.to_uint()
                              << std::dec << '\n';
                }
                ++sideband_mismatches;
            }
        }

        if (!stream_in.empty() || !stream_out.empty()) {
            std::cerr << "[FAIL] Stream contains leftover data after frame "
                      << frame << '\n';
            return 1;
        }

        std::cout << "ISP top frame " << frame
                  << " (" << cfg.name << ") completed\n";
    }

    const std::uint64_t output_pixels =
        static_cast<std::uint64_t>(FRAME_COUNT) * PIXELS_PER_FRAME;
    std::cout << "ISP top HLS-fixed mismatches="
              << data_mismatches << '/' << channel_samples << '\n'
              << "ISP top sideband mismatches="
              << sideband_mismatches << '/' << output_pixels << '\n'
              << "ISP top fixed-float MAE="
              << absolute_error_sum / channel_samples
              << " MSE=" << squared_error_sum / channel_samples
              << " MAX=" << maximum_error << '\n';

    return (data_mismatches || sideband_mismatches) ? 1 : 0;
}
