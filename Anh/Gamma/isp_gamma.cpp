#include "../ltm_gamma.hpp"
#include "gamma_lut.h"

void isp_gamma_top(hls::stream<gamma_in_t>& stream_in,
                      hls::stream<gamma_out_t>& stream_out)
{
#pragma HLS INTERFACE axis port = stream_in
#pragma HLS INTERFACE axis port = stream_out
#pragma HLS INTERFACE s_axilite port = return bundle = CTRL
#pragma HLS BIND_STORAGE variable=GAMMA_LUT impl=bram

    for (int i = 0; i < HEIGHT; i++)
    {
        for (int j = 0; j < WIDTH; j++)
        {
#pragma HLS PIPELINE II = 1

            gamma_in_t pixel_in;
            gamma_out_t pixel_out;

            pixel_in = stream_in.read();

            // Unpack RGB channels
            ap_uint<12> r_in = pixel_in.data.range(11, 0);
            ap_uint<12> g_in = pixel_in.data.range(23, 12);
            ap_uint<12> b_in = pixel_in.data.range(35, 24);

            // Gamma
            ap_uint<8> r_out = GAMMA_LUT[r_in];
            ap_uint<8> g_out = GAMMA_LUT[g_in];
            ap_uint<8> b_out = GAMMA_LUT[b_in];

            // Pack RGB channels
            pixel_out.data.range(7, 0) = r_out;
            pixel_out.data.range(15, 8) = g_out;
            pixel_out.data.range(23, 16) = b_out;

            pixel_out.last = pixel_in.last;
            pixel_out.user = pixel_in.user;
#ifdef USE_AP_AXIU
            pixel_out.keep = pixel_in.keep;
            pixel_out.strb = pixel_in.strb;
#endif

            stream_out.write(pixel_out);
        }
    }
}
#ifdef VER2

void isp_gamma_task(hls::stream<gamma_in_t>& stream_in,
                      hls::stream<gamma_out_t>& stream_out)
{
#pragma HLS PIPELINE II = 1
#pragma HLS BIND_STORAGE variable=GAMMA_LUT impl=bram

    gamma_in_t pixel_in;
    gamma_out_t pixel_out;

    pixel_in = stream_in.read();

    // Unpack RGB channels
    ap_uint<12> r_in = pixel_in.data.range(11, 0);
    ap_uint<12> g_in = pixel_in.data.range(23, 12);
    ap_uint<12> b_in = pixel_in.data.range(35, 24);

    // Gamma
    ap_uint<8> r_out = GAMMA_LUT[r_in];
    ap_uint<8> g_out = GAMMA_LUT[g_in];
    ap_uint<8> b_out = GAMMA_LUT[b_in];

    // Pack RGB channels
    pixel_out.data.range(7, 0) = r_out;
    pixel_out.data.range(15, 8) = g_out;
    pixel_out.data.range(23, 16) = b_out;

    pixel_out.last = pixel_in.last;
    pixel_out.user = pixel_in.user;
#ifdef USE_AP_AXIU
    pixel_out.keep = pixel_in.keep;
    pixel_out.strb = pixel_in.strb;
#endif

    stream_out.write(pixel_out);
}

void isp_gamma_top_ver2(hls::stream<gamma_in_t>& stream_in,
                      hls::stream<gamma_out_t>& stream_out)
{
#pragma HLS INTERFACE axis port = stream_in
#pragma HLS INTERFACE axis port = stream_out
#pragma HLS INTERFACE ap_ctrl_none port = return

    hls_thread_local hls::task t(isp_gamma_task, stream_in, stream_out);
}
#endif