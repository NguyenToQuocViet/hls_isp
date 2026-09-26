/*
Project: Adaptive Directional BPC and BLC
Module: HLS Black Level Correction
Description: Implement the HLS Black Level Correction pixel algorithm.
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
