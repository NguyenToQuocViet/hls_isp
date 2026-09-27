#include "isp_3blocks_dataflow_top.h"
#include "isp_axis_adapters.h"
#include "isp_cores.h"

void isp_3blocks_dataflow_top(
    hls::stream<axis_raw10_t> &stream_in,
    hls::stream<axis_rgb36_t> &stream_out,
    wb_gain_t gain_r,
    wb_gain_t gain_gr,
    wb_gain_t gain_gb,
    wb_gain_t gain_b,
    int edge_threshold,
    int edge_mag,
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
    #pragma HLS INTERFACE axis port=stream_in 
    #pragma HLS INTERFACE axis port=stream_out

    #pragma HLS INTERFACE s_axilite port=gain_r         bundle=CTRL offset=0x10
    #pragma HLS INTERFACE s_axilite port=gain_gr        bundle=CTRL offset=0x18
    #pragma HLS INTERFACE s_axilite port=gain_gb        bundle=CTRL offset=0x20
    #pragma HLS INTERFACE s_axilite port=gain_b         bundle=CTRL offset=0x28
    #pragma HLS INTERFACE s_axilite port=edge_threshold bundle=CTRL offset=0x30
    #pragma HLS INTERFACE s_axilite port=edge_mag       bundle=CTRL offset=0x38
    #pragma HLS INTERFACE s_axilite port=a00            bundle=CTRL offset=0x40
    #pragma HLS INTERFACE s_axilite port=a01            bundle=CTRL offset=0x48
    #pragma HLS INTERFACE s_axilite port=a02            bundle=CTRL offset=0x50
    #pragma HLS INTERFACE s_axilite port=a10            bundle=CTRL offset=0x58
    #pragma HLS INTERFACE s_axilite port=a11            bundle=CTRL offset=0x60
    #pragma HLS INTERFACE s_axilite port=a12            bundle=CTRL offset=0x68
    #pragma HLS INTERFACE s_axilite port=a20            bundle=CTRL offset=0x70
    #pragma HLS INTERFACE s_axilite port=a21            bundle=CTRL offset=0x78
    #pragma HLS INTERFACE s_axilite port=a22            bundle=CTRL offset=0x80
    #pragma HLS INTERFACE s_axilite port=frame_count    bundle=CTRL offset=0x88
    #pragma HLS INTERFACE s_axilite port=return         bundle=CTRL
    #pragma HLS DATAFLOW

    hls::stream<raw10_packet_t> raw10_stream("raw10_stream");
    hls::stream<raw12_packet_t> wb_stream("wb_stream");
    hls::stream<rgb36_packet_t> demosaic_stream("demosaic_stream");
    hls::stream<rgb36_packet_t> ccm_stream("ccm_stream");
    #pragma HLS STREAM variable=raw10_stream depth=2
    #pragma HLS STREAM variable=wb_stream depth=2
    #pragma HLS STREAM variable=demosaic_stream depth=2
    #pragma HLS STREAM variable=ccm_stream depth=2

    axis_to_raw10(stream_in, raw10_stream, frame_count);
    isp_wb_process(
        raw10_stream,
        wb_stream,
        gain_r,
        gain_gr,
        gain_gb,
        gain_b,
        frame_count
    );
    isp_demosaic_process(
        wb_stream,
        demosaic_stream,
        edge_threshold,
        edge_mag,
        frame_count
    );
    isp_ccm_process(
        demosaic_stream,
        ccm_stream,
        a00, a01, a02,
        a10, a11, a12,
        a20, a21, a22,
        frame_count
    );
    rgb36_to_axis(ccm_stream, stream_out, frame_count);
}
