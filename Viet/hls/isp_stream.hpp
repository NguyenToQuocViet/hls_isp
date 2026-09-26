/*
Project: Adaptive Directional BPC and BLC
Module: HLS Internal ISP Stream
Description: Define the internal pixel packet and AXI Stream boundary adapters.
Author: Viet Nguyen To Quoc
*/

#pragma once

#include "isp_frame.hpp"

#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <hls_stream.h>

struct IspStreamPixel {
    ap_uint<16> data;
    ap_uint<2> keep;
    ap_uint<2> strb;
    ap_uint<1> user;
    ap_uint<1> last;
};

inline void axis_to_internal_frames(
    hls::stream<ap_axiu<16, 1, 0, 0>>& input,
    hls::stream<IspStreamPixel>& output,
    ap_uint<32> frame_count
) {
#pragma HLS INLINE off

    ap_uint<32> frame = 0;
    ap_uint<21> pixel_index = 0;

    while (frame < frame_count) {
#pragma HLS PIPELINE II=1
        const ap_axiu<16, 1, 0, 0> axis_pixel = input.read();
        IspStreamPixel pixel;
        pixel.data = axis_pixel.data;
        pixel.keep = axis_pixel.keep;
        pixel.strb = axis_pixel.strb;
        pixel.user = axis_pixel.user;
        pixel.last = axis_pixel.last;
        output.write(pixel);

        if (pixel_index == FRAME_WIDTH * FRAME_HEIGHT - 1) {
            pixel_index = 0;
            frame++;
        } else {
            pixel_index++;
        }
    }
}

inline void axis_to_internal_frame(
    hls::stream<ap_axiu<16, 1, 0, 0>>& input,
    hls::stream<IspStreamPixel>& output
) {
#pragma HLS INLINE off
    axis_to_internal_frames(input, output, 1);
}

inline void internal_to_axis_frames(
    hls::stream<IspStreamPixel>& input,
    hls::stream<ap_axiu<16, 1, 0, 0>>& output,
    ap_uint<32> frame_count
) {
#pragma HLS INLINE off

    ap_uint<32> frame = 0;
    ap_uint<21> pixel_index = 0;

    while (frame < frame_count) {
#pragma HLS PIPELINE II=1
        const IspStreamPixel pixel = input.read();
        ap_axiu<16, 1, 0, 0> axis_pixel;
        axis_pixel.data = pixel.data;
        axis_pixel.keep = pixel.keep;
        axis_pixel.strb = pixel.strb;
        axis_pixel.user = pixel.user;
        axis_pixel.last = pixel.last;
        output.write(axis_pixel);

        if (pixel_index == FRAME_WIDTH * FRAME_HEIGHT - 1) {
            pixel_index = 0;
            frame++;
        } else {
            pixel_index++;
        }
    }
}

inline void internal_to_axis_frame(
    hls::stream<IspStreamPixel>& input,
    hls::stream<ap_axiu<16, 1, 0, 0>>& output
) {
#pragma HLS INLINE off
    internal_to_axis_frames(input, output, 1);
}
