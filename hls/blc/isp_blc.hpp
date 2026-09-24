/*
Project: Adaptive Directional BPC and BLC
Module: HLS Black Level Correction Interface
Description: Declare the synthesizable Black Level Correction interface.
Author: Viet Nguyen To Quoc
*/

#pragma once

#include <ap_int.h>
#include <ap_fixed.h>
#include <ap_axi_sdata.h>
#include <hls_stream.h>

#include "../isp_frame.hpp"
#include "../isp_stream.hpp"

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

void blc_process_frame(
    hls::stream<IspStreamPixel>& input,
    hls::stream<IspStreamPixel>& output,
    ap_ufixed<10, 10> bl_r,
    ap_ufixed<10, 10> bl_gr,
    ap_ufixed<10, 10> bl_gb,
    ap_ufixed<10, 10> bl_b
);

void isp_blc_top(
    hls::stream<ap_axiu<16, 1, 0, 0>>& input,
    hls::stream<ap_axiu<16, 1, 0, 0>>& output,
    ap_ufixed<10, 10> bl_r,
    ap_ufixed<10, 10> bl_gr,
    ap_ufixed<10, 10> bl_gb,
    ap_ufixed<10, 10> bl_b
);
