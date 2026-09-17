/*
Project: Adaptive Directional BPC and BLC
Module: HLS Black Level Correction
Description: Implement the synthesizable Black Level Correction block.
Author: Viet Nguyen To Quoc
*/

#include "isp_blc.hpp"

ap_ufixed<10, 10> blc_pixel(
    ap_ufixed<10, 10> input,
    ap_uint<11> row,
    ap_uint<11> col,
    const BlcConfig& config
) {
    ap_ufixed<10, 10> black_level;

    /*
    ap_uint<1> row_even = row % 2 == 0;
    ap_uint<1> col_even = col % 2 == 0;
    */

    //get CFA Phase
    ap_uint<2> phase;
    phase[0] = col[0];
    phase[1] = row[0];

    //get black level from differenct CFA Phase
    switch (phase) {
        case 0b00:
            black_level = config.bl_r;
            break;

        case 0b01:
            black_level = config.bl_gr;
            break;

        case 0b10:
            black_level = config.bl_gb;
            break;

        case 0b11:
            black_level = config.bl_b;
            break;
    }

    //clamp(x - T)
    if (input <= black_level) {
        return ap_ufixed<10, 10>(0);
    }

    return (input - black_level);
}

void isp_blc_top(
    hls::stream<ap_axiu<16, 1, 0, 0>>& input,
    hls::stream<ap_axiu<16, 1, 0, 0>>& output,
    ap_ufixed<10, 10> bl_r,
    ap_ufixed<10, 10> bl_gr,
    ap_ufixed<10, 10> bl_gb,
    ap_ufixed<10, 10> bl_b
) {
    //AXI4-Stream RAW10 ports
#pragma HLS INTERFACE axis port=input
#pragma HLS INTERFACE axis port=output

    //AXI4-Lite control map
#pragma HLS INTERFACE s_axilite port=bl_r bundle=s_axi_ctrl offset=0x10
#pragma HLS INTERFACE s_axilite port=bl_gr bundle=s_axi_ctrl offset=0x18
#pragma HLS INTERFACE s_axilite port=bl_gb bundle=s_axi_ctrl offset=0x20
#pragma HLS INTERFACE s_axilite port=bl_b bundle=s_axi_ctrl offset=0x28

#pragma HLS INTERFACE s_axilite port=return bundle=s_axi_ctrl

    //local BLC config
    const BlcConfig config {
        bl_r,
        bl_gr,
        bl_gb,
        bl_b
    };

    //Full-HD raster scan
row_loop:
    for (ap_uint<11> row = 0; row < FRAME_HEIGHT; row++) {
    col_loop:
        for (ap_uint<11> col = 0; col < FRAME_WIDTH; col++) {
#pragma HLS PIPELINE II=1

            //unpack and correct RAW10
            ap_axiu<16, 1, 0, 0> input_packet = input.read();
            ap_ufixed<10, 10> input_pixel = input_packet.data.range(9, 0);
            ap_ufixed<10, 10> result = blc_pixel(input_pixel, row, col, config);

            //preserve sidebands, replace RAW10 payload
            ap_axiu<16, 1, 0, 0> output_packet = input_packet;
            output_packet.data = 0;
            output_packet.data.range(9, 0) = result;

            output.write(output_packet);
        }
    }
}