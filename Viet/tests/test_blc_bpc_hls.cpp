/*
Project: Adaptive Directional BPC and BLC
Module: BLC-to-BPC HLS Integration Test
Description: Check the combined HLS top against the BLC and BPC reference chain.
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
    const std::size_t pixel_count =
        std::size_t(FRAME_WIDTH) * FRAME_HEIGHT;
    const std::size_t hot_index =
        std::size_t(100) * FRAME_WIDTH + 100;
    const std::size_t dead_index =
        std::size_t(101) * FRAME_WIDTH + 100;

    const blc::BlcConfig blc_config{20, 30, 40, 50};
    const adaptive_bpc::BpcConfig bpc_config{4, 8, 4, 3, 0};

    std::vector<std::uint16_t> raw(pixel_count, 100);
    raw[0] = 1023;
    raw[hot_index] = 1023;
    raw[dead_index] = 0;

    std::vector<std::uint16_t> post_blc;
    std::vector<std::uint16_t> expected;
    std::vector<adaptive_bpc::Detection> detections;

    blc::blc_frame(
        raw,
        post_blc,
        FRAME_WIDTH,
        FRAME_HEIGHT,
        blc_config
    );
    adaptive_bpc::bpc_frame(
        post_blc,
        expected,
        detections,
        bpc_config
    );

    if (
        post_blc[0] != 1003 ||
        post_blc[1] != 70 ||
        post_blc[FRAME_WIDTH] != 60 ||
        post_blc[FRAME_WIDTH + 1] != 50 ||
        expected[0] != 1003 ||
        expected[hot_index] != 80 ||
        expected[dead_index] != 60
    ) {
        std::cout << "Reference setup failed\n";
        return 1;
    }

    hls::stream<ap_axiu<16, 1, 0, 0>> input;
    hls::stream<ap_axiu<16, 1, 0, 0>> output;

    for (std::size_t i = 0; i < pixel_count; i++) {
        ap_axiu<16, 1, 0, 0> packet{};
        packet.data = raw[i];
        packet.keep = 3;
        packet.strb = 3;
        packet.user = i == 0;
        packet.last = i % FRAME_WIDTH == FRAME_WIDTH - 1;
        input.write(packet);
    }

    isp_top(
        input,
        output,
        ap_ufixed<10, 10>(blc_config.black_level_r),
        ap_ufixed<10, 10>(blc_config.black_level_gr),
        ap_ufixed<10, 10>(blc_config.black_level_gb),
        ap_ufixed<10, 10>(blc_config.black_level_b),
        ap_ufixed<10, 10>(bpc_config.base_threshold_r),
        ap_ufixed<10, 10>(bpc_config.base_threshold_g),
        ap_ufixed<10, 10>(bpc_config.base_threshold_b),
        ap_uint<4>(bpc_config.signal_shift),
        ap_uint<4>(bpc_config.activity_shift)
    );

    int failures = 0;

    for (std::size_t i = 0; i < pixel_count; i++) {
        if (output.empty()) {
            std::cout << "Missing output at pixel " << i << "\n";
            return 1;
        }

        const ap_axiu<16, 1, 0, 0> packet = output.read();
        const bool match =
            packet.data.to_uint() == expected[i] &&
            packet.keep == 3 &&
            packet.strb == 3 &&
            packet.user == (i == 0) &&
            packet.last == (i % FRAME_WIDTH == FRAME_WIDTH - 1);

        if (!match) {
            failures++;

            if (failures <= 10) {
                std::cout << "Pixel " << i
                          << ": expected=" << expected[i]
                          << " got=" << packet.data.to_uint()
                          << " user=" << packet.user
                          << " last=" << packet.last
                          << " keep=" << packet.keep
                          << " strb=" << packet.strb
                          << "\n";
            }
        }
    }

    if (!output.empty()) {
        failures++;
        std::cout << "Extra output after frame\n";
    }

    std::cout << "BLC-to-BPC HLS: " << pixel_count
              << " pixels checked, " << failures
              << " failures\n";

    return failures == 0 ? 0 : 1;
}
