/*
Project: Adaptive Directional BPC and BLC
Module: HLS ISP Top Interface
Description: Declare the combined streaming BLC and BPC HLS top.
Author: Viet Nguyen To Quoc
*/

#pragma once

#include "blc/isp_blc.hpp"
#include "bpc/isp_bpc.hpp"

void isp_top(
    hls::stream<ap_axiu<16, 1, 0, 0>>& input,
    hls::stream<ap_axiu<16, 1, 0, 0>>& output,
    ap_ufixed<10, 10> bl_r,
    ap_ufixed<10, 10> bl_gr,
    ap_ufixed<10, 10> bl_gb,
    ap_ufixed<10, 10> bl_b,
    ap_ufixed<10, 10> thresh_r,
    ap_ufixed<10, 10> thresh_g,
    ap_ufixed<10, 10> thresh_b,
    ap_uint<4> shift_signal,
    ap_uint<4> shift_gradient
);

void isp_top_frames(
    hls::stream<ap_axiu<16, 1, 0, 0>>& input,
    hls::stream<ap_axiu<16, 1, 0, 0>>& output,
    ap_ufixed<10, 10> bl_r,
    ap_ufixed<10, 10> bl_gr,
    ap_ufixed<10, 10> bl_gb,
    ap_ufixed<10, 10> bl_b,
    ap_ufixed<10, 10> thresh_r,
    ap_ufixed<10, 10> thresh_g,
    ap_ufixed<10, 10> thresh_b,
    ap_uint<4> shift_signal,
    ap_uint<4> shift_gradient,
    ap_uint<32> frame_count
);
