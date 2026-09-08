/*
Project: Adaptive Directional BPC and BLC
Module: Reference Directed Tests
Description: Check BLC phase/saturation and adaptive direction ties through public interfaces.
Author: Viet Nguyen To Quoc
*/
#include "blc.hpp"
#include "bpc_adaptive.hpp"
#include "bpc_baseline.hpp"
#include <array>
#include <cassert>
#include <iostream>

int main() {
    const blc::BlcConfig black{64, 66, 65, 68};
    const std::array<std::uint16_t, 4> values{64, 66, 65, 68};
    for (std::size_t phase = 0; phase < 4; phase++) {
        const auto value = values[phase];
        assert(blc::blc_pixel(value - 1, phase / 2, phase % 2, black) == 0);
        assert(blc::blc_pixel(value, phase / 2, phase % 2, black) == 0);
        assert(blc::blc_pixel(value + 1, phase / 2, phase % 2, black) == 1);
    }
    std::vector<std::uint16_t> input{0, 212, 300, 4095}, output;
    blc::blc_frame(input, output, 2, 2, {212, 212, 212, 212});
    assert((output == std::vector<std::uint16_t>{0, 0, 88, 3883}));
    blc::blc_frame(input, output, 2, 2, {0, 0, 0, 0});
    assert(output == input);

    constexpr int width = 1920;
    constexpr int row = 10, col = 10, i = row * width + col;
    std::vector<std::uint16_t> frame(width * 1080, 100);
    frame[i] = 4095;
    frame[i - 2] = frame[i + 2] = 100;
    frame[i - 2 * width] = frame[i + 2 * width] = 200;
    frame[i - 2 * width - 2] = frame[i + 2 * width + 2] = 300;
    frame[i - 2 * width + 2] = frame[i + 2 * width - 2] = 400;
    const adaptive_bpc::BpcConfig config{0, 0, 0, 12, 12};
    assert(adaptive_bpc::bpc_pixel(frame, row, col, config).value == 100);
    frame[i + 2] = 101; // H loses its tie; V wins over D1 and D2.
    assert(adaptive_bpc::bpc_pixel(frame, row, col, config).value == 200);
    frame[i + 2 * width] = 201;
    assert(adaptive_bpc::bpc_pixel(frame, row, col, config).value == 300);
    frame[i + 2 * width + 2] = 301;
    assert(adaptive_bpc::bpc_pixel(frame, row, col, config).value == 400);
    std::fill(frame.begin(), frame.end(), 100);
    frame[i] = 110;
    assert(!adaptive_bpc::bpc_pixel(frame, row, col, {10, 10, 10, 12, 12}).detected);
    assert(adaptive_bpc::bpc_pixel(frame, row, col, {9, 9, 9, 12, 12}).detected);
    assert(!bpc_pixel(frame, row, col, {10, 10, 10}).detected);
    assert(bpc_pixel(frame, row, col, {9, 9, 9}).detected);

    // adaptive_v2 uses the selected pair's minimum activity in its strict threshold.
    std::fill(frame.begin(), frame.end(), 1000);
    frame[i - 2] = 100;
    frame[i + 2] = 110; // H is selected, P=105 and G_min=10.
    frame[i - 2 * width] = 200;
    frame[i + 2 * width] = 220;
    frame[i - 2 * width - 2] = 300;
    frame[i + 2 * width + 2] = 330;
    frame[i - 2 * width + 2] = 400;
    frame[i + 2 * width - 2] = 440;
    frame[i] = 115;
    assert(!adaptive_bpc::bpc_pixel(frame, row, col, {0, 0, 0, 12, 0}).detected);
    frame[i] = 116;
    assert(adaptive_bpc::bpc_pixel(frame, row, col, {0, 0, 0, 12, 0}).detected);

    // Adaptive BPC bypasses the two-pixel border.
    frame[0] = 4095;
    assert(!adaptive_bpc::bpc_pixel(frame, 0, 0, config).detected);
    assert(adaptive_bpc::bpc_pixel(frame, 0, 0, config).value == 4095);
    std::cout << "BLC phase/saturation, adaptive tie priority, G_min, strict threshold and border bypass: PASS\n";
}
