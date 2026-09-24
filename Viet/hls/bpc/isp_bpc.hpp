/*
Project: Adaptive Directional BPC and BLC
Module: HLS Bad Pixel Correction Interface
Description: Declare the synthesizable Bad Pixel Correction interface.
Author: Viet Nguyen To Quoc
*/

#pragma once

#include <ap_fixed.h>
#include <ap_int.h>
#include <ap_axi_sdata.h>
#include <hls_stream.h>

#include "../isp_frame.hpp"
#include "../isp_stream.hpp"

struct BpcConfig {
    ap_ufixed<10, 10> thresh_r;
    ap_ufixed<10, 10> thresh_g;
    ap_ufixed<10, 10> thresh_b;

    ap_uint<4> shift_signal;
    ap_uint<4> shift_gradient;
};

struct BpcWindow {
    ap_ufixed<10, 10> center;
    ap_ufixed<10, 10> left;
    ap_ufixed<10, 10> right;
    ap_ufixed<10, 10> up;
    ap_ufixed<10, 10> down;
    ap_ufixed<10, 10> up_left;
    ap_ufixed<10, 10> up_right;
    ap_ufixed<10, 10> down_left;
    ap_ufixed<10, 10> down_right;
};

ap_ufixed<10, 10> bpc_pixel(
    const BpcWindow& input,
    ap_uint<11> row,
    ap_uint<11> col,
    const BpcConfig& config
);

void bpc_process_frame(
    hls::stream<IspStreamPixel>& input,
    hls::stream<IspStreamPixel>& output,
    ap_ufixed<10, 10> thresh_r,
    ap_ufixed<10, 10> thresh_g,
    ap_ufixed<10, 10> thresh_b,
    ap_uint<4> shift_signal,
    ap_uint<4> shift_gradient
);

void isp_bpc_top(
    hls::stream<ap_axiu<16, 1, 0, 0>>& input,
    hls::stream<ap_axiu<16, 1, 0, 0>>& output,
    ap_ufixed<10, 10> thresh_r,
    ap_ufixed<10, 10> thresh_g,
    ap_ufixed<10, 10> thresh_b,
    ap_uint<4> shift_signal,
    ap_uint<4> shift_gradient
);
