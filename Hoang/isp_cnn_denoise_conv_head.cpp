#include "isp_cnn_denoise.h"
#include "isp_cnn_denoise_params.h"

static feature_t quantize_raw(ap_uint<10> raw)
{
#pragma HLS INLINE
    // filter raw
    acc_t v = (raw > L0_POST_BLC_MAX) ? (acc_t)L0_POST_BLC_MAX : (acc_t)raw;

    acc_t q = q31_round(v, L0_RAW_Q31, L0_RAW_EXP);
    if (q < 0)
        q = 0;
    if (q > 127)
        q = 127;
    return (feature_t)q;
}

// Access weight of 1 channel
static ch_window_t make_window_pkt(feature_t hw_in[4][3][3], ap_uint<2> ch, int out_y, int out_x)
{
#pragma HLS INLINE
#pragma HLS ARRAY_PARTITION variable = hw_in complete dim = 0

    ch_window_t p;
    p.taps = 0;

    for (int ky = 0; ky < 3; ++ky)
    {
#pragma HLS UNROLL
        for (int kx = 0; kx < 3; ++kx)
        {
#pragma HLS UNROLL
            int k = ky * 3 + kx;
            p.taps.range(8 * k + 7, 8 * k) = feature_to_bits(hw_in[ch][ky][kx]);
        }
    }

    p.ch = ch;
    p.x = out_x;
    p.y = out_y;
    return p;
}

// Reset weight BRAM
static void reset_channel_hwin(feature_t hwin[4][3][3], ap_uint<2> ch)
{
#pragma HLS INLINE
#pragma HLS ARRAY_PARTITION variable = hwin complete dim = 0
    for (int ky = 0; ky < 3; ++ky)
    {
#pragma HLS UNROLL
        for (int kx = 0; kx < 3; ++kx)
        {
#pragma HLS UNROLL
            hwin[ch][ky][kx] = 0;
        }
    }
}

static void shift_channel_hwin(feature_t hwin[4][3][3], ap_uint<2> ch, feature_t top, feature_t mid, feature_t bot)
{
#pragma HLS INLINE
#pragma HLS ARRAY_PARTITION variable = hwin complete dim = 0

    hwin[ch][0][0] = hwin[ch][0][1];
    hwin[ch][0][1] = hwin[ch][0][2];
    hwin[ch][0][2] = top;

    hwin[ch][1][0] = hwin[ch][1][1];
    hwin[ch][1][1] = hwin[ch][1][2];
    hwin[ch][1][2] = mid;

    hwin[ch][2][0] = hwin[ch][2][1];
    hwin[ch][2][1] = hwin[ch][2][2];
    hwin[ch][2][2] = bot;
}

