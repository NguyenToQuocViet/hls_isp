#include "isp_cnn_denoise.h"
#include "isp_cnn_denoise_params.h"

template <int ID, bool INPUT_SIGNED>
static void conv16_group_accum(hls::stream<feat16_window_t> &win_in, hls::stream<acc16_pkt_t> &acc_out, const body_weight_word_t weights[9][CNN_C], const bias_t bias[CNN_C])
{
#pragma HLS ARRAY_PARTITION variable = weights complete dim = 1
#pragma HLS ARRAY_PARTITION variable = weights cyclic factor = 4 dim = 2
#pragma HLS ARRAY_PARTITION variable = bias complete
    acc_t group_sum[CNN_GROUPS][CNN_C];
#pragma HLS ARRAY_PARTITION variable = group_sum complete dim = 0
    feat16_window_t current;
    const int total_groups = CNN_PIXELS * CNN_GROUPS;
GROUP_LOOP:
    for (int n = 0; n < total_groups; ++n)
    {
#pragma HLS PIPELINE II = 1
        const int g = n & (CNN_GROUPS - 1);
        if (g == 0)
            current = win_in.read();
        acc_t delta[CNN_C];
#pragma HLS ARRAY_PARTITION variable = delta complete
        for (int oc = 0; oc < CNN_C; ++oc)
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
                    ap_int<16> xv = INPUT_SIGNED ? (ap_int<16>)(ap_int<8>)xb : (ap_int<16>)(ap_uint<8>)xb;
                    body_weight_word_t ww = weights[k][ic];
                    weight_t wv = (weight_t)ww.range(oc * 8 + 7, oc * 8);
                    ap_int<16> prod = xv * (ap_int<16>)wv;
                    sum += (acc_t)prod;
                }
            }
            delta[oc] = sum;
            group_sum[g][oc] = sum;
        }
        if (g == CNN_GROUPS - 1)
        {
            acc16_pkt_t out;
            out.data = 0;
            out.user = (current.y == 0 && current.x == 0) ? 1 : 0;
            out.last = (current.x == CNN_PACK_W - 1) ? 1 : 0;
            for (int oc = 0; oc < CNN_C; ++oc)
            {
#pragma HLS UNROLL
                acc_t v = bias[oc] + group_sum[0][oc] + group_sum[1][oc] + group_sum[2][oc] + delta[oc];
                out.data.range(oc * 32 + 31, oc * 32) = v.range(31, 0);
            }
            acc_out.write(out);
        }
    }
}

template <int ID, bool INPUT_SIGNED>
static void conv16_accum_layer(hls::stream<IspPixelPacket<128>> &in, hls::stream<acc16_pkt_t> &acc_out, const body_weight_word_t weights[9][CNN_C], const bias_t bias[CNN_C])
{
    hls::stream<feat16_window_t> win_fifo("feat16_window_fifo");
#pragma HLS STREAM variable = win_fifo depth = 16
#pragma HLS DATAFLOW
    feature16_to_windows_core(in, win_fifo, ID);
    conv16_group_accum<ID, INPUT_SIGNED>(win_fifo, acc_out, weights, bias);
}

static void relu_requant16_u8(hls::stream<acc16_pkt_t> &acc_in, hls::stream<IspPixelPacket<128>> &out, const q31_t q31[CNN_C], const qexp_t exp[CNN_C], int qmin, int qmax)
{
#pragma HLS ARRAY_PARTITION variable = q31 complete
#pragma HLS ARRAY_PARTITION variable = exp complete
RELU_RQ:
    for (int p = 0; p < CNN_PIXELS; ++p)
    {
#pragma HLS PIPELINE II = 1
        acc16_pkt_t a = acc_in.read();
        IspPixelPacket<128> o;
        o.data = 0;
        o.user = a.user;
        o.last = a.last;
        for (int c = 0; c < CNN_C; ++c)
        {
#pragma HLS UNROLL
            acc_t v = (acc_t)a.data.range(c * 32 + 31, c * 32);
            acc_t relu_v = (v < 0) ? (acc_t)0 : v;
            acc_t rq = q31_round(relu_v, q31[c], exp[c]);
            acc_t clamped = (rq < qmin) ? (acc_t)qmin : ((rq > qmax) ? (acc_t)qmax : rq);
            o.data.range(c * 8 + 7, c * 8) = (ap_uint<8>)clamped.range(7, 0);
        }
        out.write(o);
    }
}

