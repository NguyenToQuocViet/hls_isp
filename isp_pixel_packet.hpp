/*
Project: Adaptive Directional BPC and BLC
Module: HLS ISP Pixel Packet
Description: Define a width-parameterized internal pixel stream packet.
Author: Viet Nguyen To Quoc
*/

#pragma once

#include <ap_int.h>

template<int DATA_BITS>
struct IspPixelPacket {
    ap_uint<DATA_BITS> data;
    ap_uint<1> user;
    ap_uint<1> last;
};
