/*
Project: Adaptive Directional BPC and BLC
Module: Adaptive Directional BPC Interface
Description: Declare the host-side adaptive directional BPC reference-model interface.
Author: Viet Nguyen To Quoc
*/

#pragma once

#include <cstdint>
#include <vector>

namespace adaptive_bpc {

//adaptive parameters
struct BpcConfig {
    std::uint16_t base_threshold_r;
    std::uint16_t base_threshold_g;
    std::uint16_t base_threshold_b;
    std::uint8_t signal_shift;
    std::uint8_t activity_shift;
};

//pixel result
struct BpcPixelResult {
    std::uint16_t value;
    bool detected;
};

//detection record
struct Detection {
    int row;
    int col;
    std::uint16_t original;
    std::uint16_t replacement;
};

//single-pixel interface
BpcPixelResult bpc_pixel(
    const std::vector<std::uint16_t>& input,
    int row,
    int col,
    const BpcConfig& config
);

//full-frame interface
void bpc_frame(
    const std::vector<std::uint16_t>& input,
    std::vector<std::uint16_t>& output,
    std::vector<Detection>& detections,
    const BpcConfig& config
);

} //namespace adaptive_bpc
