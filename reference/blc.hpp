/*
Project: Adaptive Directional BPC and BLC
Module: Black Level Correction Interface
Description: Declare the host-side Black Level Correction reference-model interface.
Author: Viet Nguyen To Quoc
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace blc {

struct BlcConfig {
    std::uint16_t black_level_r;
    std::uint16_t black_level_gr;
    std::uint16_t black_level_gb;
    std::uint16_t black_level_b;
};

std::uint16_t blc_pixel(std::uint16_t input, std::size_t row, std::size_t col, const BlcConfig& config);

void blc_frame(
    const std::vector<std::uint16_t>& input,
    std::vector<std::uint16_t>& output,
    std::size_t width,
    std::size_t height,
    const BlcConfig& config
);

} //namespace blc
