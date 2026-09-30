#ifndef ISP_WB_DEMOSAIC_CCM_H_
#define ISP_WB_DEMOSAIC_CCM_H_

#include "ap_axi_sdata.h"
#include "ap_fixed.h"
#include "ap_int.h"
#include "hls_stream.h"
#include "../isp_pixel_packet.hpp"

#define FRAME_WIDTH 80
#define FRAME_HEIGHT 45
//isp_wb
#define offset_gain_r 0x10
#define offset_gain_gr 0x18
#define offset_gain_gb 0x20
#define offset_gain_b 0x28
//isp_ccm
#define offset_a00 0x30
#define offset_a01 0x38
#define offset_a02 0x40
#define offset_a10 0x48
#define offset_a11 0x50
#define offset_a12 0x58
#define offset_a20 0x60
#define offset_a21 0x68
#define offset_a22 0x70
//isp_demosaic
#define THRESHOLD_T 270
#define EDGE_MAG_T  23
#define MAX_WIDTH FRAME_WIDTH

//
typedef ap_ufixed<12, 12, AP_RND, AP_SAT> o_pixel;

//isp_wb
typedef ap_ufixed<16, 4> gain_t;

void isp_wb(
    hls::stream<IspPixelPacket<10>>& stream_in,
    hls::stream<IspPixelPacket<12>>& stream_out,
    gain_t gain_r,
    gain_t gain_gr,
    gain_t gain_gb,
    gain_t gain_b
);

//isp_ccm
typedef ap_fixed<16, 4> gain_matrix;

void isp_ccm(
    hls::stream<IspPixelPacket<36>> &stream_in,
    hls::stream<IspPixelPacket<36>> &stream_out,
    gain_matrix a00, gain_matrix a01, gain_matrix a02,
    gain_matrix a10, gain_matrix a11, gain_matrix a12,
    gain_matrix a20, gain_matrix a21, gain_matrix a22 
);

//isp_demosaic
// G giu them 4 bit phan le trong qua trinh noi suy.
typedef ap_ufixed<16, 12> g_pixel;
// Pixel trung gian giua tang noi suy G va tang noi suy R/B.
struct green_pixel_t {
    ap_uint<12> raw;
    g_pixel g;
};

void isp_demosaic(
    hls::stream<IspPixelPacket<12>> &stream_in,
    hls::stream<IspPixelPacket<36>> &stream_out
);

//
void isp_top_wb_demosaic_ccm(
    hls::stream<ap_axiu<16, 1, 0, 0>>& input,
    hls::stream<ap_axiu<40, 1, 0, 0>>& output,
    gain_t gain_r,
    gain_t gain_gr,
    gain_t gain_gb,
    gain_t gain_b,
    gain_matrix a00, gain_matrix a01, gain_matrix a02,
    gain_matrix a10, gain_matrix a11, gain_matrix a12,
    gain_matrix a20, gain_matrix a21, gain_matrix a22
);

#endif
