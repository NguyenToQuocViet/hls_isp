/*
Project: Adaptive Directional BPC and BLC
Module: Fixed-Threshold BPC Baseline
Description: Implement the host-side fixed-threshold BPC reference model.
Author: Viet Nguyen To Quoc
*/

#include "bpc_baseline.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>

//fixed-threshold baseline

//frame configuration
constexpr int WIDTH = 1920;
constexpr int HEIGHT = 1080;

//single-pixel correction
BpcPixelResult bpc_pixel(const std::vector<std::uint16_t>& input, int row, int col, const BpcConfig& config) {
    //center sample
    const int index = row * WIDTH + col;
    const std::uint16_t center = input[index];

    //two-pixel border bypass
    if ((row < 2) || (row >= (HEIGHT - 2)) || (col < 2) || (col >= (WIDTH - 2))) {
        return {center, false};
    }

    //eight same-phase neighbors
    std::array<std::uint16_t, 8> neighbors = {
        input[(row * WIDTH) + (col - 2)],
        input[(row * WIDTH) + (col + 2)],
        input[((row - 2) * WIDTH) + col],
        input[((row + 2) * WIDTH) + col],
        input[((row - 2) * WIDTH) + (col - 2)],
        input[((row - 2) * WIDTH) + (col + 2)],
        input[((row + 2) * WIDTH) + (col - 2)],
        input[((row + 2) * WIDTH) + (col + 2)]
    };

    //median of eight neighbors
    std::sort(neighbors.begin(), neighbors.end());

    const std::uint32_t middle_sum = static_cast<std::uint32_t>(neighbors[3]) + static_cast<std::uint32_t>(neighbors[4]);
    const std::uint16_t median = static_cast<std::uint16_t>(middle_sum >> 1);

    //RGGB phase threshold
    const bool row_odd = (row % 2) == 1;
    const bool col_odd = (col % 2) == 1;

    std::uint16_t threshold = config.threshold_g;
    if (!row_odd && !col_odd) {
        threshold = config.threshold_r;
    } else if (row_odd && col_odd) {
        threshold = config.threshold_b;
    }

    //fixed-threshold detection and correction
    const std::uint16_t difference =
        (center > median) ? (center - median) : (median - center);

    if (difference > threshold) {
        return {median, true};
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
