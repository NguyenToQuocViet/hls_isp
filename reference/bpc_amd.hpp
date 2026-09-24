/*
Project: Adaptive Directional BPC and BLC
Module: AMD Vitis Vision BPC Reference Interface
Description: Declare the host-side reference interface for the AMD Vitis Vision bad-pixel correction behavior.
Author: Viet Nguyen To Quoc
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace amd_bpc {

constexpr int FRAME_WIDTH = 1920;
constexpr int FRAME_HEIGHT = 1080;
constexpr std::size_t FRAME_PIXELS =
    static_cast<std::size_t>(FRAME_WIDTH) * static_cast<std::size_t>(FRAME_HEIGHT);

struct BpcPixelResult {
    std::uint16_t value;
    bool detected;
};

struct Detection {
    int row;
    int col;
    std::uint16_t original;
    std::uint16_t replacement;
};

BpcPixelResult bpc_pixel(
    const std::vector<std::uint16_t>& input,
    int row,
    int col
);

void bpc_frame(
    const std::vector<std::uint16_t>& input,
    std::vector<std::uint16_t>& output,
    std::vector<Detection>& detections
);

} // namespace amd_bpc

// Stable C entry point used by scripts/amd_bpc_benchmark.py through ctypes.
// Returns 0 on success, -1 for a null pointer, -2 for an invalid frame size,
// -3 for aliased input/output, and -4 for an unexpected implementation error.
extern "C" int amd_bpc_process_raw10(
    const std::uint16_t* input,
    std::uint16_t* output,
    std::size_t pixel_count
);