template <bool SKIP_SIGNED>
static void residual_add_requant_next(hls::stream<acc16_pkt_t> &main_acc, hls::stream<IspPixelPacket<128>> &skip, hls::stream<IspPixelPacket<128>> &out, const q31_t main_q31[CNN_C], const qexp_t main_exp[CNN_C], q31_t next_q31, qexp_t next_exp, int next_qmin, int next_qmax)
{
#pragma HLS ARRAY_PARTITION variable = main_q31 complete
#pragma HLS ARRAY_PARTITION variable = main_exp complete
RES_ADD_RQ:
    for (int p = 0; p < CNN_PIXELS; ++p)
    {
#pragma HLS PIPELINE II = 1
        acc16_pkt_t a = main_acc.read();
        IspPixelPacket<128> s = skip.read();
        IspPixelPacket<128> o;
        o.data = 0;
        o.user = a.user;
        o.last = a.last;
        for (int c = 0; c < CNN_C; ++c)
        {
#pragma HLS UNROLL
            acc_t main_acc_v = (acc_t)a.data.range(c * 32 + 31, c * 32);
            acc_t main_units = q31_round(main_acc_v, main_q31[c], main_exp[c]);
            ap_uint<8> sb = s.data.range(c * 8 + 7, c * 8);
            acc_t skip_units = SKIP_SIGNED ? (acc_t)(ap_int<8>)sb : (acc_t)(ap_uint<8>)sb;
            acc_t residual_wide = skip_units + main_units;
            acc_t next_units = q31_round(residual_wide, next_q31, next_exp);
            acc_t clamped = (next_units < next_qmin) ? (acc_t)next_qmin : ((next_units > next_qmax) ? (acc_t)next_qmax : next_units);
            o.data.range(c * 8 + 7, c * 8) = (ap_uint<8>)clamped.range(7, 0);
        }
        out.write(o);
    }
}

void cnn_l1_conv1(hls::stream<IspPixelPacket<128>> &in, hls::stream<IspPixelPacket<128>> &out)
{
    hls::stream<acc16_pkt_t> acc("l1_acc");
#pragma HLS STREAM variable = acc depth = 32
#pragma HLS DATAFLOW
    conv16_accum_layer<1, false>(in, acc, L1_WBANK, L1_BIAS);
    relu_requant16_u8(acc, out, L1_TO_CONV2_Q31, L1_TO_CONV2_EXP, L2_INPUT_QMIN, L2_INPUT_QMAX);
}

void cnn_l2_conv2_add(hls::stream<IspPixelPacket<128>> &in, hls::stream<IspPixelPacket<128>> &skip, hls::stream<IspPixelPacket<128>> &out)
{
    hls::stream<acc16_pkt_t> acc("l2_acc");
#pragma HLS STREAM variable = acc depth = 32
#pragma HLS DATAFLOW
    conv16_accum_layer<2, false>(in, acc, L2_WBANK, L2_BIAS);
    residual_add_requant_next<false>(acc, skip, out, L2_MAIN_TO_SKIP_Q31, L2_MAIN_TO_SKIP_EXP, L2_RESIDUAL_TO_L3_INPUT_Q31, L2_RESIDUAL_TO_L3_INPUT_EXP, L2_RESIDUAL_TO_L3_INPUT_QMIN, L2_RESIDUAL_TO_L3_INPUT_QMAX);
}

void cnn_l3_conv1(hls::stream<IspPixelPacket<128>> &in, hls::stream<IspPixelPacket<128>> &out)
{
    hls::stream<acc16_pkt_t> acc("l3_acc");
#pragma HLS STREAM variable = acc depth = 32
#pragma HLS DATAFLOW
    conv16_accum_layer<3, true>(in, acc, L3_WBANK, L3_BIAS);
    relu_requant16_u8(acc, out, L3_TO_CONV2_Q31, L3_TO_CONV2_EXP, L4_INPUT_QMIN, L4_INPUT_QMAX);
}

void cnn_l4_conv2_add(hls::stream<IspPixelPacket<128>> &in, hls::stream<IspPixelPacket<128>> &skip, hls::stream<IspPixelPacket<128>> &out)
{
    hls::stream<acc16_pkt_t> acc("l4_acc");
#pragma HLS STREAM variable = acc depth = 32
#pragma HLS DATAFLOW
    conv16_accum_layer<4, false>(in, acc, L4_WBANK, L4_BIAS);
    residual_add_requant_next<true>(acc, skip, out, L4_MAIN_TO_SKIP_Q31, L4_MAIN_TO_SKIP_EXP, L4_RESIDUAL_TO_L5_INPUT_Q31, L4_RESIDUAL_TO_L5_INPUT_EXP, L4_RESIDUAL_TO_L5_INPUT_QMIN, L4_RESIDUAL_TO_L5_INPUT_QMAX);
}

void cnn_resblock0(hls::stream<IspPixelPacket<128>> &in, hls::stream<IspPixelPacket<128>> &out)
{
    hls::stream<IspPixelPacket<128>> main_in("rb0_main_in"), conv1_out("rb0_conv1_out"), skip("rb0_skip");
#pragma HLS STREAM variable = main_in depth = 64
#pragma HLS STREAM variable = conv1_out depth = 64
#pragma HLS STREAM variable = skip depth = 4096
#pragma HLS DATAFLOW
    fork_feature16(in, main_in, skip);
    cnn_l1_conv1(main_in, conv1_out);
    cnn_l2_conv2_add(conv1_out, skip, out);
}

void cnn_resblock1(hls::stream<IspPixelPacket<128>> &in, hls::stream<IspPixelPacket<128>> &out)
{
    hls::stream<IspPixelPacket<128>> main_in("rb1_main_in"), conv1_out("rb1_conv1_out"), skip("rb1_skip");
#pragma HLS STREAM variable = main_in depth = 64
#pragma HLS STREAM variable = conv1_out depth = 64
#pragma HLS STREAM variable = skip depth = 4096
#pragma HLS DATAFLOW
    fork_feature16(in, main_in, skip);
    cnn_l3_conv1(main_in, conv1_out);
    cnn_l4_conv2_add(conv1_out, skip, out);
}
