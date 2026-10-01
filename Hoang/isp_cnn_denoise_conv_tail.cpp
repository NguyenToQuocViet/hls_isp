#include "isp_cnn_denoise.h"
#include "isp_cnn_denoise_params.h"

static void tail_group_accum(hls::stream<feat16_window_t> &win_in, hls::stream<acc4_pkt_t> &acc_out)
{
#pragma HLS ARRAY_PARTITION variable = L5_WBANK complete dim = 1
#pragma HLS ARRAY_PARTITION variable = L5_WBANK cyclic factor = 4 dim = 2
#pragma HLS ARRAY_PARTITION variable = L5_BIAS complete
    acc_t group_sum[CNN_GROUPS][4];
#pragma HLS ARRAY_PARTITION variable = group_sum complete dim = 0
    feat16_window_t current;
TAIL_GROUP:
    for (int n = 0; n < CNN_PIXELS * CNN_GROUPS; ++n)
    {
#pragma HLS PIPELINE II = 1
        const int g = n & 3;
        if (g == 0)
            current = win_in.read();
        acc_t delta[4];
#pragma HLS ARRAY_PARTITION variable = delta complete
        for (int oc = 0; oc < 4; ++oc)
        {
#pragma HLS UNROLL
            acc_t sum = 0;
            for (int li = 0; li < CNN_PAR_IN; ++li)
            {
#pragma HLS UNROLL
                const int ic = g * CNN_PAR_IN + li;
                for (int k = 0; k < 9; ++k)
                {
#pragma HLS UNROLL
                    ap_uint<8> xb = current.taps[ic].range(k * 8 + 7, k * 8);
                    ap_int<16> xv = (ap_int<16>)(ap_int<8>)xb;
                    tail_weight_word_t ww = L5_WBANK[k][ic];
                    weight_t wv = (weight_t)ww.range(oc * 8 + 7, oc * 8);
                    ap_int<16> prod = xv * (ap_int<16>)wv;
                    sum += (acc_t)prod;
                }
            }
            delta[oc] = sum;
            group_sum[g][oc] = sum;
        }
        if (g == 3)
        {
            acc4_pkt_t a;
            a.data = 0;
            a.user = (current.y == 0 && current.x == 0) ? 1 : 0;
            a.last = (current.x == CNN_PACK_W - 1) ? 1 : 0;
            for (int oc = 0; oc < 4; ++oc)
            {
#pragma HLS UNROLL
                acc_t v = L5_BIAS[oc] + group_sum[0][oc] + group_sum[1][oc] + group_sum[2][oc] + delta[oc];
                a.data.range(oc * 32 + 31, oc * 32) = v.range(31, 0);
            }
            acc_out.write(a);
        }
    }
}

static void tail_add_exact_raw_skip(hls::stream<acc4_pkt_t> &main_acc, hls::stream<IspPixelPacket<40>> &global_skip, hls::stream<IspPixelPacket<40>> &out)
{
#pragma HLS ARRAY_PARTITION variable = L5_TAIL_TO_RAW_DN_Q31 complete
#pragma HLS ARRAY_PARTITION variable = L5_TAIL_TO_RAW_DN_EXP complete
TAIL_ADD:
    for (int p = 0; p < CNN_PIXELS; ++p)
    {
#pragma HLS PIPELINE II = 1
        acc4_pkt_t a = main_acc.read();
        IspPixelPacket<40> s = global_skip.read();
        IspPixelPacket<40> o;
        o.data = 0;
        o.user = a.user;
        o.last = a.last;
        for (int c = 0; c < 4; ++c)
        {
#pragma HLS UNROLL
            acc_t av = (acc_t)a.data.range(c * 32 + 31, c * 32);
            acc_t residual_dn = q31_round(av, L5_TAIL_TO_RAW_DN_Q31[c], L5_TAIL_TO_RAW_DN_EXP[c]);
            acc_t raw_skip_dn = (acc_t)(ap_uint<10>)s.data.range(c * 10 + 9, c * 10);
            acc_t raw_sum_dn = raw_skip_dn + residual_dn;
            acc_t raw_clamped_dn = (raw_sum_dn < 0) ? (acc_t)0 : ((raw_sum_dn > L0_POST_BLC_MAX) ? (acc_t)L0_POST_BLC_MAX : raw_sum_dn);
            o.data.range(c * 10 + 9, c * 10) = (ap_uint<10>)raw_clamped_dn;
        }
        out.write(o);
    }
}

void cnn_l5_tail(hls::stream<IspPixelPacket<128>> &in, hls::stream<IspPixelPacket<40>> &global_skip, hls::stream<IspPixelPacket<40>> &packed_out)
{
    hls::stream<feat16_window_t> win("tail_win");
    hls::stream<acc4_pkt_t> acc("tail_acc");
#pragma HLS STREAM variable = win depth = 16
#pragma HLS STREAM variable = acc depth = 32
#pragma HLS DATAFLOW
    feature16_to_windows_core(in, win, 5);
    tail_group_accum(win, acc);
    tail_add_exact_raw_skip(acc, global_skip, packed_out);
}

static ap_uint<40> pack_raw_cell(IspPixelPacket<40> p)
{
#pragma HLS INLINE
    return p.data;
}

void packed4_to_raw10(hls::stream<IspPixelPacket<40>> &packed_in, hls::stream<IspPixelPacket<10>> &raw_out)
{
    ap_uint<40> rowbuf[2][CNN_PACK_W];
#pragma HLS ARRAY_PARTITION variable = rowbuf complete dim = 1
#pragma HLS BIND_STORAGE variable = rowbuf type = ram_1p impl = bram
FIRST_FILL:
    for (int x = 0; x < CNN_PACK_W; ++x)
    {
#pragma HLS PIPELINE II = 1
        IspPixelPacket<40> p = packed_in.read();
        rowbuf[0][x] = pack_raw_cell(p);
    }
PACKED_ROW:
    for (int py = 0; py < CNN_PACK_H; ++py)
    {
        const int active = py & 1, fill = active ^ 1;
        int fill_x = 0;
    RAW_ROW_PAIR:
        for (int phase = 0; phase < CNN_RAW_W * 2; ++phase)
        {
#pragma HLS PIPELINE II = 1
            int local = (phase < CNN_RAW_W) ? phase : (phase - CNN_RAW_W);
            int px = local >> 1;
            int c = (phase < CNN_RAW_W) ? ((local & 1) ? 1 : 0) : ((local & 1) ? 3 : 2);
            ap_uint<40> cell = rowbuf[active][px];
            IspPixelPacket<10> o;
            o.data = cell.range(c * 10 + 9, c * 10);
            o.user = (py == 0 && phase == 0) ? 1 : 0;
            o.last = ((phase == CNN_RAW_W - 1) || (phase == CNN_RAW_W * 2 - 1)) ? 1 : 0;
            raw_out.write(o);
            if ((py + 1 < CNN_PACK_H) && ((phase & 3) == 3))
            {
                IspPixelPacket<40> np = packed_in.read();
                rowbuf[fill][fill_x] = pack_raw_cell(np);
                ++fill_x;
            }
        }
    }
}
