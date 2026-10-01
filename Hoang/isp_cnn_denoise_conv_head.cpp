#include "isp_cnn_denoise.h"
#include "isp_cnn_denoise_params.h"

static feature_bits_t quantize_raw(ap_uint<10> raw)
{
#pragma HLS INLINE
    acc_t v = (raw > L0_POST_BLC_MAX) ? (acc_t)L0_POST_BLC_MAX : (acc_t)raw;
    acc_t q = q31_round(v, L0_RAW_TO_HEAD_Q31, L0_RAW_TO_HEAD_EXP);
    acc_t clamped = (q < L0_RAW_TO_HEAD_QMIN) ? (acc_t)L0_RAW_TO_HEAD_QMIN : ((q > L0_RAW_TO_HEAD_QMAX) ? (acc_t)L0_RAW_TO_HEAD_QMAX : q);
    return (feature_bits_t)clamped.range(7, 0);
}

// Access weight of 1 channel
static ch_window_t make_window_pkt(feature_bits_t hw_in[4][3][3], ap_uint<2> ch, int out_y, int out_x)
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
            p.taps.range(8 * k + 7, 8 * k) = hw_in[ch][ky][kx];
        }
    }

    p.ch = ch;
    p.x = out_x;
    p.y = out_y;
    return p;
}

// Reset weight BRAM
static void reset_channel_hwin(feature_bits_t hwin[4][3][3], ap_uint<2> ch)
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

static void shift_channel_hwin(feature_bits_t hwin[4][3][3], ap_uint<2> ch, feature_bits_t top, feature_bits_t mid, feature_bits_t bot)
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

#if FREE_RUNNING

// Ingestion adapter cho AXI-Stream ngoại vi
static void axis_to_internal_stream(
    hls::stream<axis_raw10_t> &stream_in,
    hls::stream<IspPixelPacket<10>> &stream_out)
{
#pragma HLS INLINE
    // #pragma HLS PIPELINE II = 1
    axis_raw10_t in_pkt = stream_in.read();

    IspPixelPacket<10> out_pkt;
    out_pkt.data = in_pkt.data;
    out_pkt.user = in_pkt.user;
    out_pkt.last = in_pkt.last;
    stream_out.write(out_pkt);
}

