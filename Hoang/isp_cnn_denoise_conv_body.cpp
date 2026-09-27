#include "isp_cnn_denoise.h"
#include "isp_cnn_denoise_params.h"


template<int ID>
static void conv16_group_accum(hls::stream<feat16_window_t>& win_in, hls::stream<acc16_pkt_t>& acc_out, const body_weight_word_t weights[9][CNN_C], const bias_t bias[CNN_C]) {
#pragma HLS ARRAY_PARTITION variable=weights complete dim=1
#pragma HLS ARRAY_PARTITION variable=weights cyclic factor=4 dim=2
#pragma HLS ARRAY_PARTITION variable=bias complete

    acc_t group_sum[CNN_GROUPS][CNN_C];
#pragma HLS ARRAY_PARTITION variable=group_sum complete dim=0

    feat16_window_t current;
    const int total_groups = CNN_PIXELS * CNN_GROUPS;

GROUP_LOOP:
for (int n = 0; n < total_groups; ++n) {
#pragma HLS PIPELINE II=1
        const int g = n & (CNN_GROUPS - 1);

        if (g == 0) current = win_in.read();

        acc_t delta[CNN_C];
#pragma HLS ARRAY_PARTITION variable=delta complete

        for (int oc = 0; oc < CNN_C; ++oc) {
#pragma HLS UNROLL
            acc_t sum = 0;

            for (int li = 0; li < CNN_PAR_IN; ++li) {
#pragma HLS UNROLL
                const int ic = g * CNN_PAR_IN + li;

                for (int k = 0; k < 9; ++k) {
#pragma HLS UNROLL
                    feature_t xv = bits_to_feature(current.taps[ic].range(k * 8 + 7, k * 8));

                    body_weight_word_t ww = weights[k][ic];
                    feature_t wv = bits_to_feature(ww.range(oc*8+7,oc*8));

                    ap_int<16> prod = (ap_int<16>)xv * (ap_int<16>)wv;
                    sum += (acc_t)prod;
                }
            }

            delta[oc] = sum;
            group_sum[g][oc] = sum;
        }

        if (g == CNN_GROUPS - 1) {
            acc16_pkt_t out;
            out.data = 0; out.user = (current.y == 0 && current.x == 0) ? 1 : 0; 
            out.last = (current.x == CNN_PACK_W - 1) ? 1 : 0;

            for (int oc = 0; oc < CNN_C; ++oc) {
#pragma HLS UNROLL
                acc_t v = bias[oc] + group_sum[0][oc] + group_sum[1][oc] + group_sum[2][oc] + delta[oc];

                out.data.range(oc*32+31,oc*32) = v.range(31,0);
            }

            acc_out.write(out);
        }
    }
}

// Full Conv16 accumulator = window stage + grouped MAC stage.
template<int ID>
static void conv16_accum_layer(hls::stream<IspPixelPacket<128>>& in, hls::stream<acc16_pkt_t>& acc_out, const body_weight_word_t weights[9][CNN_C], const bias_t bias[CNN_C]) {
    hls::stream<feat16_window_t> win_fifo("feat16_window_fifo");
#pragma HLS STREAM variable=win_fifo depth=16
#pragma HLS DATAFLOW

    feature16_to_windows_core(in, win_fifo, ID);
    conv16_group_accum<ID>(win_fifo, acc_out, weights, bias);
}

static void relu_requant16(hls::stream<acc16_pkt_t>& acc_in, hls::stream<IspPixelPacket<128>>& out, const q31_t q31[CNN_C], const qexp_t exp[CNN_C]) {
#pragma HLS ARRAY_PARTITION variable=q31 complete
#pragma HLS ARRAY_PARTITION variable=exp complete

RELU_RQ:
for (int p = 0; p < CNN_PIXELS; ++p) {
#pragma HLS PIPELINE II=1
        acc16_pkt_t a = acc_in.read();
        IspPixelPacket<128> o;
        o.data = 0;

        for (int c = 0; c < CNN_C; ++c) {
#pragma HLS UNROLL
            acc_t v = (acc_t)a.data.range(c*32+31,c*32);
            if (v < 0) v = 0;
            feature_t q = sat8(q31_round(v,q31[c],exp[c]));
            o.data.range(c*8+7,c*8) = feature_to_bits(q);
        }

        o.user = a.user; 
        o.last = a.last;
        out.write(o);
    }
}

