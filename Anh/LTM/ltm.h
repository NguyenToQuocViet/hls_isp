#pragma once

#include "ap_fixed.h"
#include "ap_int.h"
#include "hls_stream.h"
#include "hls_video_mem.h"
#include "ltm.h"
#include "hls_math.h"
#include "hls_print.h"
#include "ap_axi_sdata.h"
#include <iostream>
#include <fstream>
#include <cmath>

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

// Reciprocal LUT type for Final Gain (Removed for manual calculation)
// typedef ap_ufixed<24, 1> gain_recip_t;

// Dimension Types for Optimization
typedef ap_uint<12> dim_t;
typedef ap_int<13> s_dim_t;
typedef ap_uint<22> img_size_t;


#define MAX_WIDTH 713
#define MAX_HEIGHT 535
#define DEBUG

struct IspStreamPixel36 {
    ap_uint<36> data;
    ap_uint<4> keep; // unused
    ap_uint<4> strb; // unused
    ap_uint<1> user; // start of frame
    ap_uint<1> last; // end of line
};

// AXI4-Stream custom packed structs
// typedef IspStreamPixel36 AXI_PIXEL_IN;
// typedef IspStreamPixel36 AXI_PIXEL_OUT;

typedef ap_axiu<36, 1, 0, 0> AXI_PIXEL_IN;
typedef ap_axiu<36, 1, 0, 0> AXI_PIXEL_OUT;

// Top-level function
#ifdef DEBUG
void ltm(hls::stream<AXI_PIXEL_IN>& s_axis, hls::stream<AXI_PIXEL_OUT>& m_axis,
         const log_t log_lut[4096], const log_t reinhard_lut[1024],
         const exp_out_t exp_lut[1024],
         volatile int& debug_pixels);
#else
void ltm(hls::stream<AXI_PIXEL_IN>& s_axis, hls::stream<AXI_PIXEL_OUT>& m_axis,
         const log_t log_lut[4096], const log_t reinhard_lut[1024],
         const exp_out_t exp_lut[1024]);
#endif
