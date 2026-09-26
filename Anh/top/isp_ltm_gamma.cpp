#include "../ltm_gamma.hpp"

void axis2isp(hls::stream<axis_pixel_36b>& axis_in, hls::stream<IspPixelPacket<36>>& isp_out) {
    for (int i = 0; i < HEIGHT * WIDTH; i++) {
#pragma HLS PIPELINE II=1
        axis_pixel_36b in = axis_in.read();
        IspPixelPacket<36> out;
        out.data = in.data;
        out.user = in.user;
        out.last = in.last;
        isp_out.write(out);
    }
}

void isp2axis(hls::stream<IspPixelPacket<24>>& isp_in, hls::stream<axis_pixel_24b>& axis_out) {
    for (int i = 0; i < HEIGHT * WIDTH; i++) {
#pragma HLS PIPELINE II=1
        IspPixelPacket<24> in = isp_in.read();
        axis_pixel_24b out;
        out.data = in.data;
        out.user = in.user;
        out.last = in.last;
        axis_out.write(out);
    }
}

void isp_ltm_gamma_top(
    hls::stream<axis_pixel_36b>& s_axis,
    hls::stream<axis_pixel_24b>& m_axis,
    const log_t log_lut[4096],
    const log_t reinhard_lut[1024],
    const exp_out_t exp_lut[1024],
    const ap_uint<8> gamma_lut_r[4096],
    const ap_uint<8> gamma_lut_g[4096],
    const ap_uint<8> gamma_lut_b[4096]
)
{
#pragma HLS INTERFACE axis port=s_axis
#pragma HLS INTERFACE axis port=m_axis
#pragma HLS INTERFACE s_axilite port=return bundle=CTRL
#pragma HLS INTERFACE s_axilite port=log_lut bundle=CTRL
#pragma HLS INTERFACE s_axilite port=reinhard_lut bundle=CTRL
#pragma HLS INTERFACE s_axilite port=exp_lut bundle=CTRL
#pragma HLS INTERFACE s_axilite port=gamma_lut_r bundle=CTRL
#pragma HLS INTERFACE s_axilite port=gamma_lut_g bundle=CTRL
#pragma HLS INTERFACE s_axilite port=gamma_lut_b bundle=CTRL

#pragma HLS DATAFLOW

    hls::stream<IspPixelPacket<36>> isp_stream_in("isp_stream_in");
    hls::stream<IspPixelPacket<36>> isp_stream_ltm_out("isp_stream_ltm_out");
    hls::stream<IspPixelPacket<24>> isp_stream_gamma_out("isp_stream_gamma_out");

    axis2isp(s_axis, isp_stream_in);
    
    isp_ltm_top(isp_stream_in, isp_stream_ltm_out, log_lut, reinhard_lut, exp_lut);
    isp_gamma_top(isp_stream_ltm_out, isp_stream_gamma_out, gamma_lut_r, gamma_lut_g, gamma_lut_b);

    isp2axis(isp_stream_gamma_out, m_axis);
}
