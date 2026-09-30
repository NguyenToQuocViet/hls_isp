/*
Project: Adaptive Directional BPC and BLC
Module: HLS Black Level Correction Interface
Description: Declare the HLS Black Level Correction pixel algorithm and streaming engine.
Author: Viet Nguyen To Quoc
*/

#pragma once

#include <ap_int.h>
#include <ap_fixed.h>
#include <hls_stream.h>
#include "isp_frame.hpp"
#include "isp_pixel_packet.hpp"

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

void blc_engine(
    hls::stream<IspPixelPacket<10>>& input,
    hls::stream<IspPixelPacket<10>>& output,
    const BlcConfig& config
);
