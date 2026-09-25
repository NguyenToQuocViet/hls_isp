#include "../LTM_Gamma.hpp"
#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <hls_stream.h>
#include <iostream>
using namespace std;

// Use built-in AXI-Stream structs
// ap_axiu <DataWidth, UserWidth, DestWidth, IdWidth>

// Module signature
void gamma_correction(hls::stream<IspPixelPacket<36>>& stream_in,
                      hls::stream<IspPixelPacket<24>>& stream_out,
                      const ap_uint<8> gamma_lut_r[4096],
                      const ap_uint<8> gamma_lut_g[4096],
                      const ap_uint<8> gamma_lut_b[4096])
{
#pragma HLS INTERFACE axis port = stream_in
#pragma HLS INTERFACE axis port = stream_out
#pragma HLS INTERFACE s_axilite port = gamma_lut_r bundle = CTRL
#pragma HLS INTERFACE s_axilite port = gamma_lut_g bundle = CTRL
#pragma HLS INTERFACE s_axilite port = gamma_lut_b bundle = CTRL
#pragma HLS INTERFACE s_axilite port = return bundle = CTRL
    for (int i = 0; i < HEIGHT; i++)
    {
        for (int j = 0; j < WIDTH; j++)
        {
#pragma HLS PIPELINE II = 1

            IspPixelPacket<36> pixel_in;
            IspPixelPacket<24> pixel_out;

            pixel_in = stream_in.read();

            // Unpack RGB channels
            ap_uint<12> r_in = pixel_in.data.range(11, 0);
            ap_uint<12> g_in = pixel_in.data.range(23, 12);
            ap_uint<12> b_in = pixel_in.data.range(35, 24);

            // Gamma
            ap_uint<8> r_out = gamma_lut_r[r_in];
            ap_uint<8> g_out = gamma_lut_g[g_in];
            ap_uint<8> b_out = gamma_lut_b[b_in];

            // Pack RGB channels
            pixel_out.data.range(7, 0) = r_out;
            pixel_out.data.range(15, 8) = g_out;
            pixel_out.data.range(23, 16) = b_out;

            pixel_out.last = pixel_in.last;
            pixel_out.user = pixel_in.user;

            stream_out.write(pixel_out);
        }
    }
}