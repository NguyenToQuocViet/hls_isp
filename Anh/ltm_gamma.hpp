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
#include "hls_task.h"
#include <cmath>
#include <fstream>
#include <iostream>

#define WIDTH 100
#define HEIGHT 100
// #define VER2 // data driven
// #define USE_AP_AXIU

#ifdef USE_AP_AXIU
typedef ap_axiu<36, 1, 0, 0> ltm_in_t;
typedef ap_axiu<36, 1, 0, 0> ltm_out_t;

typedef ap_axiu<36, 1, 0, 0> gamma_in_t;
typedef ap_axiu<24, 1, 0, 0> gamma_out_t;
#else
typedef IspPixelPacket<36> ltm_in_t;
typedef IspPixelPacket<36> ltm_out_t;

typedef IspPixelPacket<36> gamma_in_t;
typedef IspPixelPacket<24> gamma_out_t;
#endif

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

void isp_ltm_top(hls::stream<ltm_in_t>& s_axis,
         hls::stream<ltm_out_t>& m_axis);


void isp_gamma_top(hls::stream<gamma_in_t>& stream_in,
                      hls::stream<gamma_out_t>& stream_out);

void isp_gamma_top_ver2(hls::stream<gamma_in_t>& stream_in,
                      hls::stream<gamma_out_t>& stream_out);


// void rgb2yuv(hls::stream<IspPixelPacket<24>>& stream_in,
//              hls::stream<IspPixelPacket<16>>& stream_out, int height,
//              int width);

void isp_ltm_gamma_top(
    hls::stream<ltm_in_t>& s_axis,
    hls::stream<gamma_out_t>& m_axis
);

#endif
