#include "isp_cores.h"

typedef ap_ufixed<12, 12> ccm_input_t;
typedef ap_ufixed<12, 12, AP_RND, AP_SAT> ccm_output_t;
typedef ap_fixed<30, 18> ccm_accum_t;

void isp_ccm_process(
    hls::stream<rgb36_packet_t> &stream_in,
    hls::stream<rgb36_packet_t> &stream_out,
    ccm_coeff_t a00,
    ccm_coeff_t a01,
    ccm_coeff_t a02,
    ccm_coeff_t a10,
    ccm_coeff_t a11,
    ccm_coeff_t a12,
    ccm_coeff_t a20,
    ccm_coeff_t a21,
    ccm_coeff_t a22,
    frame_count_t frame_count
) {
    #pragma HLS INLINE off

    const unsigned total_pixels =
        (unsigned)frame_count * (unsigned)ISP_FRAME_PIXELS;

CCM_PIXEL_LOOP:
    for (unsigned index = 0; index < total_pixels; ++index) {
        #pragma HLS PIPELINE II=1

        const rgb36_packet_t p_in = stream_in.read();
        const ccm_input_t r_in = rgb_get_r(p_in);
        const ccm_input_t g_in = rgb_get_g(p_in);
        const ccm_input_t b_in = rgb_get_b(p_in);

        const ccm_accum_t r_acc =
            r_in * a00 + g_in * a01 + b_in * a02;
        const ccm_accum_t g_acc =
            r_in * a10 + g_in * a11 + b_in * a12;
        const ccm_accum_t b_acc =
            r_in * a20 + g_in * a21 + b_in * a22;

        const ccm_output_t r_out =
            (r_acc < 0) ? (ccm_output_t)0 : (ccm_output_t)r_acc;
        const ccm_output_t g_out =
            (g_acc < 0) ? (ccm_output_t)0 : (ccm_output_t)g_acc;
        const ccm_output_t b_out =
            (b_acc < 0) ? (ccm_output_t)0 : (ccm_output_t)b_acc;

        rgb36_packet_t p_out;
        p_out.data = 0;
        p_out.user = p_in.user;
        p_out.last = p_in.last;
        rgb_set(
            p_out,
            (ap_uint<12>)r_out,
            (ap_uint<12>)g_out,
            (ap_uint<12>)b_out
        );
        stream_out.write(p_out);
    }
}
