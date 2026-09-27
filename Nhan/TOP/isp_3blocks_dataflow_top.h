#ifndef ISP_3BLOCKS_DATAFLOW_TOP_H_
#define ISP_3BLOCKS_DATAFLOW_TOP_H_

#include "isp_types.h"

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
);

#endif
