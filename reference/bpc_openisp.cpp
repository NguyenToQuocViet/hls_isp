/*
Project: Adaptive Directional BPC and BLC
Module: OpenISP DPC Reference
Description: Implement the OpenISP gradient dead-pixel correction baseline for RAW Bayer frames.
Author: Viet Nguyen To Quoc
*/

#include "bpc_openisp.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <stdexcept>

namespace {

// Behavioral source, pinned for provenance:
// https://github.com/cruxopen/openISP/blob/d4947e1aa5f4af83c3640131dbca8a675b613ec6/model/dpc.py
// OpenISP uses a reflected 5 x 5 Bayer patch, compares the center against all
// eight same-CFA neighbors with a strict threshold, and uses gradient mode by
// default. The published 10-bit threshold 30 maps to 120 in this RAW12 model.

int reflect_index(int index, int extent) {
    if (index < 0) {
        return -index;
    }
    if (index >= extent) {
        return (2 * extent) - index - 2;
    }
    return index;
}

std::uint16_t sample_reflect(
    const std::uint16_t* input,
    int row,
    int col
) {
    const int reflected_row = reflect_index(row, openisp_bpc::FRAME_HEIGHT);
    const int reflected_col = reflect_index(col, openisp_bpc::FRAME_WIDTH);
    return input[(reflected_row * openisp_bpc::FRAME_WIDTH) + reflected_col];
}

openisp_bpc::BpcPixelResult correct_pixel(
    const std::uint16_t* input,
    int row,
    int col,
    std::uint16_t threshold
) {
    const int center = sample_reflect(input, row, col);
    const std::array<int, 8> neighbors = {
        sample_reflect(input, row - 2, col - 2),
        sample_reflect(input, row - 2, col),
        sample_reflect(input, row - 2, col + 2),
        sample_reflect(input, row, col - 2),
        sample_reflect(input, row, col + 2),
        sample_reflect(input, row + 2, col - 2),
        sample_reflect(input, row + 2, col),
        sample_reflect(input, row + 2, col + 2),
    };

    const bool detected = std::all_of(
        neighbors.begin(),
        neighbors.end(),
        [center, threshold](int neighbor) {
            return std::abs(neighbor - center) > threshold;
        }
    );
    if (!detected) {
        return {static_cast<std::uint16_t>(center), false};
    }

    const std::array<std::array<int, 2>, 4> directional_pairs = {{
        {neighbors[1], neighbors[6]},
        {neighbors[3], neighbors[4]},
        {neighbors[0], neighbors[7]},
        {neighbors[2], neighbors[5]},
    }};
    std::size_t selected = 0;
    int minimum_gradient = std::abs(
        (2 * center) - directional_pairs[0][0] - directional_pairs[0][1]
    );
    for (std::size_t direction = 1; direction < directional_pairs.size(); direction++) {
        const int gradient = std::abs(
            (2 * center) -
            directional_pairs[direction][0] -
            directional_pairs[direction][1]
        );
        if (gradient < minimum_gradient) {
            minimum_gradient = gradient;
            selected = direction;
        }
    }

    const int replacement =
        (directional_pairs[selected][0] + directional_pairs[selected][1] + 1) / 2;
    return {static_cast<std::uint16_t>(replacement), true};
}

void process_frame(
    const std::uint16_t* input,
    std::uint16_t* output,
    std::vector<openisp_bpc::Detection>* detections,
    std::uint16_t threshold
) {
    if (detections != nullptr) {
        detections->clear();
    }

    for (int row = 0; row < openisp_bpc::FRAME_HEIGHT; row++) {
        for (int col = 0; col < openisp_bpc::FRAME_WIDTH; col++) {
            const std::size_t index =
                static_cast<std::size_t>(row) * openisp_bpc::FRAME_WIDTH + col;
            const openisp_bpc::BpcPixelResult result =
                correct_pixel(input, row, col, threshold);
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

namespace openisp_bpc {

BpcPixelResult bpc_pixel(
    const std::vector<std::uint16_t>& input,
    int row,
    int col,
    std::uint16_t threshold
) {
    if (input.size() != FRAME_PIXELS) {
        throw std::invalid_argument("input must contain one 1920 x 1080 frame");
    }
    if ((row < 0) || (row >= FRAME_HEIGHT) ||
        (col < 0) || (col >= FRAME_WIDTH)) {
        throw std::out_of_range("pixel coordinate is outside the frame");
    }
    return correct_pixel(input.data(), row, col, threshold);
}

void bpc_frame(
    const std::vector<std::uint16_t>& input,
    std::vector<std::uint16_t>& output,
    std::vector<Detection>& detections,
    std::uint16_t threshold
) {
    if (&input == &output) {
        throw std::invalid_argument("input and output must be distinct");
    }
    if (input.size() != FRAME_PIXELS) {
        throw std::invalid_argument("input must contain one 1920 x 1080 frame");
    }

    output.resize(FRAME_PIXELS);
    process_frame(input.data(), output.data(), &detections, threshold);
}

} // namespace openisp_bpc

extern "C" int openisp_bpc_process_raw12(
    const std::uint16_t* input,
    std::uint16_t* output,
    std::size_t pixel_count,
    std::uint16_t threshold
) {
    if ((input == nullptr) || (output == nullptr)) {
        return -1;
    }
    if (pixel_count != openisp_bpc::FRAME_PIXELS) {
        return -2;
    }
    if (input == output) {
        return -3;
    }

    try {
        process_frame(input, output, nullptr, threshold);
    } catch (...) {
        return -4;
    }
    return 0;
}