static void residual_add16(hls::stream<acc16_pkt_t>& main_acc, hls::stream<IspPixelPacket<128>>& skip, hls::stream<IspPixelPacket<128>>& out, const q31_t main_q31[CNN_C], const qexp_t main_exp[CNN_C], q31_t skip_q31, qexp_t skip_exp) {
#pragma HLS ARRAY_PARTITION variable=main_q31 complete
#pragma HLS ARRAY_PARTITION variable=main_exp complete

RES_ADD:
for (int p = 0; p < CNN_PIXELS; ++p) {
#pragma HLS PIPELINE II=1
        acc16_pkt_t a = main_acc.read();
        IspPixelPacket<128> s = skip.read();

        IspPixelPacket<128> o;
        o.data = 0;

        for (int c = 0; c < CNN_C; ++c) {
#pragma HLS UNROLL
            acc_t main_acc_v = (acc_t)a.data.range(c*32+31,c*32);
            acc_t main_units = q31_round(main_acc_v,main_q31[c],main_exp[c]);

            feature_t sx = bits_to_feature(s.data.range(c*8+7,c*8));
            acc_t skip_units = q31_round((acc_t)sx,skip_q31,skip_exp);

            feature_t q = sat8(main_units + skip_units);
            o.data.range(c*8+7,c*8) = feature_to_bits(q);
        }

        o.user = a.user; o.last = a.last;
        out.write(o);
    }
}


// ------------------------------------------------------------
// L1 = Residual Block 0 / Conv1
// Conv3x3 16->16 -> ReLU -> requant INT8
void cnn_l1_conv1(hls::stream<IspPixelPacket<128>>& in, hls::stream<IspPixelPacket<128>>& out) {
    hls::stream<acc16_pkt_t> acc("l1_acc");
#pragma HLS STREAM variable=acc depth=32
#pragma HLS DATAFLOW

    conv16_accum_layer<1>(in, acc, L1_WBANK, L1_BIAS);
    relu_requant16(acc, out, L1_Q31, L1_EXP);
}


// L2 = Residual Block 0 / Conv2
// Conv3x3 16->16 -> main requant -> local residual add -> INT8
void cnn_l2_conv2_add(hls::stream<IspPixelPacket<128>>& in, hls::stream<IspPixelPacket<128>>& skip, hls::stream<IspPixelPacket<128>>& out) {
    hls::stream<acc16_pkt_t> acc("l2_acc");
#pragma HLS STREAM variable=acc depth=32
#pragma HLS DATAFLOW

    conv16_accum_layer<2>(in, acc, L2_WBANK, L2_BIAS);
    residual_add16(acc, skip, out, L2_MAIN_Q31, L2_MAIN_EXP, L2_SKIP_Q31, L2_SKIP_EXP);
}


// L3 = Residual Block 1 / Conv1
void cnn_l3_conv1(hls::stream<IspPixelPacket<128>>& in, hls::stream<IspPixelPacket<128>>& out) {
    hls::stream<acc16_pkt_t> acc("l3_acc");
#pragma HLS STREAM variable=acc depth=32
#pragma HLS DATAFLOW

    conv16_accum_layer<3>(in, acc, L3_WBANK, L3_BIAS);
    relu_requant16(acc, out, L3_Q31, L3_EXP);
}


// L4 = Residual Block 1 / Conv2 + residual add
void cnn_l4_conv2_add(hls::stream<IspPixelPacket<128>>& in, hls::stream<IspPixelPacket<128>>& skip, hls::stream<IspPixelPacket<128>>& out) {
    hls::stream<acc16_pkt_t> acc("l4_acc");
#pragma HLS STREAM variable=acc depth=32
#pragma HLS DATAFLOW

    conv16_accum_layer<4>(in, acc, L4_WBANK, L4_BIAS);
    residual_add16(acc, skip, out, L4_MAIN_Q31, L4_MAIN_EXP, L4_SKIP_Q31, L4_SKIP_EXP);
}


// L5 = Residual Block 2 / Conv1
void cnn_l5_conv1(hls::stream<IspPixelPacket<128>>& in, hls::stream<IspPixelPacket<128>>& out) {
    hls::stream<acc16_pkt_t> acc("l5_acc");
#pragma HLS STREAM variable=acc depth=32
#pragma HLS DATAFLOW

    conv16_accum_layer<5>(in, acc, L5_WBANK, L5_BIAS);
    relu_requant16(acc, out, L5_Q31, L5_EXP);
}


