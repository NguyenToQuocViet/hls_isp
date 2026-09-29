#ifndef ISP_DEMOSAIC_H_
#define ISP_DEMOSAIC_H_

#include "ap_axi_sdata.h"
#include "ap_fixed.h"
#include "ap_int.h"
#include "hls_stream.h"
#include "../isp_pixel_packet.hpp"

#define MAX_WIDTH 1920

#define THRESHOLD_T 270
#define EDGE_MAG_T  23
#define FRAME_WIDTH 80
#define FRAME_HEIGHT 45

typedef ap_ufixed<12, 12, AP_RND, AP_SAT> o_pixel;

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

void isp_demosaic_top(
    hls::stream<ap_axiu<16, 1, 0, 0>>& input,
    hls::stream<ap_axiu<40, 1, 0, 0>>& output
);

#endif
