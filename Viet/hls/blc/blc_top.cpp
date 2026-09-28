/*
Project: Adaptive Directional BPC and BLC
Module: HLS Black Level Correction Top
Description: Connect AXI4-Stream pixel ports to the internal Black Level Correction engine.
Author: Viet Nguyen To Quoc
*/

#include "blc_top.hpp"
#include "isp_blc.hpp"
#include <hls_task.h>

static void publish_config(
    hls::ap_none<ap_uint<10>>& bl_r,
    hls::ap_none<ap_uint<10>>& bl_gr,
    hls::ap_none<ap_uint<10>>& bl_gb,
    hls::ap_none<ap_uint<10>>& bl_b,
    hls::ap_none<ap_uint<1>>& config_valid,
    hls::stream<BlcConfig, 2>& configs,
    hls::stream<ap_uint<1>, 2>& ingress_start
) {
    static bool published = false;
#pragma HLS RESET variable=published
    if (published || !config_valid.read()) {
        return;
    }

    BlcConfig config;
    config.bl_r = bl_r.read();
    config.bl_gr = bl_gr.read();
    config.bl_gb = bl_gb.read();
    config.bl_b = bl_b.read();
    configs.write(config);
    ingress_start.write(1);
    published = true;
}

static void input_adapter(
    hls::stream<ap_axiu<16, 1, 0, 0>>& input,
    hls::stream<IspPixelPacket<10>>& output,
    hls::stream<ap_uint<1>, 2>& ingress_start
) {
    static bool started = false;
#pragma HLS RESET variable=started

    if (!started) {
        ingress_start.read();
        started = true;
    }

#pragma HLS PIPELINE II=1 style=flp
    //convert one AXI beat to one internal pixel
    ap_axiu<16, 1, 0, 0> axi_pixel = input.read();
    IspPixelPacket<10> pixel;
    pixel.data = axi_pixel.data.range(9, 0);
    pixel.user = axi_pixel.user;
    pixel.last = axi_pixel.last;
    output.write(pixel);
}

static void run_engine(
    hls::stream<IspPixelPacket<10>>& input,
    hls::stream<IspPixelPacket<10>>& output,
    hls::stream<BlcConfig, 2>& configs
) {
    static bool configured = false;
    static BlcConfig config;
#pragma HLS RESET variable=configured

    if (!configured) {
        config = configs.read();
        configured = true;
    }

#pragma HLS PIPELINE II=1 style=flp
    blc_engine(input, output, config);
}

static void output_adapter(
    hls::stream<IspPixelPacket<10>>& input,
    hls::stream<ap_axiu<16, 1, 0, 0>>& output
) {
#pragma HLS PIPELINE II=1 style=flp
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
    hls::ap_none<ap_uint<10>>& bl_b,
    hls::ap_none<ap_uint<1>>& config_valid
) {
#pragma HLS INTERFACE mode=axis port=input
#pragma HLS INTERFACE mode=axis port=output

#pragma HLS INTERFACE mode=s_axilite port=bl_r
#pragma HLS INTERFACE mode=s_axilite port=bl_gr
#pragma HLS INTERFACE mode=s_axilite port=bl_gb
#pragma HLS INTERFACE mode=s_axilite port=bl_b
#pragma HLS INTERFACE mode=s_axilite port=config_valid

#pragma HLS INTERFACE mode=ap_ctrl_none port=return

    hls_thread_local hls::stream<IspPixelPacket<10>> input_pixels;
    hls_thread_local hls::stream<IspPixelPacket<10>> output_pixels;
    hls_thread_local hls::stream<BlcConfig, 2> configs;
    hls_thread_local hls::stream<ap_uint<1>, 2> ingress_start;

    hls_thread_local hls::task publisher(
        publish_config, bl_r, bl_gr, bl_gb, bl_b, config_valid,
        configs, ingress_start
    );

    hls_thread_local hls::task ingress(input_adapter, input, input_pixels, ingress_start);
    hls_thread_local hls::task engine(run_engine, input_pixels, output_pixels, configs);
    hls_thread_local hls::task egress(output_adapter, output_pixels, output);
}
