/*
Project: Adaptive Directional BPC and BLC
Module: HLS Bad Pixel Correction
Description: Implement the HLS Bad Pixel Correction pixel algorithm and streaming engine.
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

    //keep H, V, D1, D2 tie priority
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

void bpc_engine(
    hls::stream<IspPixelPacket<10>>& input,
    hls::stream<IspPixelPacket<10>>& output,
    const BpcConfig& config
) {
    ap_uint<11> in_row = 0;
    ap_uint<11> in_col = 0;
    ap_uint<11> out_row = 0;
    ap_uint<11> out_col = 0;

    ap_uint<11> lb_addr = 0;

    static ap_uint<10> lb_0[FRAME_WIDTH];
    static ap_uint<10> lb_1[FRAME_WIDTH];
    static ap_uint<10> lb_2[FRAME_WIDTH];
    static ap_uint<10> lb_3[FRAME_WIDTH];

    static ap_uint<10> horizontal_window[3][5];
#pragma HLS ARRAY_PARTITION variable=horizontal_window complete dim=0

#pragma HLS INLINE off

    const int frame_pixels = FRAME_WIDTH * FRAME_HEIGHT;
    const int center_delay = (2 * FRAME_WIDTH) + 2;

    for (int step = 0; step < frame_pixels + center_delay; step++) {
#pragma HLS PIPELINE II=1 style=flp
        BpcWindow computing_window;

        IspPixelPacket<10> in_pixel;
        IspPixelPacket<10> out_pixel;
        const bool is_real_input = (step < frame_pixels);

        //get EOL from input coordinates
        bool expected_eol = (in_col == (FRAME_WIDTH - 1));
        ap_uint<10> new_pixel = 0;

        //select real input or drain at the end of the frame
        if (is_real_input) {
            in_pixel = input.read();
            new_pixel = in_pixel.data;
        }

        //read old values before shifting line buffers
        ap_uint<10> old_lb0;
        ap_uint<10> old_lb1;
        ap_uint<10> old_lb2;
        ap_uint<10> old_lb3;

        old_lb0 = lb_0[lb_addr];
        old_lb1 = lb_1[lb_addr];
        old_lb2 = lb_2[lb_addr];
        old_lb3 = lb_3[lb_addr];

        //shift new pixel through four line buffers
        lb_0[lb_addr] = new_pixel;
        lb_1[lb_addr] = old_lb0;
        lb_2[lb_addr] = old_lb1;
        lb_3[lb_addr] = old_lb2;

        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 4; col++) {
                horizontal_window[row][col] = horizontal_window[row][col + 1];
            }
        }

        horizontal_window[0][4] = old_lb3;
        horizontal_window[1][4] = old_lb1;
        horizontal_window[2][4] = new_pixel;

        computing_window.center = horizontal_window[1][2];
        computing_window.left = horizontal_window[1][0];
        computing_window.right = horizontal_window[1][4];
        computing_window.up = horizontal_window[0][2];
        computing_window.down = horizontal_window[2][2];
        computing_window.up_left = horizontal_window[0][0];
        computing_window.up_right = horizontal_window[0][4];
        computing_window.down_left = horizontal_window[2][0];
        computing_window.down_right = horizontal_window[2][4];

        bool out_pixel_written = false;

        //emit real centers
        const bool center_is_real = (step >= center_delay);

        if (center_is_real) {
            const bool is_border =
                (out_row < 2) || (out_row >= FRAME_HEIGHT - 2) ||
                (out_col < 2) || (out_col >= FRAME_WIDTH - 2);

            if (is_border) {
                out_pixel.data = computing_window.center;
            } else {
                out_pixel.data = bpc_pixel(computing_window, out_row, out_col, config);
            }

            out_pixel.user = (out_row == 0) && (out_col == 0);
            out_pixel.last = (out_col == FRAME_WIDTH - 1);
            output.write(out_pixel);
            out_pixel_written = true;
        }

        if (out_pixel_written) {
            if (out_col == FRAME_WIDTH - 1) {
                out_col = 0;
                if (out_row == FRAME_HEIGHT - 1) {
                    out_row = 0;
                } else {
                    out_row++;
                }
            } else {
                out_col++;
            }
        }

        //advance input coordinates for real pixels
        if (is_real_input) {
            if (expected_eol) {
                in_col = 0;

                if (in_row == FRAME_HEIGHT - 1) {
                    in_row = 0;
                } else {
                    in_row++;
                }
            } else {
                in_col++;
            }
        }

        //advance line buffer address for real or synthetic pixels
        if (lb_addr == FRAME_WIDTH - 1) {
            lb_addr = 0;
        } else {
            lb_addr++;
        }
    }

}
