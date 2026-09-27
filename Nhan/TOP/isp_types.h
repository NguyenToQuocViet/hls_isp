#ifndef ISP_TYPES_H_
#define ISP_TYPES_H_

#include "ap_axi_sdata.h"
#include "ap_fixed.h"
#include "ap_int.h"
#include "hls_stream.h"

#include "isp_frame_config.h"
#include "isp_pixel_packet.hpp"

typedef ap_uint<10> raw10_t;
typedef ap_uint<12> raw12_t;
typedef ap_uint<36> rgb36_t;

// 
typedef ap_uint<12> wb_scaled_input_t;

// 
typedef ap_ufixed<16, 4, AP_RND, AP_SAT> wb_gain_t;
typedef ap_fixed<16, 4> ccm_coeff_t;

// Gioi han 255 frame/transaction
typedef ap_uint<8> frame_count_t;

// 
typedef IspPixelPacket<10> raw10_packet_t;
typedef IspPixelPacket<12> raw12_packet_t;
typedef IspPixelPacket<36> rgb36_packet_t;

//
typedef ap_axiu<10, 1, 1, 1> axis_raw10_t;
typedef ap_axiu<36, 1, 1, 1> axis_rgb36_t;

static inline ap_uint<12> rgb_get_r(const rgb36_packet_t &pixel) {
    return pixel.data.range(11, 0);
}

static inline ap_uint<12> rgb_get_g(const rgb36_packet_t &pixel) {
    return pixel.data.range(23, 12);
}

static inline ap_uint<12> rgb_get_b(const rgb36_packet_t &pixel) {
    return pixel.data.range(35, 24);
}

static inline void rgb_set(
    rgb36_packet_t &pixel,
    ap_uint<12> r,
    ap_uint<12> g,
    ap_uint<12> b
) {
    pixel.data.range(11, 0) = r;
    pixel.data.range(23, 12) = g;
    pixel.data.range(35, 24) = b;
}

#endif
