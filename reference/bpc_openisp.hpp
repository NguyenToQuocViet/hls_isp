/*
Project: Adaptive Directional BPC and BLC
Module: OpenISP DPC Reference Interface
Description: Declare the host-side reference interface for the OpenISP gradient dead-pixel correction behavior.
Author: Viet Nguyen To Quoc
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace openisp_bpc {

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
    int col,
    std::uint16_t threshold
);

void bpc_frame(
    const std::vector<std::uint16_t>& input,
    std::vector<std::uint16_t>& output,
    std::vector<Detection>& detections,
    std::uint16_t threshold
);

} // namespace openisp_bpc

// Stable C entry point used by scripts/openisp_bpc_benchmark.py through ctypes.
// Returns 0 on success, -1 for a null pointer, -2 for an invalid frame size,
// -3 for aliased input/output, and -4 for an unexpected implementation error.
extern "C" int openisp_bpc_process_raw12(
    const std::uint16_t* input,
    std::uint16_t* output,
    std::size_t pixel_count,
    std::uint16_t threshold
);
