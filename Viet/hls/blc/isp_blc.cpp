/*
Project: Adaptive Directional BPC and BLC
Module: HLS Black Level Correction
Description: Implement the HLS Black Level Correction pixel algorithm and streaming engine.
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

void blc_engine(
    hls::stream<IspPixelPacket<10>>& input,
    hls::stream<IspPixelPacket<10>>& output,
    hls::ap_none<ap_uint<10>>& bl_r,
    hls::ap_none<ap_uint<10>>& bl_gr,
    hls::ap_none<ap_uint<10>>& bl_gb,
    hls::ap_none<ap_uint<10>>& bl_b
) {
#pragma HLS PIPELINE II=1 style=flp
    static ap_uint<11> in_row = 0;
    static ap_uint<11> in_col = 0;
    static ap_uint<1> config_loaded = 0;
    static BlcConfig config;

    IspPixelPacket<10> pixel = input.read();

    //get SOF & EOL from internal coordinates
    ap_uint<1> expected_sof = ((in_row == 0) && (in_col == 0));
    ap_uint<1> expected_eol = (in_col == (FRAME_WIDTH - 1));

    //wait for SOF at the start of each frame
    if (expected_sof && !pixel.user) {
        return;
    }

    if (!config_loaded) {
        config.bl_r = bl_r.read();
        config.bl_gr = bl_gr.read();
        config.bl_gb = bl_gb.read();
        config.bl_b = bl_b.read();
        config_loaded = 1;
    }

    //correct pixel and generate output SOF & EOL
    pixel.data = blc_pixel(pixel.data, in_row, in_col, config);
    pixel.user = expected_sof;
    pixel.last = expected_eol;
    output.write(pixel);

    //advance once for every processed pixel
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