template <typename RAW_PKT_T>
static void raw_to_channel_windows(hls::stream<RAW_PKT_T> &stream_in, hls::stream<ch_window_t> &win_out, hls::stream<IspPixelPacket<32>> &global_skip_out, int height, int width)
{
    const int pack_w = width >> 1;
    const int pack_h = height >> 1;

    static feature_t lb0[4][L0_MAX_PACK_W];
    static feature_t lb1[4][L0_MAX_PACK_W];
    static feature_t lb2[4][L0_MAX_PACK_W];

#pragma HLS ARRAY_PARTITION variable=lb0 complete dim=1
#pragma HLS ARRAY_PARTITION variable=lb1 complete dim=1
#pragma HLS ARRAY_PARTITION variable=lb2 complete dim=1

#pragma HLS BIND_STORAGE variable=lb0 type=ram_1p impl=bram
#pragma HLS BIND_STORAGE variable=lb1 type=ram_1p impl=bram
#pragma HLS BIND_STORAGE variable=lb2 type=ram_1p impl=bram

#pragma HLS RESET variable=lb0 off
#pragma HLS RESET variable=lb1 off
#pragma HLS RESET variable=lb2 off

    feature_t hwin[4][3][3];
#pragma HLS ARRAY_PARTITION variable=hwin complete dim=0

    int row = 0;
    int col = 0;
    const int total_raw = height * width;

RAW_LOOP:
    for (int n = 0; n < total_raw; ++n)
    {
#pragma HLS PIPELINE II=1

        RAW_PKT_T pkt = stream_in.read();
        feature_t q = quantize_raw(pkt.data.range(9, 0));

        const int py = row >> 1;
        const int px = col >> 1;
        const ap_uint<2> ch = ((row & 1) << 1) | (col & 1);
        const int bank = py % 3;

        if (px == 0)
        {
            if (py >= 2)
            {
                shift_channel_hwin(hwin, ch, 0, 0, 0);
                win_out.write(make_window_pkt(hwin, ch, py - 2, pack_w - 1));
            }

            reset_channel_hwin(hwin, ch);
        }

        feature_t top = 0;
        feature_t mid = 0;

        if (bank == 0)
        {
            top = (py >= 2) ? lb1[ch][px] : (feature_t)0;
            mid = (py >= 1) ? lb2[ch][px] : (feature_t)0;
            lb0[ch][px] = q;
        }
        else if (bank == 1)
        {
            top = (py >= 2) ? lb2[ch][px] : (feature_t)0;
            mid = (py >= 1) ? lb0[ch][px] : (feature_t)0;
            lb1[ch][px] = q;
        }
        else
        {
            top = (py >= 2) ? lb0[ch][px] : (feature_t)0;
            mid = (py >= 1) ? lb1[ch][px] : (feature_t)0;
            lb2[ch][px] = q;
        }

        if (ch == 3)
        {
            feature_t r_q;
            feature_t gr_q;
            feature_t gb_q;

            if (bank == 0)
            {
                r_q = lb0[0][px];
                gr_q = lb0[1][px];
                gb_q = lb0[2][px];
            }
            else if (bank == 1)
            {
                r_q = lb1[0][px];
                gr_q = lb1[1][px];
                gb_q = lb1[2][px];
            }
            else
            {
                r_q = lb2[0][px];
                gr_q = lb2[1][px];
                gb_q = lb2[2][px];
            }

            IspPixelPacket<32> gs;
            gs.data = 0;
            gs.data.range(7, 0) = feature_to_bits(r_q);
            gs.data.range(15, 8) = feature_to_bits(gr_q);
            gs.data.range(23, 16) = feature_to_bits(gb_q);
            gs.data.range(31, 24) = feature_to_bits(q);
            gs.user = (py == 0 && px == 0) ? 1 : 0;
            gs.last = (px == pack_w - 1) ? 1 : 0;

            global_skip_out.write(gs);
        }

        shift_channel_hwin(hwin, ch, top, mid, q);

        if ((py >= 1) && (px >= 1))
        {
            win_out.write(make_window_pkt(hwin, ch, py - 1, px - 1));
        }

        if (col == width - 1)
        {
            col = 0;
            ++row;
        }
        else
        {
            ++col;
        }
    }

RIGHT_EDGE_LAST_REAL_ROW:
    for (int ch_i = 0; ch_i < 4; ++ch_i)
    {
#pragma HLS PIPELINE II = 1
        ap_uint<2> ch = ch_i;
        shift_channel_hwin(hwin, ch, 0, 0, 0);
        win_out.write(
            make_window_pkt(hwin, ch, pack_h - 2, pack_w - 1));
        reset_channel_hwin(hwin, ch);
    }

    // top = real row pack_h-2
    // mid = real row pack_h-1
    // bot = zero
    const int vpy = pack_h;
    const int vbank = vpy % 3;

BOTTOM_ROW_X:
    for (int px = 0; px < pack_w; ++px)
    {
    BOTTOM_ROW_CH:
        for (int ch_i = 0; ch_i < 4; ++ch_i)
        {
#pragma HLS PIPELINE II = 1
            ap_uint<2> ch = ch_i;

            feature_t top, mid;
            if (vbank == 0)
            {
                top = lb1[ch][px];
                mid = lb2[ch][px];
            }
            else if (vbank == 1)
            {
                top = lb2[ch][px];
                mid = lb0[ch][px];
            }
            else
            {
                top = lb0[ch][px];
                mid = lb1[ch][px];
            }

            shift_channel_hwin(hwin, ch, top, mid, 0);

            if (px >= 1)
            {
                win_out.write(make_window_pkt(hwin, ch, pack_h - 1, px - 1));
            }
        }
    }

    // Bottom-right corner x = pack_w-1.
BOTTOM_RIGHT:
    for (int ch_i = 0; ch_i < 4; ++ch_i)
    {
#pragma HLS PIPELINE II = 1
        ap_uint<2> ch = ch_i;
        shift_channel_hwin(hwin, ch, 0, 0, 0);
        win_out.write(make_window_pkt(hwin, ch, pack_h - 1, pack_w - 1));
    }
}

