#include "isp_wb_top.hpp"
using namespace hls;

void isp_wb(
    stream<IspPixelPacket<10>>& stream_in,
    stream<IspPixelPacket<12>>& stream_out,
    gain_t gain_r,
    gain_t gain_gr,
    gain_t gain_gb,
    gain_t gain_b
) {
    #pragma HLS INLINE off
    
    for(int y = 0; y < FRAME_HEIGHT; ++y) {
        for(int x = 0; x < FRAME_WIDTH; ++x) {
            #pragma HLS LOOP_FLATTEN
            #pragma HLS PIPELINE II=1
            
            bool is_col_even = ((x & 1u) == 0u);
            bool is_row_even = ((y & 1u) == 0u);
            gain_t current_gain;
            IspPixelPacket<10> p_in = stream_in.read();
            
            if(is_col_even && is_row_even)
                current_gain = gain_r;
            else if(!is_col_even && is_row_even)
                current_gain = gain_gr;
            else if(is_col_even && !is_row_even)
                current_gain = gain_gb;
            else current_gain = gain_b;

            ap_uint<10> input_pixel = p_in.data;

            ap_uint<12> input_pixel_12 = ((ap_uint<12>)input_pixel) << 2;

            const ap_ufixed<28,16> product = input_pixel_12 * current_gain;

            const o_pixel output_pixel = (o_pixel)product;

            IspPixelPacket<12> p_out;
            p_out.data = output_pixel;
            p_out.user = p_in.user;
            p_out.last = p_in.last;
            stream_out.write(p_out);           
        }
    }
}   

static void input_adapter(
    hls::stream<ap_axiu<16, 1, 0, 0>>& input,
    hls::stream<IspPixelPacket<10>>& output
) {
    #pragma HLS INLINE off
    
    for(int y = 0; y < FRAME_HEIGHT; ++y) {
        for(int x = 0; x < FRAME_WIDTH; ++x) {
            #pragma HLS LOOP_FLATTEN
            #pragma HLS PIPELINE II=1
            ap_axiu<16, 1, 0, 0> axi_pixel = input.read();
            IspPixelPacket<10> pixel;
            pixel.data = axi_pixel.data.range(9, 0);
            pixel.user = axi_pixel.user;
            pixel.last = axi_pixel.last;
            output.write(pixel);
        }
    }
    
}

static void output_adapter(
    hls::stream<IspPixelPacket<12>>& input,
    hls::stream<ap_axiu<16, 1, 0, 0>>& output
) {
    #pragma HLS INLINE off
    
    for(int y = 0; y < FRAME_HEIGHT; ++y) {
        for(int x = 0; x < FRAME_WIDTH; ++x) {
            #pragma HLS LOOP_FLATTEN
            #pragma HLS PIPELINE II=1
            IspPixelPacket<12> pixel = input.read();
            ap_axiu<16, 1, 0, 0> axi_pixel;
            axi_pixel.data = 0;
            axi_pixel.data.range(11, 0) = pixel.data;
            axi_pixel.keep = -1;
            axi_pixel.strb = -1;
            axi_pixel.user = pixel.user;
            axi_pixel.last = pixel.last;
            output.write(axi_pixel);
        }
    }
}

void isp_wb_top(
    hls::stream<ap_axiu<16, 1, 0, 0>>& input,
    hls::stream<ap_axiu<16, 1, 0, 0>>& output,
    gain_t gain_r,
    gain_t gain_gr,
    gain_t gain_gb,
    gain_t gain_b
) {
    #pragma HLS INTERFACE mode=axis port=input
    #pragma HLS INTERFACE mode=axis port=output
    
    #pragma HLS INTERFACE mode=s_axilite port=gain_r bundle=CTRL offset=offset_gain_r
    #pragma HLS INTERFACE mode=s_axilite port=gain_gr bundle=CTRL offset=offset_gain_gr
    #pragma HLS INTERFACE mode=s_axilite port=gain_gb bundle=CTRL offset=offset_gain_gb
    #pragma HLS INTERFACE mode=s_axilite port=gain_b bundle=CTRL offset=offset_gain_b
    #pragma HLS INTERFACE mode=s_axilite port=return bundle=CTRL

    #pragma HLS DATAFLOW

    hls::stream<IspPixelPacket<10>> input_pixels;
    hls::stream<IspPixelPacket<12>> output_pixels;

    #pragma HLS STREAM variable=input_pixels depth=2
    #pragma HLS STREAM variable=output_pixels depth=2
    
    input_adapter(input, input_pixels);
    isp_wb(input_pixels, output_pixels, gain_r, gain_gr, gain_gb, gain_b);
    output_adapter(output_pixels, output);
}