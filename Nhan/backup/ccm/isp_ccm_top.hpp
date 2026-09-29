#ifndef ISP_CCM_H_
#define ISP_CCM_H_

#include "ap_axi_sdata.h"
#include "ap_fixed.h"
#include "ap_int.h"
#include "hls_stream.h"
#include "../isp_pixel_packet.hpp"

typedef ap_fixed<16, 4> gain_matrix;
typedef ap_ufixed<12, 12, AP_RND, AP_SAT> o_pixel;


void isp_ccm(
    hls::stream<IspPixelPacket<36>> &stream_in,
    hls::stream<IspPixelPacket<36>> &stream_out
);

void isp_ccm_top(
    hls::stream<ap_axiu<40, 1, 0, 0>>& input,
    hls::stream<ap_axiu<40, 1, 0, 0>>& output
);

#endif
