/*
Project: Adaptive Directional BPC and BLC
Module: HLS Black Level Correction Top
Description: Connect AXI4-Stream pixel ports to the internal Black Level Correction engine.
Author: Viet Nguyen To Quoc
*/

#include "blc_top.hpp"
#include "isp_blc.hpp"

static void input_adapter(
    hls::stream<ap_axiu<16, 1, 0, 0>>& input,
    hls::stream<IspPixelPacket<10>>& output
) {
#pragma HLS INLINE off
    bool started = false;

    for (int index = 0; index < FRAME_WIDTH * FRAME_HEIGHT;) {
#pragma HLS PIPELINE II=1 style=flp
        //convert one AXI beat to one internal pixel
        ap_axiu<16, 1, 0, 0> axi_pixel = input.read();
        IspPixelPacket<10> pixel;
        pixel.data = axi_pixel.data.range(9, 0);
        pixel.user = axi_pixel.user;
        pixel.last = axi_pixel.last;
        //wait for SOF at the start of each frame
        if (!started && !pixel.user) {
            continue;
        }

        started = true;
        output.write(pixel);
        index++;
    }
}

static void output_adapter(
    hls::stream<IspPixelPacket<10>>& input,
    hls::stream<ap_axiu<16, 1, 0, 0>>& output
) {
#pragma HLS INLINE off
    for (int index = 0; index < FRAME_WIDTH * FRAME_HEIGHT; index++) {
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
}

void blc_top(
    hls::stream<ap_axiu<16, 1, 0, 0>>& input,
    hls::stream<ap_axiu<16, 1, 0, 0>>& output,
    ap_uint<10> bl_r,
    ap_uint<10> bl_gr,
    ap_uint<10> bl_gb,
    ap_uint<10> bl_b
) {
#pragma HLS INTERFACE mode=axis port=input
#pragma HLS INTERFACE mode=axis port=output

#pragma HLS INTERFACE mode=s_axilite port=bl_r bundle=control
#pragma HLS INTERFACE mode=s_axilite port=bl_gr bundle=control
#pragma HLS INTERFACE mode=s_axilite port=bl_gb bundle=control
#pragma HLS INTERFACE mode=s_axilite port=bl_b bundle=control
#pragma HLS INTERFACE mode=s_axilite port=return bundle=control
#pragma HLS INTERFACE mode=ap_ctrl_hs port=return

    const BlcConfig config = {bl_r, bl_gr, bl_gb, bl_b};
    hls::stream<IspPixelPacket<10>> input_pixels;
    hls::stream<IspPixelPacket<10>> output_pixels;
#pragma HLS STREAM variable=input_pixels depth=2
#pragma HLS STREAM variable=output_pixels depth=2

#pragma HLS DATAFLOW
    input_adapter(input, input_pixels);
    blc_engine(input_pixels, output_pixels, config);
    output_adapter(output_pixels, output);
}