static void raw_to_channel_windows(
    hls::stream<IspPixelPacket<10>> &stream_in,
    hls::stream<ch_window_t> &win_out,
    hls::stream<IspPixelPacket<40>> &global_skip_out,
    int height,
    int width)
{
#pragma HLS PIPELINE II = 1

    static feature_bits_t lb0[4][L0_MAX_PACK_W];
    static feature_bits_t lb1[4][L0_MAX_PACK_W];
    static feature_bits_t lb2[4][L0_MAX_PACK_W];
    static ap_uint<10> raw_hold[4][L0_MAX_PACK_W];

#pragma HLS ARRAY_PARTITION variable = lb0 complete dim = 1
#pragma HLS ARRAY_PARTITION variable = lb1 complete dim = 1
#pragma HLS ARRAY_PARTITION variable = lb2 complete dim = 1
#pragma HLS ARRAY_PARTITION variable = raw_hold complete dim = 1

#pragma HLS BIND_STORAGE variable = lb0 type = ram_1p impl = bram
#pragma HLS BIND_STORAGE variable = lb1 type = ram_1p impl = bram
#pragma HLS BIND_STORAGE variable = lb2 type = ram_1p impl = bram

    static feature_bits_t hwin[4][3][3] = {0};
#pragma HLS ARRAY_PARTITION variable = hwin complete dim = 0

    static int rx_row = 0;
    static int rx_col = 0;
    static bool frame_active = false;

    static int drain_step = 0;
    static int drain_sub = 0;
    static int drain_px = 0;

    const int pack_w = width >> 1;
    const int pack_h = height >> 1;

    // 1. DRAIN / FLUSH FSM CHO CAC DONG CUOI CUA FRAME
    if (drain_step != 0)
    {
        if (drain_step == 1) // RIGHT_EDGE_LAST_REAL_ROW
        {
            ap_uint<2> ch = drain_sub;
            shift_channel_hwin(hwin, ch, 0, 0, 0);
            win_out.write(make_window_pkt(hwin, ch, pack_h - 2, pack_w - 1));
            reset_channel_hwin(hwin, ch);

            if (drain_sub == 3)
            {
                drain_sub = 0;
                drain_px = 0;
                drain_step = 2;
            }
            else
            {
                ++drain_sub;
            }
        }
        else if (drain_step == 2) // BOTTOM_ROW_X
        {
            ap_uint<2> ch = drain_sub;
            const int vbank = pack_h % 3;

            feature_bits_t top, mid;
            if (vbank == 0)
            {
                top = lb1[ch][drain_px];
                mid = lb2[ch][drain_px];
            }
            else if (vbank == 1)
            {
                top = lb2[ch][drain_px];
                mid = lb0[ch][drain_px];
            }
            else
            {
                top = lb0[ch][drain_px];
                mid = lb1[ch][drain_px];
            }

            shift_channel_hwin(hwin, ch, top, mid, 0);

            if (drain_px >= 1)
            {
                win_out.write(make_window_pkt(hwin, ch, pack_h - 1, drain_px - 1));
            }

            if (drain_sub == 3)
            {
                drain_sub = 0;
                if (drain_px == pack_w - 1)
                {
                    drain_step = 3;
                }
                else
                {
                    ++drain_px;
                }
            }
            else
            {
                ++drain_sub;
            }
        }
        else if (drain_step == 3) // BOTTOM_RIGHT
        {
            ap_uint<2> ch = drain_sub;
            shift_channel_hwin(hwin, ch, 0, 0, 0);
            win_out.write(make_window_pkt(hwin, ch, pack_h - 1, pack_w - 1));

            if (drain_sub == 3)
            {
                drain_sub = 0;
                drain_step = 0;
            }
            else
            {
                ++drain_sub;
            }
        }
    }

    // 2. NHAN STREAMING VA HUNG DU LIEU MOI
    IspPixelPacket<10> pkt;
    if (stream_in.read_nb(pkt))
    {
        if (pkt.user == 1)
        {
            rx_row = 0;
            rx_col = 0;
            frame_active = true;
        }

        if (frame_active)
        {
            ap_uint<10> raw_v = pkt.data.range(9, 0);
            if (raw_v > L0_POST_BLC_MAX)
                raw_v = L0_POST_BLC_MAX;
            feature_bits_t q = quantize_raw(raw_v);

            const int py = rx_row >> 1;
            const int px = rx_col >> 1;
            const ap_uint<2> ch = ((rx_row & 1) << 1) | (rx_col & 1);
            const int bank = py % 3;
            raw_hold[ch][px] = raw_v;

            if (px == 0)
            {
                if (py >= 2)
                {
                    shift_channel_hwin(hwin, ch, 0, 0, 0);
                    win_out.write(make_window_pkt(hwin, ch, py - 2, pack_w - 1));
                }
                reset_channel_hwin(hwin, ch);
            }

            feature_bits_t top = 0;
            feature_bits_t mid = 0;

            if (bank == 0)
            {
                top = (py >= 2) ? lb1[ch][px] : (feature_bits_t)0;
                mid = (py >= 1) ? lb2[ch][px] : (feature_bits_t)0;
                lb0[ch][px] = q;
            }
            else if (bank == 1)
            {
                top = (py >= 2) ? lb2[ch][px] : (feature_bits_t)0;
                mid = (py >= 1) ? lb0[ch][px] : (feature_bits_t)0;
                lb1[ch][px] = q;
            }
            else
            {
                top = (py >= 2) ? lb0[ch][px] : (feature_bits_t)0;
                mid = (py >= 1) ? lb1[ch][px] : (feature_bits_t)0;
                lb2[ch][px] = q;
            }

            if (ch == 3)
            {
                IspPixelPacket<40> gs;
                gs.data = 0;
                gs.data.range(9, 0) = raw_hold[0][px];
                gs.data.range(19, 10) = raw_hold[1][px];
                gs.data.range(29, 20) = raw_hold[2][px];
                gs.data.range(39, 30) = raw_hold[3][px];
                gs.user = (py == 0 && px == 0) ? 1 : 0;
                gs.last = (px == pack_w - 1) ? 1 : 0;
                global_skip_out.write(gs);
            }

            shift_channel_hwin(hwin, ch, top, mid, q);

            if (py >= 1 && px >= 1)
            {
                win_out.write(make_window_pkt(hwin, ch, py - 1, px - 1));
            }

            if (rx_col == width - 1 && rx_row == height - 1)
            {
                frame_active = false;
                drain_step = 1;
                drain_sub = 0;
                drain_px = 0;
            }

            if (rx_col == width - 1)
            {
                rx_col = 0;
                ++rx_row;
            }
            else
            {
                ++rx_col;
            }
        }
    }
}

#else

