/*
Project: Adaptive Directional BPC and BLC
Module: HLS Black Level Correction Interface
Description: Declare the HLS Black Level Correction pixel algorithm.
Author: Viet Nguyen To Quoc
*/

#pragma once

#include <ap_int.h>
#include <ap_fixed.h>

struct BlcConfig {
    ap_ufixed<10, 10> bl_r;
    ap_ufixed<10, 10> bl_gr;
    ap_ufixed<10, 10> bl_gb;
    ap_ufixed<10, 10> bl_b;
};

ap_ufixed<10, 10> blc_pixel(
    ap_ufixed<10, 10> input,
    ap_uint<11> row,
    ap_uint<11> col,
    const BlcConfig& config
);
