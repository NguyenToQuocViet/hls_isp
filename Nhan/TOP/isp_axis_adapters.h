#ifndef ISP_AXIS_ADAPTERS_H_
#define ISP_AXIS_ADAPTERS_H_

#include "isp_types.h"

void axis_to_raw10(
    hls::stream<axis_raw10_t> &axis_in,
    hls::stream<raw10_packet_t> &internal_out,
    frame_count_t frame_count
);

void rgb36_to_axis(
    hls::stream<rgb36_packet_t> &internal_in,
    hls::stream<axis_rgb36_t> &axis_out,
    frame_count_t frame_count
);

#endif