// L6 = Residual Block 2 / Conv2 + residual add
void cnn_l6_conv2_add(hls::stream<IspPixelPacket<128>>& in, hls::stream<IspPixelPacket<128>>& skip, hls::stream<IspPixelPacket<128>>& out) {
    hls::stream<acc16_pkt_t> acc("l6_acc");
#pragma HLS STREAM variable=acc depth=32
#pragma HLS DATAFLOW

    conv16_accum_layer<6>(in, acc, L6_WBANK, L6_BIAS);
    residual_add16(acc, skip, out, L6_MAIN_Q31, L6_MAIN_EXP, L6_SKIP_Q31, L6_SKIP_EXP);
}


// L7 = Residual Block 3 / Conv1
void cnn_l7_conv1(hls::stream<IspPixelPacket<128>>& in, hls::stream<IspPixelPacket<128>>& out) {
    hls::stream<acc16_pkt_t> acc("l7_acc");
#pragma HLS STREAM variable=acc depth=32
#pragma HLS DATAFLOW

    conv16_accum_layer<7>(in, acc, L7_WBANK, L7_BIAS);
    relu_requant16(acc, out, L7_Q31, L7_EXP);
}


// L8 = Residual Block 3 / Conv2 + residual add
void cnn_l8_conv2_add(hls::stream<IspPixelPacket<128>>& in, hls::stream<IspPixelPacket<128>>& skip, hls::stream<IspPixelPacket<128>>& out) {
    hls::stream<acc16_pkt_t> acc("l8_acc");
#pragma HLS STREAM variable=acc depth=32
#pragma HLS DATAFLOW

    conv16_accum_layer<8>(in, acc, L8_WBANK, L8_BIAS);
    residual_add16(acc, skip, out, L8_MAIN_Q31, L8_MAIN_EXP, L8_SKIP_Q31, L8_SKIP_EXP);
}


void cnn_resblock0(hls::stream<IspPixelPacket<128>>& in, hls::stream<IspPixelPacket<128>>& out) {
    hls::stream<IspPixelPacket<128>> main_in("rb0_main_in"), conv1_out("rb0_conv1_out"), skip("rb0_skip");

#pragma HLS STREAM variable=main_in depth=64
#pragma HLS STREAM variable=conv1_out depth=64
#pragma HLS STREAM variable=skip depth=4096
#pragma HLS DATAFLOW

    fork_feature16(in, main_in, skip);
    cnn_l1_conv1(main_in, conv1_out);
    cnn_l2_conv2_add(conv1_out, skip, out);
}

void cnn_resblock1(hls::stream<IspPixelPacket<128>>& in, hls::stream<IspPixelPacket<128>>& out) {
    hls::stream<IspPixelPacket<128>> main_in("rb1_main_in"), conv1_out("rb1_conv1_out"), skip("rb1_skip");

#pragma HLS STREAM variable=main_in depth=64
#pragma HLS STREAM variable=conv1_out depth=64
#pragma HLS STREAM variable=skip depth=4096
#pragma HLS DATAFLOW

    fork_feature16(in, main_in, skip);
    cnn_l3_conv1(main_in, conv1_out);
    cnn_l4_conv2_add(conv1_out, skip, out);
}

void cnn_resblock2(hls::stream<IspPixelPacket<128>>& in, hls::stream<IspPixelPacket<128>>& out) {
    hls::stream<IspPixelPacket<128>> main_in("rb2_main_in"), conv1_out("rb2_conv1_out"), skip("rb2_skip");

#pragma HLS STREAM variable=main_in depth=64
#pragma HLS STREAM variable=conv1_out depth=64
#pragma HLS STREAM variable=skip depth=4096
#pragma HLS DATAFLOW

    fork_feature16(in, main_in, skip);
    cnn_l5_conv1(main_in, conv1_out);
    cnn_l6_conv2_add(conv1_out, skip, out);
}

void cnn_resblock3(hls::stream<IspPixelPacket<128>>& in, hls::stream<IspPixelPacket<128>>& out) {
    hls::stream<IspPixelPacket<128>> main_in("rb3_main_in"), conv1_out("rb3_conv1_out"), skip("rb3_skip");

#pragma HLS STREAM variable=main_in depth=64
#pragma HLS STREAM variable=conv1_out depth=64
#pragma HLS STREAM variable=skip depth=4096
#pragma HLS DATAFLOW

    fork_feature16(in, main_in, skip);
    cnn_l7_conv1(main_in, conv1_out);
    cnn_l8_conv2_add(conv1_out, skip, out);
}

