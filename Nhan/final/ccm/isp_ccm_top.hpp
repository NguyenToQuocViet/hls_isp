#ifndef ISP_CCM_H_
#define ISP_CCM_H_

#include "ap_axi_sdata.h"
#include "ap_fixed.h"
#include "ap_int.h"
#include "hls_stream.h"
#include "../isp_pixel_packet.hpp"

#define FRAME_WIDTH 80
#define FRAME_HEIGHT 45

#define offset_a00 0x10
#define offset_a01 0x18
#define offset_a02 0x20
#define offset_a10 0x28
#define offset_a11 0x30
#define offset_a12 0x38
#define offset_a20 0x40
#define offset_a21 0x48
#define offset_a22 0x50

typedef ap_fixed<16, 4> gain_matrix;
typedef ap_ufixed<12, 12, AP_RND, AP_SAT> o_pixel;


void isp_ccm(
    hls::stream<IspPixelPacket<36>> &stream_in,
    hls::stream<IspPixelPacket<36>> &stream_out,
    gain_matrix a00, gain_matrix a01, gain_matrix a02,
    gain_matrix a10, gain_matrix a11, gain_matrix a12,
    gain_matrix a20, gain_matrix a21, gain_matrix a22 
);

void isp_ccm_top(
    hls::stream<ap_axiu<40, 1, 0, 0>>& input,
    hls::stream<ap_axiu<40, 1, 0, 0>>& output,
    gain_matrix a00, gain_matrix a01, gain_matrix a02,
    gain_matrix a10, gain_matrix a11, gain_matrix a12,
    gain_matrix a20, gain_matrix a21, gain_matrix a22 
);

#endif
