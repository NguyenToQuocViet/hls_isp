#include "isp_axis_adapters.h"

static unsigned transaction_pixels(frame_count_t frame_count) {
    #pragma HLS INLINE
    return (unsigned)frame_count * (unsigned)ISP_FRAME_PIXELS;
}

void axis_to_raw10(
    hls::stream<axis_raw10_t> &axis_in,
    hls::stream<raw10_packet_t> &internal_out,
    frame_count_t frame_count
) {
    #pragma HLS INLINE off
    const unsigned total_pixels = transaction_pixels(frame_count);

AXIS_TO_RAW10_LOOP:
    for (unsigned i = 0; i < total_pixels; ++i) {
        #pragma HLS PIPELINE II=1
        const axis_raw10_t a = axis_in.read();
        raw10_packet_t p;
        p.data = a.data;
        p.user = a.user;
        p.last = a.last;
        internal_out.write(p);
    }
}

void rgb36_to_axis(
    hls::stream<rgb36_packet_t> &internal_in,
    hls::stream<axis_rgb36_t> &axis_out,
    frame_count_t frame_count
) {
    #pragma HLS INLINE off
    const unsigned total_pixels = transaction_pixels(frame_count);

RGB36_TO_AXIS_LOOP:
    for (unsigned i = 0; i < total_pixels; ++i) {
        #pragma HLS PIPELINE II=1
        const rgb36_packet_t p = internal_in.read();
        axis_rgb36_t a;
        a.data = p.data;
        a.keep = -1;
        a.strb = -1;
        a.user = p.user;
        a.last = p.last;
        a.id = 0;
        a.dest = 0;
        axis_out.write(a);
    }
}
