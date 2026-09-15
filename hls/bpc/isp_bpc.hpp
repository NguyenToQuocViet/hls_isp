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

constexpr int FRAME_WIDTH = 1920;
constexpr int FRAME_HEIGHT = 1080;

struct BpcConfig {
    ap_ufixed<12, 12> thresh_r;
    ap_ufixed<12, 12> thresh_g;
    ap_ufixed<12, 12> thresh_b;

    ap_uint<4> shift_signal;
    ap_uint<4> shift_gradient;
};

struct BpcWindow {
    ap_ufixed<12, 12> center;
    ap_ufixed<12, 12> left;
    ap_ufixed<12, 12> right;
    ap_ufixed<12, 12> up;
    ap_ufixed<12, 12> down;
    ap_ufixed<12, 12> up_left;
    ap_ufixed<12, 12> up_right;
    ap_ufixed<12, 12> down_left;
    ap_ufixed<12, 12> down_right;
};

ap_ufixed<12, 12> bpc_pixel(const BpcWindow& input, ap_uint<12> row, ap_uint<12> col, const BpcConfig& config);

void isp_bpc_top(hls::stream<ap_axiu<16, 1, 0, 0>>& input, hls::stream<ap_axiu<16, 1, 0, 0>>& output, ap_ufixed<12, 12> thresh_r, ap_ufixed<12, 12> thresh_g, ap_ufixed<12, 12> thresh_b, ap_uint<4> shift_signal, ap_uint<4> shift_gradient);
