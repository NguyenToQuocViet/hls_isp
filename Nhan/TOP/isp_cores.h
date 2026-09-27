#ifndef ISP_CORES_H_
#define ISP_CORES_H_

#include "isp_types.h"


void isp_wb_process(
    hls::stream<raw10_packet_t> &stream_in,
    hls::stream<raw12_packet_t> &stream_out,
    wb_gain_t gain_r,
    wb_gain_t gain_gr,
    wb_gain_t gain_gb,
    wb_gain_t gain_b,
    frame_count_t frame_count
);

void isp_demosaic_process(
    hls::stream<raw12_packet_t> &stream_in,
    hls::stream<rgb36_packet_t> &stream_out,
    int edge_threshold,
    int edge_mag,
    frame_count_t frame_count
);

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
);

#endif