static void axis_to_internal_stream(
    hls::stream<axis_raw10_t> &stream_in,
    hls::stream<IspPixelPacket<10>> &stream_out,
    int total_pixels)
{
AXIS_IN_LOOP:
    for (int i = 0; i < total_pixels; ++i)
    {
#pragma HLS PIPELINE II = 1
        axis_raw10_t in_pkt = stream_in.read();
        IspPixelPacket<10> out_pkt;
        out_pkt.data = in_pkt.data;
        out_pkt.user = in_pkt.user;
        out_pkt.last = in_pkt.last;
        stream_out.write(out_pkt);
    }
}

static void raw_to_channel_windows(
    hls::stream<IspPixelPacket<10>> &stream_in,
    hls::stream<ch_window_t> &win_out,
    hls::stream<IspPixelPacket<40>> &global_skip_out,
    int height,
    int width)
{
    const int pack_w = width >> 1;
    const int pack_h = height >> 1;

    static feature_bits_t lb0[4][L0_MAX_PACK_W];
    static feature_bits_t lb1[4][L0_MAX_PACK_W];
    static feature_bits_t lb2[4][L0_MAX_PACK_W];
    static ap_uint<10> raw_hold[4][L0_MAX_PACK_W];

#pragma HLS ARRAY_PARTITION variable = lb0 complete dim = 1
#pragma HLS ARRAY_PARTITION variable = lb1 complete dim = 1
#pragma HLS ARRAY_PARTITION variable = lb2 complete dim = 1
#pragma HLS ARRAY_PARTITION variable = raw_hold complete dim = 1

#pragma HLS BIND_STORAGE variable = lb0 type = ram_1p impl = bram
#pragma HLS BIND_STORAGE variable = lb1 type = ram_1p impl = bram
#pragma HLS BIND_STORAGE variable = lb2 type = ram_1p impl = bram

#pragma HLS RESET variable = lb0 off
#pragma HLS RESET variable = lb1 off
#pragma HLS RESET variable = lb2 off

    feature_bits_t hwin[4][3][3];
#pragma HLS ARRAY_PARTITION variable = hwin complete dim = 0

    int row = 0;
    int col = 0;
    const int total_raw = height * width;

RAW_LOOP:
    for (int n = 0; n < total_raw; ++n)
    {
#pragma HLS PIPELINE II = 1

        IspPixelPacket<10> pkt = stream_in.read();
        ap_uint<10> raw_v = pkt.data.range(9, 0);
        if (raw_v > L0_POST_BLC_MAX)
            raw_v = L0_POST_BLC_MAX;
        feature_bits_t q = quantize_raw(raw_v);

        const int py = row >> 1;
        const int px = col >> 1;
        const ap_uint<2> ch = ((row & 1) << 1) | (col & 1);
        const int bank = py % 3;
        raw_hold[ch][px] = raw_v;

        if (px == 0)
        {
            if (py >= 2)
            {
                shift_channel_hwin(hwin, ch, 0, 0, 0);
                win_out.write(make_window_pkt(hwin, ch, py - 2, pack_w - 1));
            }
            reset_channel_hwin(hwin, ch);
        }

        feature_bits_t top = 0;
        feature_bits_t mid = 0;

        if (bank == 0)
        {
            top = (py >= 2) ? lb1[ch][px] : (feature_bits_t)0;
            mid = (py >= 1) ? lb2[ch][px] : (feature_bits_t)0;
            lb0[ch][px] = q;
        }
        else if (bank == 1)
        {
            top = (py >= 2) ? lb2[ch][px] : (feature_bits_t)0;
            mid = (py >= 1) ? lb0[ch][px] : (feature_bits_t)0;
            lb1[ch][px] = q;
        }
        else
        {
            top = (py >= 2) ? lb0[ch][px] : (feature_bits_t)0;
            mid = (py >= 1) ? lb1[ch][px] : (feature_bits_t)0;
            lb2[ch][px] = q;
        }

        if (ch == 3)
        {
            IspPixelPacket<40> gs;
            gs.data = 0;
            gs.data.range(9, 0) = raw_hold[0][px];
            gs.data.range(19, 10) = raw_hold[1][px];
            gs.data.range(29, 20) = raw_hold[2][px];
            gs.data.range(39, 30) = raw_hold[3][px];
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
        win_out.write(make_window_pkt(hwin, ch, pack_h - 2, pack_w - 1));
        reset_channel_hwin(hwin, ch);
    }

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

            feature_bits_t top, mid;
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

BOTTOM_RIGHT:
    for (int ch_i = 0; ch_i < 4; ++ch_i)
    {
#pragma HLS PIPELINE II = 1
        ap_uint<2> ch = ch_i;
        shift_channel_hwin(hwin, ch, 0, 0, 0);
        win_out.write(make_window_pkt(hwin, ch, pack_h - 1, pack_w - 1));
    }
}

#endif

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

#if FREE_RUNNING
static void conv_accum_single_psum(
    hls::stream<ch_window_t> &win_in,
    hls::stream<l0_acc_pkt_t> &acc_out,
    int pack_h,
    int pack_w)
{
    static acc_t psum[L0_COUT][L0_MAX_PACK_W];

#pragma HLS ARRAY_PARTITION variable = psum complete dim = 1
#pragma HLS BIND_STORAGE variable = psum type = ram_1p impl = bram

#pragma HLS ARRAY_PARTITION variable = L0_WBANK complete dim = 1
#pragma HLS BIND_STORAGE variable = L0_WBANK type = rom_1p impl = bram

#pragma HLS ARRAY_PARTITION variable = L0_BIAS complete

    static acc_t pair_hold[L0_COUT];
#pragma HLS ARRAY_PARTITION variable = pair_hold complete

#pragma HLS PIPELINE II = 1

    ch_window_t wp;
    if (win_in.read_nb(wp))
    {
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
                ap_uint<8> x_val = wp.taps.range(k * 8 + 7, k * 8);
                weight_t w = get_weight(L0_WBANK[k][ch], oc);

                ap_int<16> prod = (ap_int<16>)x_val * (ap_int<16>)w;
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

#else

static void conv_accum_single_psum(
    hls::stream<ch_window_t> &win_in,
    hls::stream<l0_acc_pkt_t> &acc_out,
    int pack_h,
    int pack_w)
{
    static acc_t psum[L0_COUT][L0_MAX_PACK_W];

#pragma HLS ARRAY_PARTITION variable = psum complete dim = 1
#pragma HLS BIND_STORAGE variable = psum type = ram_1p impl = bram

#pragma HLS ARRAY_PARTITION variable = L0_WBANK complete dim = 1
#pragma HLS BIND_STORAGE variable = L0_WBANK type = rom_1p impl = bram

#pragma HLS ARRAY_PARTITION variable = L0_BIAS complete

    static acc_t pair_hold[L0_COUT];
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
                ap_uint<8> x_val = wp.taps.range(k * 8 + 7, k * 8);
                weight_t w = get_weight(L0_WBANK[k][ch], oc);

                ap_int<16> prod = (ap_int<16>)x_val * (ap_int<16>)w;
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

#endif

#if FREE_RUNNING

static void requantize_and_stream(
    hls::stream<l0_acc_pkt_t> &acc_in,
    hls::stream<IspPixelPacket<128>> &stream_out,
    int total_out)
{
#pragma HLS ARRAY_PARTITION variable = L0_HEAD_TO_B1_Q31 complete
#pragma HLS ARRAY_PARTITION variable = L0_HEAD_TO_B1_EXP complete

#pragma HLS PIPELINE II = 1

    l0_acc_pkt_t in;
    if (acc_in.read_nb(in))
    {
        IspPixelPacket<128> out;
        out.data = 0;

        for (int oc = 0; oc < L0_COUT; ++oc)
        {
#pragma HLS UNROLL
            acc_t v = (acc_t)in.data.range(oc * 32 + 31, oc * 32);

            // ReLU
            if (v < 0)
                v = 0;

            // Quantize
            acc_t rq = q31_round(v, L0_HEAD_TO_B1_Q31[oc], L0_HEAD_TO_B1_EXP[oc]);
            ufeature_t q = sat_u8(rq);
            out.data.range(oc * 8 + 7, oc * 8) = u8_to_bits(q);
        }

        out.user = in.user;
        out.last = in.last;

        stream_out.write(out);
    }
}

#else

static void requantize_and_stream(
    hls::stream<l0_acc_pkt_t> &acc_in,
    hls::stream<IspPixelPacket<128>> &stream_out,
    int total_out)
{
#pragma HLS ARRAY_PARTITION variable = L0_HEAD_TO_B1_Q31 complete
#pragma HLS ARRAY_PARTITION variable = L0_HEAD_TO_B1_EXP complete

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

            // ReLU
            if (v < 0)
                v = 0;

            // Quantize
            acc_t rq = q31_round(v, L0_HEAD_TO_B1_Q31[oc], L0_HEAD_TO_B1_EXP[oc]);
            ufeature_t q = sat_u8(rq);
            out.data.range(oc * 8 + 7, oc * 8) = u8_to_bits(q);
        }

        out.user = in.user;
        out.last = in.last;

        stream_out.write(out);
    }
}

#endif

// Dataflow core chuyên dụng chỉ làm việc với struct nội bộ
static void cnn_l0_dataflow(
    hls::stream<IspPixelPacket<10>> &stream_in,
    hls::stream<IspPixelPacket<128>> &stream_out,
    hls::stream<IspPixelPacket<40>> &global_skip_out,
    int height, int width, int pack_h, int pack_w, int total_out)
{
    hls::stream<ch_window_t> win_stream("l0_ch_window_stream");
    hls::stream<l0_acc_pkt_t> acc_stream("l0_acc_stream");

#pragma HLS STREAM variable = win_stream depth = 64
#pragma HLS STREAM variable = acc_stream depth = 64
#pragma HLS DATAFLOW

    raw_to_channel_windows(stream_in, win_stream, global_skip_out, height, width);
    conv_accum_single_psum(win_stream, acc_stream, pack_h, pack_w);
    requantize_and_stream(acc_stream, stream_out, total_out);
}

// TOP Module với AXI4-Stream Interface
void cnn_l0_streaming(hls::stream<axis_raw10_t> &stream_in,
                      hls::stream<IspPixelPacket<128>> &stream_out,
                      hls::stream<IspPixelPacket<40>> &global_skip_out,
                      int height,
                      int width)
{
#pragma HLS INTERFACE axis port = stream_in
#pragma HLS INTERFACE axis port = stream_out
#pragma HLS INTERFACE axis port = global_skip_out

    // #pragma HLS INTERFACE s_axilite port = height bundle = control
    // #pragma HLS INTERFACE s_axilite port = width bundle = control

#if FREE_RUNNING
#pragma HLS INTERFACE ap_ctrl_none port = return
#else
#pragma HLS INTERFACE s_axilite port = return bundle = control
#endif

    if (height <= 0 || width <= 0 || height > L0_MAX_RAW_H || width > L0_MAX_RAW_W || (height & 1) || (width & 1))
        return;

    const int pack_h = height >> 1;
    const int pack_w = width >> 1;
    const int total_out = pack_h * pack_w;

    hls::stream<IspPixelPacket<10>> internal_raw("l0_internal_raw");
    hls::stream<ch_window_t> win_stream("l0_ch_window_stream");
    hls::stream<l0_acc_pkt_t> acc_stream("l0_acc_stream");

#pragma HLS STREAM variable = internal_raw depth = 64
#pragma HLS STREAM variable = win_stream depth = 64
#pragma HLS STREAM variable = acc_stream depth = 64
#pragma HLS DATAFLOW

#if FREE_RUNNING
    axis_to_internal_stream(stream_in, internal_raw);
#else
    axis_to_internal_stream(stream_in, internal_raw, height * width);
#endif
    raw_to_channel_windows(internal_raw, win_stream, global_skip_out, CNN_RAW_H, CNN_RAW_W);
    conv_accum_single_psum(win_stream, acc_stream, pack_h, pack_w);
    requantize_and_stream(acc_stream, stream_out, pack_h * pack_w);
}

// TOP Interface core cho mạng ghép
void cnn_l0_axis_core(
    hls::stream<axis_raw10_t> &stream_in,
    hls::stream<IspPixelPacket<128>> &stream_out,
    hls::stream<IspPixelPacket<40>> &global_skip_out)
{
#pragma HLS INTERFACE axis port = stream_in
#pragma HLS INTERFACE axis port = stream_out
#pragma HLS INTERFACE axis port = global_skip_out

    cnn_l0_streaming(stream_in, stream_out, global_skip_out, L0_MAX_RAW_H, L0_MAX_RAW_W);
}

// INTERNAL FIXED CORE (nhận IspPixelPacket<10>)
void cnn_l0_core(
    hls::stream<IspPixelPacket<10>> &stream_in,
    hls::stream<IspPixelPacket<128>> &stream_out,
    hls::stream<IspPixelPacket<40>> &global_skip_out)
{
    cnn_l0_dataflow(stream_in, stream_out, global_skip_out,
                    L0_MAX_RAW_H, L0_MAX_RAW_W,
                    L0_MAX_PACK_H, L0_MAX_PACK_W,
                    L0_MAX_PACK_H * L0_MAX_PACK_W);
}