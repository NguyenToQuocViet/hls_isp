#include "isp_cnn_denoise.h"
#include "isp_cnn_denoise_params.h"


static void tail_group_accum(hls::stream<feat16_window_t>& win_in, hls::stream<acc4_pkt_t>& acc_out) {
#pragma HLS ARRAY_PARTITION variable=L9_WBANK complete dim=1
#pragma HLS ARRAY_PARTITION variable=L9_WBANK cyclic factor=4 dim=2
#pragma HLS ARRAY_PARTITION variable=L9_BIAS complete

    acc_t group_sum[CNN_GROUPS][4];
#pragma HLS ARRAY_PARTITION variable=group_sum complete dim=0
    feat16_window_t current;

TAIL_GROUP:
for (int n = 0; n < CNN_PIXELS * CNN_GROUPS; ++n) {
#pragma HLS PIPELINE II=1
        const int g = n & 3;
        if (g == 0) current = win_in.read();

        acc_t delta[4];
#pragma HLS ARRAY_PARTITION variable=delta complete

        for (int oc = 0; oc < 4; ++oc) {
#pragma HLS UNROLL
            acc_t sum = 0;

            for (int li = 0; li < CNN_PAR_IN; ++li) {
#pragma HLS UNROLL
                const int ic = g * CNN_PAR_IN + li;

                for (int k = 0; k < 9; ++k) {
#pragma HLS UNROLL
                    const int tap_idx = k*8;
                    feature_t xv = bits_to_feature(current.taps[ic].range(tap_idx+7,tap_idx));
                    tail_weight_word_t ww = L9_WBANK[k][ic];
                    feature_t wv = bits_to_feature(ww.range(oc*8+7,oc*8));
                    sum += (acc_t)((ap_int<16>)xv * (ap_int<16>)wv);
                }
            }

            delta[oc] = sum;
            group_sum[g][oc] = sum;
        }

        if (g == 3) {
            acc4_pkt_t a;
            a.data = 0; a.user = (current.y == 0 && current.x == 0) ? 1 : 0; a.last = (current.x == CNN_PACK_W - 1) ? 1 : 0;

            for (int oc = 0; oc < 4; ++oc) {
#pragma HLS UNROLL
                acc_t v = L9_BIAS[oc] + group_sum[0][oc] + group_sum[1][oc] + group_sum[2][oc] + delta[oc];
                a.data.range(oc*32+31,oc*32) = v.range(31,0);
            }

            acc_out.write(a);
        }
    }
}

static void tail_add_global_skip(hls::stream<acc4_pkt_t>& main_acc, hls::stream<IspPixelPacket<32>>& global_skip, hls::stream<IspPixelPacket<32>>& out) {
#pragma HLS ARRAY_PARTITION variable=L9_TAIL_Q31 complete
#pragma HLS ARRAY_PARTITION variable=L9_TAIL_EXP complete

TAIL_ADD:
for (int p = 0; p < CNN_PIXELS; ++p) {
#pragma HLS PIPELINE II=1
        acc4_pkt_t a = main_acc.read();
        IspPixelPacket<32> s = global_skip.read();

        IspPixelPacket<32> o;
        o.data = 0;

        for (int c = 0; c < 4; ++c) {
#pragma HLS UNROLL
            acc_t av = (acc_t)a.data.range(c*32+31,c*32);
            acc_t main_units = q31_round(av,L9_TAIL_Q31[c],L9_TAIL_EXP[c]);

            feature_t sx = bits_to_feature(s.data.range(c*8+7,c*8));
            acc_t skip_units = q31_round((acc_t)sx, L9_GLOBAL_SKIP_Q31, L9_GLOBAL_SKIP_EXP);

            feature_t q = sat8(main_units + skip_units);
            o.data.range(c*8+7,c*8) = feature_to_bits(q);
        }

        o.user = a.user; o.last = a.last;
        out.write(o);
    }
}

void cnn_l9_tail(hls::stream<IspPixelPacket<128>>& in, hls::stream<IspPixelPacket<32>>& global_skip, hls::stream<IspPixelPacket<32>>& packed_out) {
    hls::stream<feat16_window_t> win("tail_win");
    hls::stream<acc4_pkt_t> acc("tail_acc");
#pragma HLS STREAM variable=win depth=16
#pragma HLS STREAM variable=acc depth=32
#pragma HLS DATAFLOW

    feature16_to_windows_core(in, win, 9);
    tail_group_accum(win,acc);
    tail_add_global_skip(acc,global_skip,packed_out);
}

static ap_uint<10> feature_to_raw10(feature_t q) {
#pragma HLS INLINE
    acc_t raw = q31_round((acc_t)q,L9_OUT_RAW_Q31,L9_OUT_RAW_EXP);
    if (raw < 0) raw = 0; 
    if (raw > L0_POST_BLC_MAX) raw = L0_POST_BLC_MAX;
    return (ap_uint<10>)raw;
}

static ap_uint<40> pack_raw_cell(IspPixelPacket<32> p) {
#pragma HLS INLINE
    ap_uint<40> w = 0;
    for (int c = 0; c < 4; ++c) {
#pragma HLS UNROLL
        feature_t q = bits_to_feature(p.data.range(c*8+7,c*8));
        ap_uint<10> r = feature_to_raw10(q);
        w.range(c*10+9,c*10) = r;
    }
    return w;
}

void packed4_to_raw10(hls::stream<IspPixelPacket<32>>& packed_in, hls::stream<IspPixelPacket<10>>& raw_out) {
    ap_uint<40> rowbuf[2][CNN_PACK_W];
#pragma HLS ARRAY_PARTITION variable=rowbuf complete dim=1
#pragma HLS BIND_STORAGE variable=rowbuf type=ram_1p impl=bram

    // Fill first packed row.
FIRST_FILL:
for (int x = 0; x < CNN_PACK_W; ++x) {
#pragma HLS PIPELINE II=1
        IspPixelPacket<32> p = packed_in.read();
        rowbuf[0][x] = pack_raw_cell(p);
    }

PACKED_ROW:
for (int py = 0; py < CNN_PACK_H; ++py) {
        const int active = py & 1, fill = active ^ 1; 
        int fill_x = 0;

RAW_ROW_PAIR:
for (int phase = 0; phase < CNN_RAW_W * 2; ++phase) {
#pragma HLS PIPELINE II=1
            int local = (phase < CNN_RAW_W) ? phase : (phase - CNN_RAW_W);
            int px = local >> 1;
            int c;

            if (phase < CNN_RAW_W)
                c = (local & 1) ? 1 : 0; // R / Gr
            else
                c = (local & 1) ? 3 : 2; // Gb / B

            ap_uint<40> cell = rowbuf[active][px];

            IspPixelPacket<10> o;
            o.data = cell.range(c*10+9,c*10);
            o.user = (py == 0 && phase == 0) ? 1 : 0; 
            o.last = ((phase == CNN_RAW_W-1) || (phase == CNN_RAW_W*2-1)) ? 1 : 0;
            raw_out.write(o);

            if ((py + 1 < CNN_PACK_H) && ((phase & 3) == 3)) {
                IspPixelPacket<32> np = packed_in.read();
                rowbuf[fill][fill_x] = pack_raw_cell(np);
                ++fill_x;
            }
        }
    }
}
