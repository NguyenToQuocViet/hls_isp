#include "isp_wb.h"

using namespace hls;

void isp_wb_top(
    stream<video_in_t> &stream_in,
    stream<video_out_t> &stream_out,
    gain_t gain_r,
    gain_t gain_gr,
    gain_t gain_gb,
    gain_t gain_b
) {
    #pragma HLS INTERFACE axis port=stream_in
    #pragma HLS INTERFACE axis port=stream_out

    // The complete ISP wrapper owns the AXI-Lite registers and must keep
    // these direct ports stable while a frame is being processed.
    #pragma HLS INTERFACE ap_none port=gain_r
    #pragma HLS INTERFACE ap_none port=gain_gr
    #pragma HLS INTERFACE ap_none port=gain_gb
    #pragma HLS INTERFACE ap_none port=gain_b
    #pragma HLS INTERFACE ap_ctrl_none port=return

    // One invocation models one pixel. With ap_ctrl_none the RTL restarts
    // automatically; II=1 permits one input pixel per clock when unstalled.
    #pragma HLS PIPELINE II=1

    static ap_uint<16> col = 0;
    static ap_uint<16> row = 0;
    
    static gain_t current_gain;
    static gain_t frame_gain_r  = 0;
    static gain_t frame_gain_gr = 0;
    static gain_t frame_gain_gb = 0;
    static gain_t frame_gain_b  = 0;

    const video_in_t p_in = stream_in.read();
    const bool sof = (p_in.user != 0);

    const gain_t use_gain_r =
        sof ? gain_r : frame_gain_r;
    const gain_t use_gain_gr =
        sof ? gain_gr : frame_gain_gr;
    const gain_t use_gain_gb =
        sof ? gain_gb : frame_gain_gb;
    const gain_t use_gain_b =
        sof ? gain_b : frame_gain_b;

    if (sof) {
        frame_gain_r  = gain_r;
        frame_gain_gr = gain_gr;
        frame_gain_gb = gain_gb;
        frame_gain_b  = gain_b;
    }

    // TUSER marks the first pixel of every frame and re-synchronizes the
    // Bayer phase even when multiple frames arrive without resetting the IP.
    if (p_in.user) {
        col = 0;
        row = 0;
    }

    const bool is_col_even = ((col.to_uint() & 1u) == 0u);
    const bool is_row_even = ((row.to_uint() & 1u) == 0u);

    if (is_row_even && is_col_even) {
        current_gain = use_gain_r;
    } else if (is_row_even && !is_col_even) {
        current_gain = use_gain_gr;
    } else if (!is_row_even && is_col_even) {
        current_gain = use_gain_gb;
    } else {
        current_gain = use_gain_b;
    }

    const i_pixel input_pixel = axis_get_input_pixel(p_in);
    const wb_product_t product = input_pixel * current_gain;
    const o_pixel output_pixel = (o_pixel)product;

    video_out_t p_out;
    p_out.data = 0;
    p_out.keep = p_in.keep;
    p_out.strb = p_in.strb;
    p_out.user = p_in.user;
    p_out.last = p_in.last;
    p_out.id   = p_in.id;
    p_out.dest = p_in.dest;
    axis_set_output_pixel(p_out, (ap_uint<12>)output_pixel);
    stream_out.write(p_out);

    // Use the incoming EOL marker instead of a compile-time frame width.
    if (p_in.last) {
        col = 0;
        row = row + 1;
    } else {
        col = col + 1;
    }
}