static weight_t get_weight(weight_word_t word, int oc)
{
#pragma HLS INLINE
    ap_uint<8> b = word.range(oc * 8 + 7, oc * 8);
    return (weight_t)b;
}

struct l0_acc_pkt_t
{
    ap_uint<512> data;
    ap_uint<1> user;
    ap_uint<1> last;
};

static void conv_accum_single_psum(hls::stream<ch_window_t> &win_in, hls::stream<l0_acc_pkt_t> &acc_out, int pack_h, int pack_w)
{

    static acc_t psum[L0_COUT][L0_MAX_PACK_W];

#pragma HLS ARRAY_PARTITION variable = psum complete dim = 1
#pragma HLS BIND_STORAGE variable = psum type = ram_1p impl = bram

#pragma HLS ARRAY_PARTITION variable = L0_WBANK complete dim = 1
#pragma HLS BIND_STORAGE variable = L0_WBANK type = rom_1p impl = bram

#pragma HLS ARRAY_PARTITION variable = L0_BIAS complete

    acc_t pair_hold[L0_COUT];
#pragma HLS ARRAY_PARTITION variable = pair_hold complete

    const int total_loop = pack_h * pack_w * L0_CIN;

CONTRIB_LOOP:
    for (int n = 0; n < total_loop; ++n)
    {
#pragma HLS PIPELINE II = 1

        ch_window_t wp = win_in.read();

        const int x = (int)wp.x;
        const int y = (int)wp.y;
        const int ch = (int)wp.ch;

        acc_t delta[L0_COUT];
#pragma HLS ARRAY_PARTITION variable = delta complete

        for (int oc = 0; oc < L0_COUT; ++oc)
        {
#pragma HLS UNROLL
            acc_t sum = 0;

            for (int k = 0; k < 9; ++k)
            {
#pragma HLS UNROLL
                feature_t x = (feature_t)wp.taps.range(k * 8 + 7, k * 8);
                weight_t w = get_weight(L0_WBANK[k][ch], oc);

                ap_int<16> prod = (ap_int<16>)x * (ap_int<16>)w;
                sum += (acc_t)prod;
            }

            delta[oc] = sum;
        }

        if (ch == 0)
        { // R
            for (int oc = 0; oc < L0_COUT; ++oc)
            {
#pragma HLS UNROLL
                pair_hold[oc] = L0_BIAS[oc] + delta[oc];
            }
        }
        else if (ch == 1)
        { // Gr
            for (int oc = 0; oc < L0_COUT; ++oc)
            {
#pragma HLS UNROLL
                psum[oc][x] = pair_hold[oc] + delta[oc];
            }
        }
        else if (ch == 2)
        { // Gb
            for (int oc = 0; oc < L0_COUT; ++oc)
            {
#pragma HLS UNROLL
                acc_t oldv = psum[oc][x];
                pair_hold[oc] = oldv + delta[oc];
            }
        }
        else
        { // B
            l0_acc_pkt_t pkt;
            pkt.data = 0;

            for (int oc = 0; oc < L0_COUT; ++oc)
            {
#pragma HLS UNROLL
                acc_t complete = pair_hold[oc] + delta[oc];
                pkt.data.range(oc * 32 + 31, oc * 32) = complete.range(31, 0);
            }

            pkt.user = (y == 0 && x == 0) ? 1 : 0;
            pkt.last = (x == pack_w - 1) ? 1 : 0;

            acc_out.write(pkt);
        }
    }
}

