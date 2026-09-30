/*
Project: Adaptive Directional BPC and BLC
Module: HLS Black Level Correction Top Interface
Description: Declare the standalone AXI4-Stream Black Level Correction top.
Author: Viet Nguyen To Quoc
*/

#pragma once

#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <hls_stream.h>

void blc_top(
    hls::stream<ap_axiu<16, 1, 0, 0>>& input,
    hls::stream<ap_axiu<16, 1, 0, 0>>& output,
    ap_uint<10> bl_r,
    ap_uint<10> bl_gr,
    ap_uint<10> bl_gb,
    ap_uint<10> bl_b
);
