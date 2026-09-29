#include "isp_ccm_top.hpp"
#include <hls_task.h>

using namespace hls;

void isp_ccm(
    stream<IspPixelPacket<36>>& stream_in,
    stream<IspPixelPacket<36>>& stream_out
) {
    #pragma HLS PIPELINE II=1
    
    static const gain_matrix frame_a00 = 1.6;
    static const gain_matrix frame_a01 = -0.3;
    static const gain_matrix frame_a02 = -0.3;
    static const gain_matrix frame_a10 = -0.2;
    static const gain_matrix frame_a11 = 1.4;
    static const gain_matrix frame_a12 = -0.2;
    static const gain_matrix frame_a20 = -0.1;
    static const gain_matrix frame_a21 = -0.6;
    static const gain_matrix frame_a22 = 1.7;

    const IspPixelPacket<36> input_pixel = stream_in.read();

    const ap_uint<12> r_i = input_pixel.data.range(11, 0);
    const ap_uint<12> g_i = input_pixel.data.range(23, 12);
    const ap_uint<12> b_i = input_pixel.data.range(35, 24);

    const ap_fixed<30, 18> r_acc =
        r_i * frame_a00 + g_i * frame_a01 + b_i * frame_a02;

    const ap_fixed<30, 18> g_acc =
        r_i * frame_a10 + g_i * frame_a11 + b_i * frame_a12;

    const ap_fixed<30, 18> b_acc =
        r_i * frame_a20 + g_i * frame_a21 + b_i * frame_a22;

    const o_pixel r_o = (r_acc < 0) ? (o_pixel)0 : (o_pixel)r_acc;
    const o_pixel g_o = (g_acc < 0) ? (o_pixel)0 : (o_pixel)g_acc;
    const o_pixel b_o = (b_acc < 0) ? (o_pixel)0 : (o_pixel)b_acc;

    IspPixelPacket<36> output_pixel;
    output_pixel.data.range(11, 0) = r_o;
    output_pixel.data.range(23, 12) = g_o;
    output_pixel.data.range(35, 24) = b_o;
    output_pixel.user = input_pixel.user;
    output_pixel.last = input_pixel.last;
    
    stream_out.write(output_pixel);
}

static void input_adapter(
    hls::stream<ap_axiu<40, 1, 0, 0>>& input,
    hls::stream<IspPixelPacket<36>>& output
) {
    #pragma HLS PIPELINE II=1
    ap_axiu<40, 1, 0, 0> axi_pixel = input.read();
    IspPixelPacket<36> pixel;
    pixel.data = axi_pixel.data.range(35, 0);
    pixel.user = axi_pixel.user;
    pixel.last = axi_pixel.last;
    output.write(pixel);
}

static void output_adapter(
    hls::stream<IspPixelPacket<36>>& input,
    hls::stream<ap_axiu<40, 1, 0, 0>>& output
) {
    #pragma HLS PIPELINE II=1
    IspPixelPacket<36> pixel = input.read();
    ap_axiu<40, 1, 0, 0> axi_pixel;
    axi_pixel.data = 0;
    axi_pixel.data.range(35, 0) = pixel.data;
    axi_pixel.keep = -1;
    axi_pixel.strb = -1;
    axi_pixel.user = pixel.user;
    axi_pixel.last = pixel.last;
    output.write(axi_pixel);
}

void isp_ccm_top(
    hls::stream<ap_axiu<40, 1, 0, 0>>& input,
    hls::stream<ap_axiu<40, 1, 0, 0>>& output
) {
    #pragma HLS INTERFACE mode=axis port=input
    #pragma HLS INTERFACE mode=axis port=output
    #pragma HLS INTERFACE mode=ap_ctrl_none port=return

    hls_thread_local hls::stream<IspPixelPacket<36>> input_pixels;
    hls_thread_local hls::stream<IspPixelPacket<36>> output_pixels;

    hls_thread_local hls::task ingress_task(input_adapter, input, input_pixels);
    hls_thread_local hls::task ccm_task(isp_ccm, input_pixels, output_pixels);
    hls_thread_local hls::task egress_task(output_adapter, output_pixels, output);
}