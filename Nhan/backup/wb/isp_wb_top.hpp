#ifndef ISP_WB_H_
#define ISP_WB_H_

#include "ap_axi_sdata.h"
#include "ap_fixed.h"
#include "ap_int.h"
#include "hls_stream.h"
#include "../isp_pixel_packet.hpp"


typedef ap_ufixed<12, 12, AP_RND, AP_SAT> o_pixel;

typedef ap_ufixed<16, 4> gain_t;

void isp_wb(
    hls::stream<IspPixelPacket<10>>& stream_in,
    hls::stream<IspPixelPacket<12>>& stream_out
);

void isp_wb_top(
    hls::stream<ap_axiu<16, 1, 0, 0>>& input,
    hls::stream<ap_axiu<16, 1, 0, 0>>& output
);

#endif
