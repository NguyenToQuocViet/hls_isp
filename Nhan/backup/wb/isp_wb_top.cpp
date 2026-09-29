#include "isp_wb_top.hpp"
#include <hls_task.h>

using namespace hls;

void isp_wb(
    stream<IspPixelPacket<10>>& stream_in,
    stream<IspPixelPacket<12>>& stream_out
) {
    #pragma HLS PIPELINE II=1

    static ap_uint<16> col = 0;
    static ap_uint<16> row = 0;
    #pragma HLS RESET variable=col
    #pragma HLS RESET variable=row
    
    gain_t current_gain;
    static const gain_t frame_gain_r  = 1.512804;
    static const gain_t frame_gain_gr = 1;
    static const gain_t frame_gain_gb = 1;
    static const gain_t frame_gain_b  = 1.916974;

    const IspPixelPacket<10> p_in = stream_in.read();

    if (p_in.user) {
        col = 0;
        row = 0;
    }

    const bool is_col_even = ((col.to_uint() & 1u) == 0u);
    const bool is_row_even = ((row.to_uint() & 1u) == 0u);

    if (is_row_even && is_col_even) {
        current_gain = frame_gain_r;
    } else if (is_row_even && !is_col_even) {
        current_gain = frame_gain_gr;
    } else if (!is_row_even && is_col_even) {
        current_gain = frame_gain_gb;
    } else {
        current_gain = frame_gain_b;
    }
    const ap_uint<10> input_pixel = p_in.data;

    const ap_uint<12> input_pixel_12 = ((ap_uint<12>)input_pixel) << 2;

    const ap_ufixed<28,16> product = input_pixel_12 * current_gain;

    const o_pixel output_pixel = (o_pixel)product;

    IspPixelPacket<12> p_out;
    p_out.data = output_pixel;
    p_out.user = p_in.user;
    p_out.last = p_in.last;
    stream_out.write(p_out);

    if (p_in.last) {
        col = 0;
        row = row + 1;
    } else {
        col = col + 1;
    }
}

static void input_adapter(
    hls::stream<ap_axiu<16, 1, 0, 0>>& input,
    hls::stream<IspPixelPacket<10>>& output
) {
    #pragma HLS PIPELINE II=1
    ap_axiu<16, 1, 0, 0> axi_pixel = input.read();
    IspPixelPacket<10> pixel;
    pixel.data = axi_pixel.data.range(9, 0);
    pixel.user = axi_pixel.user;
    pixel.last = axi_pixel.last;
    output.write(pixel);
}

static void output_adapter(
    hls::stream<IspPixelPacket<12>>& input,
    hls::stream<ap_axiu<16, 1, 0, 0>>& output
) {
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

void isp_wb_top(
    hls::stream<ap_axiu<16, 1, 0, 0>>& input,
    hls::stream<ap_axiu<16, 1, 0, 0>>& output
) {
    #pragma HLS INTERFACE mode=axis port=input
    #pragma HLS INTERFACE mode=axis port=output
    #pragma HLS INTERFACE mode=ap_ctrl_none port=return

    hls_thread_local hls::stream<IspPixelPacket<10>> input_pixels;
    hls_thread_local hls::stream<IspPixelPacket<12>> output_pixels;

    hls_thread_local hls::task ingress_task(input_adapter, input, input_pixels);
    hls_thread_local hls::task wb_task(isp_wb, input_pixels, output_pixels);
    hls_thread_local hls::task egress_task(output_adapter, output_pixels, output);
}