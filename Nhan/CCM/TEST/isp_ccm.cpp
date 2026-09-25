#include "isp_ccm.h"

using namespace hls;

void isp_ccm_top(
    stream<video_in_t> &stream_in,
    stream<video_out_t> &stream_out,
    gain_matrix a00, gain_matrix a01, gain_matrix a02,
    gain_matrix a10, gain_matrix a11, gain_matrix a12,
    gain_matrix a20, gain_matrix a21, gain_matrix a22
) {
    #pragma HLS INTERFACE axis port=stream_in
    #pragma HLS INTERFACE axis port=stream_out

    // A true ap_ctrl_none core cannot own AXI-Lite control registers.
    // The complete ISP wrapper should hold these values stable while streaming.
    #pragma HLS INTERFACE ap_none port=a00
    #pragma HLS INTERFACE ap_none port=a01
    #pragma HLS INTERFACE ap_none port=a02
    #pragma HLS INTERFACE ap_none port=a10
    #pragma HLS INTERFACE ap_none port=a11
    #pragma HLS INTERFACE ap_none port=a12
    #pragma HLS INTERFACE ap_none port=a20
    #pragma HLS INTERFACE ap_none port=a21
    #pragma HLS INTERFACE ap_none port=a22
    #pragma HLS INTERFACE ap_ctrl_none port=return

    #pragma HLS PIPELINE II=1
    
    static gain_matrix frame_a00 = 0;
    static gain_matrix frame_a01 = 0;
    static gain_matrix frame_a02 = 0;
    static gain_matrix frame_a10 = 0;
    static gain_matrix frame_a11 = 0;
    static gain_matrix frame_a12 = 0;
    static gain_matrix frame_a20 = 0;
    static gain_matrix frame_a21 = 0;
    static gain_matrix frame_a22 = 0;

    const video_in_t data_i = stream_in.read();

    const bool sof = (data_i.user != 0);

    const gain_matrix c00 = sof ? a00 : frame_a00;
    const gain_matrix c01 = sof ? a01 : frame_a01;
    const gain_matrix c02 = sof ? a02 : frame_a02;
    const gain_matrix c10 = sof ? a10 : frame_a10;
    const gain_matrix c11 = sof ? a11 : frame_a11;
    const gain_matrix c12 = sof ? a12 : frame_a12;
    const gain_matrix c20 = sof ? a20 : frame_a20;
    const gain_matrix c21 = sof ? a21 : frame_a21;
    const gain_matrix c22 = sof ? a22 : frame_a22;

    if (sof) {
        frame_a00 = a00;
        frame_a01 = a01;
        frame_a02 = a02;
        frame_a10 = a10;
        frame_a11 = a11;
        frame_a12 = a12;
        frame_a20 = a20;
        frame_a21 = a21;
        frame_a22 = a22;
    }

    const i_pixel r_i = axis_get_r(data_i);
    const i_pixel g_i = axis_get_g(data_i);
    const i_pixel b_i = axis_get_b(data_i);

    const ccm_accum_t r_acc =
        r_i * c00 + g_i * c01 + b_i * c02;

    const ccm_accum_t g_acc =
        r_i * c10 + g_i * c11 + b_i * c12;

    const ccm_accum_t b_acc =
    r_i * c20 + g_i * c21 + b_i * c22;

    const o_pixel r_o = (r_acc < 0) ? (o_pixel)0 : (o_pixel)r_acc;
    const o_pixel g_o = (g_acc < 0) ? (o_pixel)0 : (o_pixel)g_acc;
    const o_pixel b_o = (b_acc < 0) ? (o_pixel)0 : (o_pixel)b_acc;

    video_out_t data_o;
    data_o.data = 0;
    data_o.keep = data_i.keep;
    data_o.strb = data_i.strb;
    data_o.user = data_i.user;
    data_o.last = data_i.last;
    data_o.id   = data_i.id;
    data_o.dest = data_i.dest;
    axis_set_rgb(
        data_o,
        (ap_uint<12>)r_o,
        (ap_uint<12>)g_o,
        (ap_uint<12>)b_o
    );
    stream_out.write(data_o);
}

