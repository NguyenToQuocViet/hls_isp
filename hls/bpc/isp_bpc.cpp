/*
Project: Adaptive Directional BPC and BLC
Module: HLS Bad Pixel Correction
Description: Implement the synthesizable Bad Pixel Correction block.
Author: Viet Nguyen To Quoc
*/

#include "isp_bpc.hpp"

#include <array>

ap_ufixed<12, 12> abs_diff(ap_ufixed<12, 12> a, ap_ufixed<12, 12> b) {
    if (a > b) {
        return a - b;
    }

    return b - a;
}

ap_ufixed<12, 12> bpc_pixel(const BpcWindow& input, ap_uint<12> row, ap_uint<12> col, const BpcConfig& config) {
    //4 directional pairs: H, V, D1, D2
    const std::array<ap_ufixed<12, 12>, 4> first = {
        input.left,
        input.up,
        input.up_left,
        input.up_right
    };
    
    const std::array<ap_ufixed<12, 12>, 4> second = {
        input.right,
        input.down,
        input.down_right,
        input.down_left
    };

    //directional gradients and predictions
    std::array<ap_ufixed<12, 12>, 4> gradients;
    std::array<ap_ufixed<12, 12>, 4> predictions;
    #pragma HLS ARRAY_PARTITION variable=gradients complete
    #pragma HLS ARRAY_PARTITION variable=predictions complete

    for (std::size_t i = 0; i < 4; i++) {
        #pragma HLS UNROLL

        gradients[i] = abs_diff(first[i], second[i]);
        const ap_ufixed<13, 13> pair_sum =
            ap_ufixed<13, 13>(first[i]) + ap_ufixed<13, 13>(second[i]);
        predictions[i] = pair_sum >> 1;
    }

    //balanced selection preserves H, V, D1, D2 tie priority
    const bool select_v = gradients[1] < gradients[0];
    const ap_ufixed<12, 12> axis_gradient = select_v ? gradients[1] : gradients[0];
    const ap_ufixed<12, 12> axis_prediction = select_v ? predictions[1] : predictions[0];

    const bool select_d2 = gradients[3] < gradients[2];
    const ap_ufixed<12, 12> diagonal_gradient = select_d2 ? gradients[3] : gradients[2];
    const ap_ufixed<12, 12> diagonal_prediction = select_d2 ? predictions[3] : predictions[2];

    const bool select_diagonal = diagonal_gradient < axis_gradient;
    const ap_ufixed<12, 12> min_gradients = select_diagonal ? diagonal_gradient : axis_gradient;
    const ap_ufixed<12, 12> prediction = select_diagonal ? diagonal_prediction : axis_prediction;

    //get CFA Phase
    ap_uint<2> phase;
    phase[0] = col[0];
    phase[1] = row[0];

    //get threshold from differenct CFA Phase
    ap_ufixed<12, 12> base_thresh;
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
    ap_ufixed<14, 14> threshold = base_thresh + (prediction >> config.shift_signal) + (min_gradients >> config.shift_gradient);

    //detection and correction
    if (abs_diff(input.center, prediction) > threshold) {
        return prediction;
    }

    return input.center;
}

