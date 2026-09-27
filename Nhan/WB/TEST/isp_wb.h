#ifndef ISP_WB_H_
#define ISP_WB_H_

#include "ap_axi_sdata.h"
#include "ap_fixed.h"
#include "ap_int.h"
#include "hls_stream.h"

// Address information reserved for the AXI-Lite register wrapper of the
// complete ISP. This free-running core exposes direct coefficient ports.
#define base_addr_wb 0x44A30000
#define offset_r     0x10
#define offset_gr    0x18
#define offset_gb    0x20
#define offset_b     0x28

// Input RAW10
typedef ap_uint<10> i_pixel;

// RAW10 duoc dich trai 2 bit thanh mien 12-bit
typedef ap_uint<12> wb_scaled_input_t;

// Output 12-bit, lam tron va saturation
typedef ap_ufixed<12, 12, AP_RND, AP_SAT> o_pixel;

typedef ap_ufixed<16, 4> gain_t;

// 12-bit integer x Q4.12
typedef ap_ufixed<28, 16> wb_product_t;

// AXI4-Stream video packets:
//   TUSER[0] = Start Of Frame
//   TLAST    = End Of Line
// TID and TDEST are kept at one bit to match the complete ISP wrapper.
typedef ap_axiu<10, 1, 1, 1> video_in_t;
typedef ap_axiu<12, 1, 1, 1> video_out_t;

static inline ap_uint<10> axis_get_input_pixel(const video_in_t &pixel) {
    return pixel.data;
}

static inline void axis_set_input_pixel(
    video_in_t &pixel,
    ap_uint<10> value
) {
    pixel.data = value;
}

static inline ap_uint<12> axis_get_output_pixel(const video_out_t &pixel) {
    return pixel.data;
}

static inline void axis_set_output_pixel(
    video_out_t &pixel,
    ap_uint<12> value
) {
    pixel.data = value;
}

void isp_wb_top(
    hls::stream<video_in_t> &stream_in,
    hls::stream<video_out_t> &stream_out,
    gain_t gain_r,
    gain_t gain_gr,
    gain_t gain_gb,
    gain_t gain_b
);

#endif
