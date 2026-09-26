/*
Project: Adaptive Directional BPC and BLC
Module: HLS Bad Pixel Correction
Description: Implement the HLS Bad Pixel Correction pixel algorithm.
Author: Viet Nguyen To Quoc
*/

#include "isp_bpc.hpp"

#include <array>

ap_ufixed<10, 10> abs_diff(
    ap_ufixed<10, 10> a,
    ap_ufixed<10, 10> b
) {
    if (a > b) {
        return a - b;
    }

    return b - a;
}

ap_ufixed<10, 10> bpc_pixel(
    const BpcWindow& input,
    ap_uint<11> row,
    ap_uint<11> col,
    const BpcConfig& config
) {
    //4 directional pairs: H, V, D1, D2
    const std::array<ap_ufixed<10, 10>, 4> first = {
        input.left,
        input.up,
        input.up_left,
        input.up_right
    };

    const std::array<ap_ufixed<10, 10>, 4> second = {
        input.right,
        input.down,
        input.down_right,
        input.down_left
    };

    //directional gradients and predictions
    std::array<ap_ufixed<10, 10>, 4> gradients;
    std::array<ap_ufixed<10, 10>, 4> predictions;
#pragma HLS ARRAY_PARTITION variable=gradients complete
#pragma HLS ARRAY_PARTITION variable=predictions complete

    for (std::size_t i = 0; i < 4; i++) {
#pragma HLS UNROLL

        gradients[i] = abs_diff(first[i], second[i]);

        const ap_ufixed<11, 11> pair_sum =
            ap_ufixed<11, 11>(first[i]) +
            ap_ufixed<11, 11>(second[i]);

        predictions[i] = pair_sum >> 1;
    }

    // Balanced selection preserves H, V, D1, D2 tie priority.
    ap_ufixed<10, 10> axis_gradient = gradients[0];
    ap_ufixed<10, 10> axis_prediction = predictions[0];

    if (gradients[1] < gradients[0]) {
        axis_gradient = gradients[1];
        axis_prediction = predictions[1];
    }

    ap_ufixed<10, 10> diagonal_gradient = gradients[2];
    ap_ufixed<10, 10> diagonal_prediction = predictions[2];

    if (gradients[3] < gradients[2]) {
        diagonal_gradient = gradients[3];
        diagonal_prediction = predictions[3];
    }

    ap_ufixed<10, 10> min_gradients = axis_gradient;
    ap_ufixed<10, 10> prediction = axis_prediction;

    if (diagonal_gradient < axis_gradient) {
        min_gradients = diagonal_gradient;
        prediction = diagonal_prediction;
    }

    //get CFA Phase
    ap_uint<2> phase;
    phase[0] = col[0];
    phase[1] = row[0];

    //get threshold from differenct CFA Phase
    ap_ufixed<10, 10> base_thresh;

    switch (phase) {
        case 0b00:
            base_thresh = config.thresh_r;
            break;

        case 0b01:
            base_thresh = config.thresh_g;
            break;

        case 0b10:
            base_thresh = config.thresh_g;
            break;

        case 0b11:
            base_thresh = config.thresh_b;
            break;
    }

    //adaptive directional threshold
    ap_ufixed<12, 12> threshold =
        base_thresh +
        (prediction >> config.shift_signal) +
        (min_gradients >> config.shift_gradient);

    //detection and correction
    if (abs_diff(input.center, prediction) > threshold) {
        return prediction;
    }

    return input.center;
}
