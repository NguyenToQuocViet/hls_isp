#ifndef LTM_GAMMA_HEADER
#define LTM_GAMMA_HEADER

#include "../isp_pixel_packet.hpp"
#include "ap_axi_sdata.h"
#include "ap_fixed.h"
#include "ap_int.h"
#include "hls_math.h"
#include "hls_print.h"
#include "hls_stream.h"
#include "hls_video_mem.h" // Includes hls::LineBuffer and hls::Window
#include <cmath>
#include <fstream>
#include <iostream>

#define WIDTH 713
#define HEIGHT 535
// #define DEBUG



// AXI-Stream video interface using Vitis library struct
typedef ap_axiu<36, 1, 0, 0> axis_pixel_36b;
typedef ap_axiu<24, 1, 0, 0> axis_pixel_24b;

// Data Types based on the specification
typedef ap_ufixed<12, 12> rgb_in_t;
typedef ap_ufixed<12, 12, AP_RND, AP_SAT> rgb_out_t;
typedef ap_ufixed<18, 12> exp_out_t; // 6 fractional bits for precision

// Log domain type: max value is ~9.7, needs to support subtraction (signed)
typedef ap_fixed<16, 5> log_t;
typedef ap_fixed<18, 8> var_t;

// Guided Filter Weights
typedef ap_ufixed<12, 1, AP_RND, AP_SAT> weight_a_t; // [0, 1]
typedef ap_fixed<15, 5> weight_b_t;                  // same magnitude as log_t

// Dimension Types for Optimization
typedef ap_uint<12> dim_t;
typedef ap_int<13> s_dim_t;
typedef ap_uint<22> img_size_t;

struct rgb_pack_t
{
    ap_uint<12> r;
    ap_uint<12> g;
    ap_uint<12> b;
    log_t log_y;
    ap_uint<12> y_int;
};

// Top-level function

void isp_ltm_top(hls::stream<IspPixelPacket<36>>& s_axis,
         hls::stream<IspPixelPacket<36>>& m_axis, const log_t log_lut[4096],
         const log_t reinhard_lut[1024], const exp_out_t exp_lut[1024]);


void isp_gamma_top(hls::stream<IspPixelPacket<36>>& stream_in,
                      hls::stream<IspPixelPacket<24>>& stream_out,
                      const ap_uint<8> gamma_lut_r[4096],
                      const ap_uint<8> gamma_lut_g[4096],
                      const ap_uint<8> gamma_lut_b[4096]);


// void rgb2yuv(hls::stream<IspPixelPacket<24>>& stream_in,
//              hls::stream<IspPixelPacket<16>>& stream_out, int height,
//              int width);

void isp_ltm_gamma_top(
    hls::stream<axis_pixel_36b>& s_axis,
    hls::stream<axis_pixel_24b>& m_axis,
    const log_t log_lut[4096],
    const log_t reinhard_lut[1024],
    const exp_out_t exp_lut[1024],
    const ap_uint<8> gamma_lut_r[4096],
    const ap_uint<8> gamma_lut_g[4096],
    const ap_uint<8> gamma_lut_b[4096]
);

#endif
