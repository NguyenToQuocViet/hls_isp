// #ifndef __ISP_CCM_H__
// #define __ISP_CCM_H__
// #include "ap_fixed.h"
// #include "ap_int.h"
// #include "hls_stream.h"

// #define base_addr_ccm 0x44A5'0000 
// #define offset_a00 0x10
// #define offset_a01 0x18
// #define offset_a02 0x20
// #define offset_a10 0x28
// #define offset_a11 0x30
// #define offset_a12 0x38
// #define offset_a20 0x40
// #define offset_a21 0x48
// #define offset_a22 0x50

// #define MAX_WIDTH 9 // 9 for testbench; 1920 for fpga

// typedef ap_ufixed<12, 12> i_pixel; //output tu demosaic la input cua ccm
// typedef ap_fixed<16, 4> gain_matrix; //kieu du lieu he so ma tran
// typedef ap_ufixed<12, 12, AP_RND, AP_SAT> o_pixel; //output cua ccm

// //Cấu trúc gói AXI4-Stream ngõ vào (36-bit từ khối demosaic)
// struct video_in_t {
//     i_pixel data_r;
//     i_pixel data_g;
//     i_pixel data_b;
//     ap_uint<1> user; // Cờ báo đầu khung (Start of Frame)
//     ap_uint<1> last; // Cờ báo cuối dòng (End of Line)
// };

// //Cấu trúc gói AXI4-Stream ngõ ra (36-bit xuất sang khối LTM
// struct video_out_t {
//     o_pixel data_r;    // Sử dụng cùng kiểu với input của LTM
//     o_pixel data_g;
//     o_pixel data_b;
//     ap_uint<1> user;
//     ap_uint<1> last;
// };

// //Prototype
// void isp_ccm_top(
//     hls::stream<video_in_t> &stream_in,
//     hls::stream<video_out_t> &stream_out,
//     gain_matrix a00, gain_matrix a01, gain_matrix a02,
//     gain_matrix a10, gain_matrix a11, gain_matrix a12,
//     gain_matrix a20, gain_matrix a21, gain_matrix a22
// );

// #endif

#ifndef ISP_CCM_H_
#define ISP_CCM_H_

#include "ap_axi_sdata.h"
#include "ap_fixed.h"
#include "ap_int.h"
#include "hls_stream.h"

// Address information reserved for the AXI-Lite register wrapper of the
// complete ISP. This free-running core itself exposes direct coefficient ports.
#define base_addr_ccm 0x44A50000
#define offset_a00    0x10
#define offset_a01    0x18
#define offset_a02    0x20
#define offset_a10    0x28
#define offset_a11    0x30
#define offset_a12    0x38
#define offset_a20    0x40
#define offset_a21    0x48
#define offset_a22    0x50

typedef ap_ufixed<12, 12> i_pixel;
typedef ap_fixed<16, 4> gain_matrix;
typedef ap_ufixed<12, 12, AP_RND, AP_SAT> o_pixel;
typedef ap_fixed<30, 18> ccm_accum_t;

// AXI4-Stream video packet:
//   TDATA[11:0]  = R
//   TDATA[23:12] = G
//   TDATA[35:24] = B
//   TUSER[0]     = Start Of Frame
//   TLAST        = End Of Line
typedef ap_axiu<36, 1, 1, 1> video_axis_t;
typedef video_axis_t video_in_t;
typedef video_axis_t video_out_t;

static inline ap_uint<12> axis_get_r(const video_axis_t &pixel) {
    return pixel.data.range(11, 0);
}

static inline ap_uint<12> axis_get_g(const video_axis_t &pixel) {
    return pixel.data.range(23, 12);
}

static inline ap_uint<12> axis_get_b(const video_axis_t &pixel) {
    return pixel.data.range(35, 24);
}

static inline void axis_set_rgb(
    video_axis_t &pixel,
    ap_uint<12> r,
    ap_uint<12> g,
    ap_uint<12> b
) {
    pixel.data.range(11, 0) = r;
    pixel.data.range(23, 12) = g;
    pixel.data.range(35, 24) = b;
}

void isp_ccm_top(
    hls::stream<video_in_t> &stream_in,
    hls::stream<video_out_t> &stream_out,
    gain_matrix a00, gain_matrix a01, gain_matrix a02,
    gain_matrix a10, gain_matrix a11, gain_matrix a12,
    gain_matrix a20, gain_matrix a21, gain_matrix a22
);

#endif
