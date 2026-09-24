/*
Project: Adaptive Directional BPC and BLC
Module: Fixed-Threshold BPC Baseline Interface
Description: Declare the host-side fixed-threshold BPC reference-model interface.
Author: Viet Nguyen To Quoc
*/

#pragma once

#include <cstdint>
#include <vector>

//fixed thresholds
struct BpcConfig {
    std::uint16_t threshold_r;
    std::uint16_t threshold_g;
    std::uint16_t threshold_b;
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
