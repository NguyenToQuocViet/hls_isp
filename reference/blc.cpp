/*
Project: Adaptive Directional BPC and BLC
Module: Black Level Correction
Description: Provide a guided skeleton for the host-side Black Level Correction reference model.
Author: Viet Nguyen To Quoc
*/

#include "blc.hpp"

#include <stdexcept>

namespace blc {

std::uint16_t blc_pixel(std::uint16_t input, std::size_t row, std::size_t col, const BlcConfig& config) {
    std::uint16_t black_level;

    if ( (row % 2 == 1) && (col % 2 == 1) ) {
        black_level = config.black_level_b;
    } else if ( (row % 2 == 0) && (col % 2 == 0) ) {
        black_level = config.black_level_r;
    } else if ( (row % 2 == 0) && (col % 2 == 1) ) {
        black_level = config.black_level_gr;
    } else {
        black_level = config.black_level_gb;
    }

    if (input > black_level) {
        return input - black_level;
    }

    return 0;
}

void blc_frame(const std::vector<std::uint16_t>& input, std::vector<std::uint16_t>& output, std::size_t width, std::size_t height, const BlcConfig& config) {
    output.resize(input.size());

    for (std::size_t row = 0; row < height; row++) {
        for (std::size_t col = 0; col < width; col++) {
            std::size_t idx = (row * width) + col;

            output[idx] = blc_pixel(input[idx], row, col, config);
        }
    }
}

} //namespace blc
