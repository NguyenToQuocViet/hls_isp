/*
Project: Adaptive Directional BPC and BLC
Module: Adaptive Directional BPC
Description: Implement the host-side adaptive directional BPC reference model.
Author: Viet Nguyen To Quoc
*/

#include "bpc_adaptive.hpp"

#include <array>
#include <cstddef>
#include <stdexcept>

namespace adaptive_bpc {

//frame configuration
constexpr int WIDTH = 1920;
constexpr int HEIGHT = 1080;

//absolute difference
std::uint16_t abs_diff(std::uint16_t a, std::uint16_t b) {
    if (a > b) {
        return a - b;
    } else {
        return b - a;
    }
}

//single-pixel correction
BpcPixelResult bpc_pixel(const std::vector<std::uint16_t>& input, int row, int col, const BpcConfig& config) {
    //center sample
    const int index = row * WIDTH + col;
    const std::uint16_t center = input[index];

    //two-pixel border bypass
    if ((row < 2) || (row >= (HEIGHT - 2)) || (col < 2) || (col >= (WIDTH - 2))) {
        return {center, false};
    }

    //directional same-phase pairs: H, V, D1, D2
    const std::array<std::array<std::uint16_t, 2>, 4> directional_pairs = {{
        {input[(row * WIDTH) + (col - 2)], input[(row * WIDTH) + (col + 2)]},
        {input[((row - 2) * WIDTH) + col], input[((row + 2) * WIDTH) + col]},
        {input[((row - 2) * WIDTH) + (col - 2)], input[((row + 2) * WIDTH) + (col + 2)]},
        {input[((row - 2) * WIDTH) + (col + 2)], input[((row + 2) * WIDTH) + (col - 2)]}
    }};

    //smoothest direction with H, V, D1, D2 tie priority
    std::size_t selected_direction = 0;
    std::uint16_t minimum_activity = abs_diff(directional_pairs[0][0], directional_pairs[0][1]);

    for (std::size_t direction = 1; direction < directional_pairs.size(); direction++) {
        const std::uint16_t activity = abs_diff(directional_pairs[direction][0], directional_pairs[direction][1]);

        if (activity < minimum_activity) {
            minimum_activity = activity;
            selected_direction = direction;
        }
    }

    //directional prediction
    const std::uint32_t pair_sum =
        static_cast<std::uint32_t>(directional_pairs[selected_direction][0]) +
        static_cast<std::uint32_t>(directional_pairs[selected_direction][1]);
    const std::uint16_t prediction = static_cast<std::uint16_t>(pair_sum >> 1);

    //RGGB base threshold
    const bool row_odd = (row % 2) == 1;
    const bool col_odd = (col % 2) == 1;

    std::uint16_t base_threshold = config.base_threshold_g;
    if (!row_odd && !col_odd) {
        base_threshold = config.base_threshold_r;
    } else if (row_odd && col_odd) {
        base_threshold = config.base_threshold_b;
    }

    //signal- and activity-adaptive threshold
    const std::uint32_t signal_term = static_cast<std::uint32_t>(prediction) >> config.signal_shift;
    const std::uint32_t activity_term = static_cast<std::uint32_t>(minimum_activity) >> config.activity_shift;
    const std::uint32_t adaptive_threshold =
        static_cast<std::uint32_t>(base_threshold) + signal_term + activity_term;

    //outlier detection and correction
    if (abs_diff(center, prediction) > adaptive_threshold) {
        return {prediction, true};
    }

    return {center, false};
}

//full-frame correction
void bpc_frame(const std::vector<std::uint16_t>& input, std::vector<std::uint16_t>& output, std::vector<Detection>& detections, const BpcConfig& config) {
    //separate input and output buffers
    if (&input == &output) {
        throw std::invalid_argument("input and output must be distinct");
    }

    //initialize outputs
    output = input;
    detections.clear();

    //process centers with a complete 5 x 5 neighborhood
    for (int row = 2; row < HEIGHT - 2; row++) {
        for (int col = 2; col < WIDTH - 2; col++) {
            const int index = (row * WIDTH) + col;

            BpcPixelResult result;
            result = bpc_pixel(input, row, col, config);
            output[index] = result.value;

            //record corrected pixels
            if (result.detected) {
                Detection data;
                data.row = row;
                data.col = col;
                data.original = input[index];
                data.replacement = result.value;

                detections.push_back(data);
            }
        }
    }
}

} //namespace adaptive_bpc
