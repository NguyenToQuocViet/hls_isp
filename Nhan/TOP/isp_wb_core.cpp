#include "isp_cores.h"

typedef ap_ufixed<12, 12, AP_RND, AP_SAT> wb_output_t;
typedef ap_ufixed<28, 16> wb_product_t;

void isp_wb_process(
    hls::stream<raw10_packet_t> &stream_in,
    hls::stream<raw12_packet_t> &stream_out,
    wb_gain_t gain_r,
    wb_gain_t gain_gr,
    wb_gain_t gain_gb,
    wb_gain_t gain_b,
    frame_count_t frame_count
) {
    #pragma HLS INLINE off

    const unsigned total_pixels =
        (unsigned)frame_count * (unsigned)ISP_FRAME_PIXELS;

    ap_uint<16> row = 0;
    ap_uint<16> col = 0;

    wb_gain_t frame_gain_r  = 0;
    wb_gain_t frame_gain_gr = 0;
    wb_gain_t frame_gain_gb = 0;
    wb_gain_t frame_gain_b  = 0;

WB_PIXEL_LOOP:
    for (unsigned index = 0; index < total_pixels; ++index) {
        #pragma HLS PIPELINE II=1

        const raw10_packet_t p_in = stream_in.read();
        const bool sof = (p_in.user != 0);

        const wb_gain_t use_gain_r  = sof ? gain_r  : frame_gain_r;
        const wb_gain_t use_gain_gr = sof ? gain_gr : frame_gain_gr;
        const wb_gain_t use_gain_gb = sof ? gain_gb : frame_gain_gb;
        const wb_gain_t use_gain_b  = sof ? gain_b  : frame_gain_b;

        if (sof) {
            frame_gain_r  = gain_r;
            frame_gain_gr = gain_gr;
            frame_gain_gb = gain_gb;
            frame_gain_b  = gain_b;
        }

        if (sof) {
            row = 0;
            col = 0;
        }

        wb_gain_t selected_gain;
        const bool row_even = ((row & 1) == 0);
        const bool col_even = ((col & 1) == 0);

        if (row_even && col_even) {
            selected_gain = use_gain_r;
        } else if (row_even) {
            selected_gain = use_gain_gr;
        } else if (col_even) {
            selected_gain = use_gain_gb;
        } else {
            selected_gain = use_gain_b;
        }
        
        const wb_scaled_input_t scaled =
            ((wb_scaled_input_t)p_in.data) << 2;

        const wb_product_t product =
            scaled * selected_gain;
        const wb_output_t result = (wb_output_t)product;

        raw12_packet_t p_out;
        p_out.data = (raw12_t)result;
        p_out.user = p_in.user;
        p_out.last = p_in.last;
        stream_out.write(p_out);

        if (p_in.last) {
            col = 0;
            if (row == ISP_FRAME_HEIGHT - 1) {
                row = 0;
            } else {
                row = row + 1;
            }
        } else {
            col = col + 1;
        }
    }
}