void isp_bpc_top(hls::stream<ap_axiu<16, 1, 0, 0>>& input, hls::stream<ap_axiu<16, 1, 0, 0>>& output, ap_ufixed<12, 12> thresh_r, ap_ufixed<12, 12> thresh_g, ap_ufixed<12, 12> thresh_b, ap_uint<4> shift_signal, ap_uint<4> shift_gradient) {
    //AXI4-Stream RAW12 ports
    #pragma HLS INTERFACE axis port=input
    #pragma HLS INTERFACE axis port=output

    //AXI4-Lite control map
    #pragma HLS INTERFACE s_axilite port=thresh_r bundle=s_axi_ctrl offset=0x10
    #pragma HLS INTERFACE s_axilite port=thresh_g bundle=s_axi_ctrl offset=0x18
    #pragma HLS INTERFACE s_axilite port=thresh_b bundle=s_axi_ctrl offset=0x20
    
    #pragma HLS INTERFACE s_axilite port=shift_signal bundle=s_axi_ctrl offset=0x28
    #pragma HLS INTERFACE s_axilite port=shift_gradient bundle=s_axi_ctrl offset=0x30
    
    #pragma HLS INTERFACE s_axilite port=return bundle=s_axi_ctrl

    constexpr int PIXEL_COUNT = FRAME_WIDTH * FRAME_HEIGHT;
    constexpr int CENTER_DELAY = (2 * FRAME_WIDTH) + 2;
    constexpr int PROCESS_COUNT = PIXEL_COUNT + CENTER_DELAY;

    //streaming window storage
    ap_ufixed<12, 12> line_buffer[4][FRAME_WIDTH];
    ap_ufixed<12, 12> horizontal[3][5];
    #pragma HLS ARRAY_PARTITION variable=line_buffer complete dim=1
    #pragma HLS ARRAY_PARTITION variable=horizontal complete dim=0

    const BpcConfig config {
        thresh_r,
        thresh_g,
        thresh_b,
        
        shift_signal,
        shift_gradient
    };

    ap_uint<12> input_row = 0;
    ap_uint<12> input_col = 0;
    ap_uint<12> output_row = 0;
    ap_uint<12> output_col = 0;

    //full-frame stream and drain
    pixel_loop:
    for (int step = 0; step < PROCESS_COUNT; step++) {
        #pragma HLS PIPELINE II=1

        const ap_uint<1> receive_pixel = step < PIXEL_COUNT;
        const ap_uint<1> produce_pixel = step >= CENTER_DELAY;

        ap_ufixed<12, 12> new_pixel = 0;
        if (receive_pixel) {
            const ap_axiu<16, 1, 0, 0> input_packet = input.read();
            new_pixel = input_packet.data.range(11, 0);
        }

        //vertical history at current column
        ap_ufixed<12, 12> old_lb1 = 0;
        ap_ufixed<12, 12> old_lb2 = 0;
        ap_ufixed<12, 12> old_lb3 = 0;
        ap_ufixed<12, 12> old_lb4 = 0;

        if (input_row >= 1) {
            old_lb1 = line_buffer[0][input_col];
        }
        if (input_row >= 2) {
            old_lb2 = line_buffer[1][input_col];
        }
        if (input_row >= 3) {
            old_lb3 = line_buffer[2][input_col];
        }
        if (input_row >= 4) {
            old_lb4 = line_buffer[3][input_col];
        }

        line_buffer[0][input_col] = new_pixel;
        line_buffer[1][input_col] = old_lb1;
        line_buffer[2][input_col] = old_lb2;
        line_buffer[3][input_col] = old_lb3;

        //continuous horizontal window
        shift_row_loop:
        for (int row = 0; row < 3; row++) {
            #pragma HLS UNROLL

            shift_col_loop:
            for (int col = 0; col < 4; col++) {
                #pragma HLS UNROLL

                horizontal[row][col] = horizontal[row][col + 1];
            }
        }

        horizontal[0][4] = old_lb4;
        horizontal[1][4] = old_lb2;
        horizontal[2][4] = new_pixel;

        if (produce_pixel) {
            const BpcWindow window {
                horizontal[1][2],
                horizontal[1][0],
                horizontal[1][4],
                horizontal[0][2],
                horizontal[2][2],
                horizontal[0][0],
                horizontal[0][4],
                horizontal[2][0],
                horizontal[2][4]
            };

            const ap_uint<1> border =
                (output_row < 2) ||
                (output_row >= (FRAME_HEIGHT - 2)) ||
                (output_col < 2) ||
                (output_col >= (FRAME_WIDTH - 2));

            ap_ufixed<12, 12> result = window.center;
            
            if (!border) {
                result = bpc_pixel(window, output_row, output_col, config);
            }

            //output payload and coordinates
            ap_axiu<16, 1, 0, 0> output_packet;
            output_packet.data = 0;
            output_packet.data.range(11, 0) = result;
            output_packet.keep = -1;
            output_packet.strb = -1;
            output_packet.user = (output_row == 0) && (output_col == 0);
            output_packet.last = output_col == (FRAME_WIDTH - 1);
            output.write(output_packet);

            if (output_col == (FRAME_WIDTH - 1)) {
                output_col = 0;
                output_row++;
            } else {
                output_col++;
            }
        }

        if (input_col == (FRAME_WIDTH - 1)) {
            input_col = 0;
            input_row++;
        } else {
            input_col++;
        }
    }
}
