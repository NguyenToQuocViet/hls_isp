#include "isp_ccm_top.hpp"

using namespace hls;

void isp_ccm(
    stream<IspPixelPacket<36>>& stream_in,
    stream<IspPixelPacket<36>>& stream_out,
    gain_matrix a00, gain_matrix a01, gain_matrix a02,
    gain_matrix a10, gain_matrix a11, gain_matrix a12,
    gain_matrix a20, gain_matrix a21, gain_matrix a22 
) {
    #pragma HLS INLINE off
    
    for(int y = 0; y < FRAME_HEIGHT; ++y) {
        for(int x = 0; x < FRAME_WIDTH; ++x) {
            #pragma HLS LOOP_FLATTEN
            #pragma HLS PIPELINE II=1
            
            IspPixelPacket<36> p_in = stream_in.read();
            
            ap_uint<12> r_i = p_in.data.range(11, 0);
            ap_uint<12> g_i = p_in.data.range(23, 12);
            ap_uint<12> b_i = p_in.data.range(35, 24);

            const ap_fixed<30, 18> r_acc = r_i * a00 + g_i * a01 + b_i * a02;
            const ap_fixed<30, 18> g_acc = r_i * a10 + g_i * a11 + b_i * a12;
            const ap_fixed<30, 18> b_acc = r_i * a20 + g_i * a21 + b_i * a22;

            const o_pixel r_o = (r_acc < 0) ? (o_pixel)0 : (o_pixel)r_acc;
            const o_pixel g_o = (g_acc < 0) ? (o_pixel)0 : (o_pixel)g_acc;
            const o_pixel b_o = (b_acc < 0) ? (o_pixel)0 : (o_pixel)b_acc;

            IspPixelPacket<36> p_out;
            p_out.data.range(11, 0) = r_o;
            p_out.data.range(23, 12) = g_o;
            p_out.data.range(35, 24) = b_o;
            p_out.user = p_in.user;
            p_out.last = p_in.last;
            stream_out.write(p_out);
        }
    }
}

static void input_adapter(
    hls::stream<ap_axiu<40, 1, 0, 0>>& input,
    hls::stream<IspPixelPacket<36>>& output
) {
    #pragma HLS INLINE off
    
    for(int y = 0; y < FRAME_HEIGHT; ++y) {
        for(int x = 0; x < FRAME_WIDTH; ++x) {
            #pragma HLS LOOP_FLATTEN
            #pragma HLS PIPELINE II=1
            
            ap_axiu<40, 1, 0, 0> axi_pixel = input.read();
            IspPixelPacket<36> pixel;
            pixel.data = axi_pixel.data.range(35, 0);
            pixel.user = axi_pixel.user;
            pixel.last = axi_pixel.last;
            output.write(pixel);
        }
    }
}

static void output_adapter(
    hls::stream<IspPixelPacket<36>>& input,
    hls::stream<ap_axiu<40, 1, 0, 0>>& output
) {
    #pragma HLS INLINE off
    
    for(int y = 0; y < FRAME_HEIGHT; ++y) {
        for(int x = 0; x < FRAME_WIDTH; ++x) {
            #pragma HLS LOOP_FLATTEN
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
    }
}

void isp_ccm_top(
    hls::stream<ap_axiu<40, 1, 0, 0>>& input,
    hls::stream<ap_axiu<40, 1, 0, 0>>& output,
    gain_matrix a00, gain_matrix a01, gain_matrix a02,
    gain_matrix a10, gain_matrix a11, gain_matrix a12,
    gain_matrix a20, gain_matrix a21, gain_matrix a22
) {
    #pragma HLS INTERFACE mode=axis port=input
    #pragma HLS INTERFACE mode=axis port=output
    
    #pragma HLS INTERFACE mode=s_axilite port=a00 bundle=CTRL offset=offset_a00
    #pragma HLS INTERFACE mode=s_axilite port=a01 bundle=CTRL offset=offset_a01
    #pragma HLS INTERFACE mode=s_axilite port=a02 bundle=CTRL offset=offset_a02
    #pragma HLS INTERFACE mode=s_axilite port=a10 bundle=CTRL offset=offset_a10
    #pragma HLS INTERFACE mode=s_axilite port=a11 bundle=CTRL offset=offset_a11
    #pragma HLS INTERFACE mode=s_axilite port=a12 bundle=CTRL offset=offset_a12
    #pragma HLS INTERFACE mode=s_axilite port=a20 bundle=CTRL offset=offset_a20
    #pragma HLS INTERFACE mode=s_axilite port=a21 bundle=CTRL offset=offset_a21
    #pragma HLS INTERFACE mode=s_axilite port=a22 bundle=CTRL offset=offset_a22
    #pragma HLS INTERFACE mode=s_axilite port=return bundle=CTRL

    #pragma HLS DATAFLOW

    hls::stream<IspPixelPacket<36>> input_pixels;
    hls::stream<IspPixelPacket<36>> output_pixels;

    #pragma HLS STREAM variable=input_pixels depth=2
    #pragma HLS STREAM variable=output_pixels depth=2

    input_adapter(input, input_pixels);
    isp_ccm(input_pixels, output_pixels, a00, a01, a02,
                                         a10, a11, a12,
                                         a20, a21, a22);
    output_adapter(output_pixels, output);
}