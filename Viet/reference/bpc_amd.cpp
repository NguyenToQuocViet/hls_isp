/*
Project: Adaptive Directional BPC and BLC
Module: AMD Vitis Vision BPC Reference
Description: Implement the parameter-free AMD Vitis Vision min-max clamp baseline for RAW Bayer frames.
Author: Viet Nguyen To Quoc
*/

#include "bpc_amd.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace {

// Behavioral source:
// https://github.com/Xilinx/Vitis_Libraries/blob/main/vision/L1/include/imgproc/xf_bpc.hpp
// The AMD implementation selects the 3 x 3 same-CFA lattice inside a 5 x 5
// patch, excludes the center, and clamps the center to the neighbors' range.

std::uint16_t sample_constant_zero(
    const std::uint16_t* input,
    int row,
    int col
) {
    if ((row < 0) || (row >= amd_bpc::FRAME_HEIGHT) ||
        (col < 0) || (col >= amd_bpc::FRAME_WIDTH)) {
        return 0;
    }
    return input[(row * amd_bpc::FRAME_WIDTH) + col];
}

amd_bpc::BpcPixelResult correct_pixel(
    const std::uint16_t* input,
    int row,
    int col
) {
    const std::uint16_t center =
        input[(row * amd_bpc::FRAME_WIDTH) + col];
    std::array<std::uint16_t, 8> neighbors{};
    std::size_t index = 0;

    for (int row_offset = -2; row_offset <= 2; row_offset += 2) {
        for (int col_offset = -2; col_offset <= 2; col_offset += 2) {
            if ((row_offset == 0) && (col_offset == 0)) {
                continue;
            }
            neighbors[index++] = sample_constant_zero(
                input,
                row + row_offset,
                col + col_offset
            );
        }
    }

    const auto bounds = std::minmax_element(neighbors.begin(), neighbors.end());
    const std::uint16_t replacement =
        std::clamp(center, *bounds.first, *bounds.second);
    return {replacement, replacement != center};
}

void process_frame(
    const std::uint16_t* input,
    std::uint16_t* output,
    std::vector<amd_bpc::Detection>* detections
) {
    if (detections != nullptr) {
        detections->clear();
    }

    for (int row = 0; row < amd_bpc::FRAME_HEIGHT; row++) {
        for (int col = 0; col < amd_bpc::FRAME_WIDTH; col++) {
            const std::size_t index =
                static_cast<std::size_t>(row) * amd_bpc::FRAME_WIDTH + col;
            const amd_bpc::BpcPixelResult result =
                correct_pixel(input, row, col);
            output[index] = result.value;

            if ((detections != nullptr) && result.detected) {
                detections->push_back({
                    row,
                    col,
                    input[index],
                    result.value
                });
            }
        }
    }
}

} // namespace

namespace amd_bpc {

BpcPixelResult bpc_pixel(
    const std::vector<std::uint16_t>& input,
    int row,
    int col
) {
    if (input.size() != FRAME_PIXELS) {
        throw std::invalid_argument("input must contain one 1920 x 1080 frame");
    }
    if ((row < 0) || (row >= FRAME_HEIGHT) ||
        (col < 0) || (col >= FRAME_WIDTH)) {
        throw std::out_of_range("pixel coordinate is outside the frame");
    }
    return correct_pixel(input.data(), row, col);
}

void bpc_frame(
    const std::vector<std::uint16_t>& input,
    std::vector<std::uint16_t>& output,
    std::vector<Detection>& detections
) {
    if (&input == &output) {
        throw std::invalid_argument("input and output must be distinct");
    }
    if (input.size() != FRAME_PIXELS) {
        throw std::invalid_argument("input must contain one 1920 x 1080 frame");
    }

    output.resize(FRAME_PIXELS);
    process_frame(input.data(), output.data(), &detections);
}

} // namespace amd_bpc

extern "C" int amd_bpc_process_raw10(
    const std::uint16_t* input,
    std::uint16_t* output,
    std::size_t pixel_count
) {
    if ((input == nullptr) || (output == nullptr)) {
        return -1;
    }
    if (pixel_count != amd_bpc::FRAME_PIXELS) {
        return -2;
    }
    if (input == output) {
        return -3;
    }

    try {
        process_frame(input, output, nullptr);
    } catch (...) {
        return -4;
    }
    return 0;
}
