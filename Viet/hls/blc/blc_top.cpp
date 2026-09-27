/*
Project: Adaptive Directional BPC and BLC
Module: HLS Black Level Correction Top
Description: Connect AXI4-Stream pixel ports to the internal Black Level Correction engine.
Author: Viet Nguyen To Quoc
*/

#include "blc_top.hpp"
#include "isp_blc.hpp"
#include <hls_task.h>

static void input_adapter(
    hls::stream<ap_axiu<16, 1, 0, 0>>& input,
    hls::stream<IspPixelPacket<10>>& output
) {
#pragma HLS PIPELINE II=1
    //convert one AXI beat to one internal pixel
    ap_axiu<16, 1, 0, 0> axi_pixel = input.read();
    IspPixelPacket<10> pixel;
    pixel.data = axi_pixel.data.range(9, 0);
    pixel.user = axi_pixel.user;
    pixel.last = axi_pixel.last;
    output.write(pixel);
}

static void output_adapter(
    hls::stream<IspPixelPacket<10>>& input,
    hls::stream<ap_axiu<16, 1, 0, 0>>& output
) {
#pragma HLS PIPELINE II=1
    //convert one internal pixel to one AXI beat
    IspPixelPacket<10> pixel = input.read();
    ap_axiu<16, 1, 0, 0> axi_pixel;
    axi_pixel.data = 0;
    axi_pixel.data.range(9, 0) = pixel.data;
    axi_pixel.keep = 0b11;
    axi_pixel.strb = 0b11;
    axi_pixel.user = pixel.user;
    axi_pixel.last = pixel.last;
    output.write(axi_pixel);
}

void blc_top(
    hls::stream<ap_axiu<16, 1, 0, 0>>& input,
    hls::stream<ap_axiu<16, 1, 0, 0>>& output,
    hls::ap_none<ap_uint<10>>& bl_r,
    hls::ap_none<ap_uint<10>>& bl_gr,
    hls::ap_none<ap_uint<10>>& bl_gb,
    hls::ap_none<ap_uint<10>>& bl_b
) {
#pragma HLS INTERFACE mode=axis port=input
#pragma HLS INTERFACE mode=axis port=output

#pragma HLS INTERFACE mode=s_axilite port=bl_r
#pragma HLS INTERFACE mode=s_axilite port=bl_gr
#pragma HLS INTERFACE mode=s_axilite port=bl_gb
#pragma HLS INTERFACE mode=s_axilite port=bl_b

#pragma HLS INTERFACE mode=ap_ctrl_none port=return
    hls_thread_local hls::stream<IspPixelPacket<10>> input_pixels;
    hls_thread_local hls::stream<IspPixelPacket<10>> output_pixels;

    hls_thread_local hls::task ingress(input_adapter, input, input_pixels);
    hls_thread_local hls::task engine(
        blc_engine, input_pixels, output_pixels, bl_r, bl_gr, bl_gb, bl_b
    );
    hls_thread_local hls::task egress(output_adapter, output_pixels, output);
}