static void requantize_and_stream(hls::stream<l0_acc_pkt_t> &acc_in, hls::stream<IspPixelPacket<128>> &stream_out, int total_out)
{
#pragma HLS ARRAY_PARTITION variable = L0_HEAD_Q31 complete
#pragma HLS ARRAY_PARTITION variable = L0_HEAD_EXP complete

REQUANT_LOOP:
    for (int p = 0; p < total_out; ++p)
    {
#pragma HLS PIPELINE II = 1

        l0_acc_pkt_t in = acc_in.read();

        IspPixelPacket<128> out;
        out.data = 0;

        for (int oc = 0; oc < L0_COUT; ++oc)
        {
#pragma HLS UNROLL
            acc_t v = (acc_t)in.data.range(oc * 32 + 31, oc * 32);

            // Relu
            if (v < 0)
                v = 0;

            // quantize for another layer
            acc_t rq = q31_round(v, L0_HEAD_Q31[oc], L0_HEAD_EXP[oc]);
            feature_t q = sat8(rq);
            out.data.range(oc * 8 + 7, oc * 8) = feature_to_bits(q);
        }

        out.user = in.user;
        out.last = in.last;

        stream_out.write(out);
    }
}

template <typename RAW_PKT_T>
static void cnn_l0_dataflow(hls::stream<RAW_PKT_T> &stream_in, hls::stream<IspPixelPacket<128>> &stream_out, hls::stream<IspPixelPacket<32>> &global_skip_out, int height, int width, int pack_h, int pack_w, int total_out)
{
    hls::stream<ch_window_t> win_stream("l0_ch_window_stream");
    hls::stream<l0_acc_pkt_t> acc_stream("l0_acc_stream");

#pragma HLS STREAM variable=win_stream depth=64
#pragma HLS STREAM variable=acc_stream depth=64
#pragma HLS DATAFLOW

    raw_to_channel_windows(stream_in, win_stream, global_skip_out, height, width);
    conv_accum_single_psum(win_stream, acc_stream, pack_h, pack_w);
    requantize_and_stream(acc_stream, stream_out, total_out);
}


void cnn_l0_streaming(hls::stream<axis_raw10_t> &stream_in, hls::stream<IspPixelPacket<128>> &stream_out, hls::stream<IspPixelPacket<32>> &global_skip_out, int height, int width)
{
#pragma HLS INTERFACE axis port=stream_in register_mode=off
#pragma HLS INTERFACE axis port=stream_out
#pragma HLS INTERFACE axis port=global_skip_out

#pragma HLS INTERFACE s_axilite port=height bundle=control
#pragma HLS INTERFACE s_axilite port=width bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

    if (height <= 0 || width <= 0 || height > L0_MAX_RAW_H || width > L0_MAX_RAW_W || (height & 1) || (width & 1)) return;

    const int pack_h = height >> 1;
    const int pack_w = width >> 1;
    const int total_out = pack_h * pack_w;

    cnn_l0_dataflow(stream_in, stream_out, global_skip_out, height, width, pack_h, pack_w, total_out);
}

// TOP
void cnn_l0_axis_core(hls::stream<axis_raw10_t> &stream_in, hls::stream<IspPixelPacket<128>> &stream_out, hls::stream<IspPixelPacket<32>> &global_skip_out)
{
    cnn_l0_dataflow(stream_in, stream_out, global_skip_out, L0_MAX_RAW_H, L0_MAX_RAW_W, L0_MAX_PACK_H, L0_MAX_PACK_W, L0_MAX_PACK_H * L0_MAX_PACK_W);
}

// INTERNAL FIXED FULL-HD CORE FOR COMPLETE NETWORK
void cnn_l0_core(hls::stream<IspPixelPacket<10>> &stream_in, hls::stream<IspPixelPacket<128>> &stream_out, hls::stream<IspPixelPacket<32>> &global_skip_out)
{
    cnn_l0_dataflow(stream_in, stream_out, global_skip_out, L0_MAX_RAW_H, L0_MAX_RAW_W, L0_MAX_PACK_H, L0_MAX_PACK_W, L0_MAX_PACK_H * L0_MAX_PACK_W);
}
