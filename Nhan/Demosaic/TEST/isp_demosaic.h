#ifndef ISP_DEMOSAIC_H_
#define ISP_DEMOSAIC_H_

#include "ap_axi_sdata.h"
#include "ap_fixed.h"
#include "ap_int.h"
#include "hls_stream.h"

// Chieu rong lon nhat duoc cap phat cho cac line buffer BRAM.
#define MAX_WIDTH 1920

// Gia tri mac dinh de software/wrapper tham khao.
#define THRESHOLD_T 270
#define EDGE_MAG_T  23

// RAW vao va moi kenh RGB ra deu co 12 bit khong dau.
typedef ap_ufixed<12, 12> i_pixel;
typedef ap_ufixed<12, 12, AP_RND, AP_SAT> o_pixel;

// G giu them 4 bit phan le trong qua trinh noi suy.
typedef ap_ufixed<16, 12> g_pixel;

// AXI4-Stream video:
//   input : TDATA[11:0]  = RAW Bayer RGGB
//   output: TDATA[11:0]  = R
//           TDATA[23:12] = G
//           TDATA[35:24] = B
//   TUSER[0] = Start Of Frame, TLAST = End Of Line.
typedef ap_axiu<12, 1, 1, 1> video_in_t;
typedef ap_axiu<36, 1, 1, 1> video_out_t;

static inline ap_uint<12> axis_get_raw(const video_in_t &pixel) {
    return pixel.data;
}

static inline void axis_set_raw(video_in_t &pixel, ap_uint<12> raw) {
    pixel.data = raw;
}

static inline ap_uint<12> axis_get_r(const video_out_t &pixel) {
    return (ap_uint<12>)pixel.data;
}

static inline ap_uint<12> axis_get_g(const video_out_t &pixel) {
    return (ap_uint<12>)(pixel.data >> 12);
}

static inline ap_uint<12> axis_get_b(const video_out_t &pixel) {
    return (ap_uint<12>)(pixel.data >> 24);
}

static inline void axis_set_rgb(
    video_out_t &pixel,
    ap_uint<12> r,
    ap_uint<12> g,
    ap_uint<12> b
) {
    pixel.data = (ap_uint<36>)r
               | ((ap_uint<36>)g << 12)
               | ((ap_uint<36>)b << 24);
}

//   8 <= width <= MAX_WIDTH, height >= 2;
//   Gioi han width toi thieu bao dam hai direction bank ping-pong da ghi xong
//   truoc khi doi vai tro trong RTL pipeline II=1.
//   edge_threshold va edge_mag nam trong [0, 4095];
//   width, height va hai nguong phai duoc giu on dinh trong khi stream chay.
void isp_demosaicing_top(
    hls::stream<video_in_t> &stream_in,
    hls::stream<video_out_t> &stream_out,
    int width,
    int height,
    int edge_threshold,
    int edge_mag
);

#endif
