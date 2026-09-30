/*
Project: Adaptive Directional BPC and BLC
Module: HLS Bad Pixel Correction Interface
Description: Declare the HLS Bad Pixel Correction pixel algorithm.
Author: Viet Nguyen To Quoc
*/

#pragma once

#include <ap_fixed.h>
#include <ap_int.h>
#include <hls_stream.h>
#include "isp_frame.hpp"
#include "isp_pixel_packet.hpp"

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

void bpc_engine(
    hls::stream<IspPixelPacket<10>> &input,
    hls::stream<IspPixelPacket<10>> &output,
    const BpcConfig &config
);