/*
Project: Adaptive Directional BPC and BLC
Module: HLS Bad Pixel Correction Top Interface
Description: Declare the standalone AXI4-Stream Bad Pixel Correction top.
Author: Viet Nguyen To Quoc
*/

#pragma once

#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <hls_stream.h>

void bpc_top(
    hls::stream<ap_axiu<16, 1, 0, 0>>& input,
    hls::stream<ap_axiu<16, 1, 0, 0>>& output,
    ap_uint<10> thresh_r,
    ap_uint<10> thresh_g,
    ap_uint<10> thresh_b,
    ap_uint<4> shift_signal,
    ap_uint<4> shift_gradient
);
