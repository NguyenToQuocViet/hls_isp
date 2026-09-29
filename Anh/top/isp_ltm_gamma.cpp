#include "../ltm_gamma.hpp"

void axis2isp(hls::stream<ap_axiu<36, 1, 0, 0>>& axis_in, hls::stream<IspPixelPacket<36>>& isp_out) {
    for (int i = 0; i < HEIGHT * WIDTH; i++) {
#pragma HLS PIPELINE II=1
        ap_axiu<36, 1, 0, 0> in = axis_in.read();
        IspPixelPacket<36> out;
        out.data = in.data;
        out.user = in.user;
        out.last = in.last;
        isp_out.write(out);
    }
}

void isp2axis(hls::stream<IspPixelPacket<24>>& isp_in, hls::stream<ap_axiu<24, 1, 0, 0>>& axis_out) {
    for (int i = 0; i < HEIGHT * WIDTH; i++) {
#pragma HLS PIPELINE II=1
        IspPixelPacket<24> in = isp_in.read();
        ap_axiu<24, 1, 0, 0> out;
        out.data = in.data;
        out.user = in.user;
        out.last = in.last;
        axis_out.write(out);
    }
}

#ifndef USE_AP_AXIU
void isp_ltm_gamma_top(
    hls::stream<ap_axiu<36, 1, 0, 0>>& s_axis,
    hls::stream<ap_axiu<24, 1, 0, 0>>& m_axis
)
{
#pragma HLS INTERFACE axis port=s_axis
#pragma HLS INTERFACE axis port=m_axis
#pragma HLS INTERFACE s_axilite port=return bundle=CTRL

#pragma HLS DATAFLOW

    hls::stream<IspPixelPacket<36>> isp_stream_in("isp_stream_in");
    hls::stream<IspPixelPacket<36>> isp_stream_ltm_out("isp_stream_ltm_out");
    hls::stream<IspPixelPacket<24>> isp_stream_gamma_out("isp_stream_gamma_out");

    axis2isp(s_axis, isp_stream_in);
    
    isp_ltm_top(isp_stream_in, isp_stream_ltm_out);
    isp_gamma_top(isp_stream_ltm_out, isp_stream_gamma_out);

    isp2axis(isp_stream_gamma_out, m_axis);
}
#endif
