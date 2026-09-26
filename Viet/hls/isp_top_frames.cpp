/*
Project: Adaptive Directional BPC and BLC
Module: Multi-Frame HLS ISP Top
Description: Process consecutive frames through one continuous BLC-to-BPC dataflow invocation.
Author: Viet Nguyen To Quoc
*/

#include "isp_top.hpp"

void isp_top_frames(
    hls::stream<ap_axiu<16, 1, 0, 0>>& input,
    hls::stream<ap_axiu<16, 1, 0, 0>>& output,
    ap_ufixed<10, 10> bl_r,
    ap_ufixed<10, 10> bl_gr,
    ap_ufixed<10, 10> bl_gb,
    ap_ufixed<10, 10> bl_b,
    ap_ufixed<10, 10> thresh_r,
    ap_ufixed<10, 10> thresh_g,
    ap_ufixed<10, 10> thresh_b,
    ap_uint<4> shift_signal,
    ap_uint<4> shift_gradient,
    ap_uint<32> frame_count
) {
#pragma HLS INTERFACE axis port=input
#pragma HLS INTERFACE axis port=output
#pragma HLS INTERFACE s_axilite port=bl_r bundle=s_axi_ctrl offset=0x10
#pragma HLS INTERFACE s_axilite port=bl_gr bundle=s_axi_ctrl offset=0x18
#pragma HLS INTERFACE s_axilite port=bl_gb bundle=s_axi_ctrl offset=0x20
#pragma HLS INTERFACE s_axilite port=bl_b bundle=s_axi_ctrl offset=0x28
#pragma HLS INTERFACE s_axilite port=thresh_r bundle=s_axi_ctrl offset=0x30
#pragma HLS INTERFACE s_axilite port=thresh_g bundle=s_axi_ctrl offset=0x38
#pragma HLS INTERFACE s_axilite port=thresh_b bundle=s_axi_ctrl offset=0x40
#pragma HLS INTERFACE s_axilite port=shift_signal bundle=s_axi_ctrl offset=0x48
#pragma HLS INTERFACE s_axilite port=shift_gradient bundle=s_axi_ctrl offset=0x50
#pragma HLS INTERFACE s_axilite port=frame_count bundle=s_axi_ctrl offset=0x58
#pragma HLS INTERFACE s_axilite port=return bundle=s_axi_ctrl

#pragma HLS DATAFLOW

    hls::stream<IspStreamPixel> raw_pixels;
    hls::stream<IspStreamPixel> blc_to_bpc;
    hls::stream<IspStreamPixel> corrected_pixels;
#pragma HLS STREAM variable=raw_pixels depth=2
#pragma HLS STREAM variable=blc_to_bpc depth=2
#pragma HLS STREAM variable=corrected_pixels depth=2

    axis_to_internal_frames(input, raw_pixels, frame_count);

    blc_process_frames(
        raw_pixels, blc_to_bpc, bl_r, bl_gr, bl_gb, bl_b,
        frame_count
    );

    bpc_process_frames(
        blc_to_bpc, corrected_pixels, thresh_r, thresh_g,
        thresh_b, shift_signal, shift_gradient, frame_count
    );

    internal_to_axis_frames(corrected_pixels, output, frame_count);
}
