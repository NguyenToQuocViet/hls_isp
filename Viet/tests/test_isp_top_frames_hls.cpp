/*
Project: Adaptive Directional BPC and BLC
Module: Multi-Frame ISP HLS Test
Description: Compare two consecutive HLS frames with independent BLC and BPC reference results.
Author: Viet Nguyen To Quoc
*/

#include "isp_top.hpp"
#include "blc.hpp"
#include "bpc_adaptive.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    constexpr int FRAME_COUNT = 2;
    const std::size_t pixel_count = std::size_t(FRAME_WIDTH) * FRAME_HEIGHT;
    const blc::BlcConfig blc_config{20, 30, 40, 50};
    const adaptive_bpc::BpcConfig bpc_config{4, 8, 4, 3, 0};

    hls::stream<ap_axiu<16, 1, 0, 0>> input;
    hls::stream<ap_axiu<16, 1, 0, 0>> output;
    std::vector<std::uint16_t> expected;
    expected.reserve(FRAME_COUNT * pixel_count);

    for (int frame = 0; frame < FRAME_COUNT; frame++) {
        std::vector<std::uint16_t> raw(pixel_count);

        for (std::size_t i = 0; i < pixel_count; i++) {
            const int row = int(i / FRAME_WIDTH);
            const int col = int(i % FRAME_WIDTH);

            raw[i] = frame == 0
                ? std::uint16_t(100 + ((row * 7 + col * 11) & 31))
                : std::uint16_t(700 - ((row * 3 + col * 5) & 63));
        }

        raw[std::size_t(100) * FRAME_WIDTH + 100] = frame == 0 ? 1023 : 0;
        raw[std::size_t(FRAME_HEIGHT - 3) * FRAME_WIDTH + FRAME_WIDTH - 3] =
            frame == 0 ? 0 : 1023;

        std::vector<std::uint16_t> post_blc;
        std::vector<std::uint16_t> frame_expected;
        std::vector<adaptive_bpc::Detection> detections;

        blc::blc_frame(raw, post_blc, FRAME_WIDTH, FRAME_HEIGHT, blc_config);
        adaptive_bpc::bpc_frame(
            post_blc, frame_expected, detections, bpc_config
        );
        expected.insert(expected.end(), frame_expected.begin(), frame_expected.end());

        for (std::size_t i = 0; i < pixel_count; i++) {
            ap_axiu<16, 1, 0, 0> packet{};
            packet.data = raw[i];
            packet.keep = 3;
            packet.strb = 3;
            packet.user = i == 0;
            packet.last = i % FRAME_WIDTH == FRAME_WIDTH - 1;
            input.write(packet);
        }
    }

    isp_top_frames(
        input, output,
        ap_ufixed<10, 10>(blc_config.black_level_r),
        ap_ufixed<10, 10>(blc_config.black_level_gr),
        ap_ufixed<10, 10>(blc_config.black_level_gb),
        ap_ufixed<10, 10>(blc_config.black_level_b),
        ap_ufixed<10, 10>(bpc_config.base_threshold_r),
        ap_ufixed<10, 10>(bpc_config.base_threshold_g),
        ap_ufixed<10, 10>(bpc_config.base_threshold_b),
        ap_uint<4>(bpc_config.signal_shift),
        ap_uint<4>(bpc_config.activity_shift),
        FRAME_COUNT
    );

    int failures = 0;

    for (std::size_t i = 0; i < expected.size(); i++) {
        if (output.empty()) {
            std::cout << "Missing output at pixel " << i << "\n";
            return 1;
        }

        const auto packet = output.read();
        const std::size_t position = i % pixel_count;
        const bool match =
            packet.data.to_uint() == expected[i] &&
            packet.keep == 3 &&
            packet.strb == 3 &&
            packet.user == (position == 0) &&
            packet.last == (position % FRAME_WIDTH == FRAME_WIDTH - 1);

        if (!match) {
            failures++;

            if (failures <= 10) {
                std::cout << "Pixel " << i
                          << ": expected=" << expected[i]
                          << " got=" << packet.data.to_uint()
                          << " user=" << packet.user
                          << " last=" << packet.last << "\n";
            }
        }
    }

    if (!input.empty() || !output.empty()) {
        failures++;
        std::cout << "Unexpected input/output count\n";
    }

    std::cout << "Multi-frame HLS: " << expected.size()
              << " pixels checked, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
