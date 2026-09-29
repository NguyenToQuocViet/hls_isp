#ifndef ISP_WB_H_
#define ISP_WB_H_

#include "ap_axi_sdata.h"
#include "ap_fixed.h"
#include "ap_int.h"
#include "hls_stream.h"
#include "../isp_pixel_packet.hpp"

#define FRAME_WIDTH 1920
#define FRAME_HEIGHT 1080

#define offset_gain_r 0x10
#define offset_gain_gr 0x18
#define offset_gain_gb 0x20
#define offset_gain_b 0x28

typedef ap_ufixed<12, 12, AP_RND, AP_SAT> o_pixel;

typedef ap_ufixed<16, 4> gain_t;

void isp_wb(
    hls::stream<IspPixelPacket<10>>& stream_in,
    hls::stream<IspPixelPacket<12>>& stream_out,
    gain_t gain_r,
    gain_t gain_gr,
    gain_t gain_gb,
    gain_t gain_b
);

void isp_wb_top(
    hls::stream<ap_axiu<16, 1, 0, 0>>& input,
    hls::stream<ap_axiu<16, 1, 0, 0>>& output,
    gain_t gain_r,
    gain_t gain_gr,
    gain_t gain_gb,
    gain_t gain_b
);

#endif
