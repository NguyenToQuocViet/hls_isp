#include "isp_cnn_denoise.h"
#include "isp_cnn_denoise_params.h"
#include <array>
#include <cstdint>
#include <iostream>
#include <vector>

using RawFrame = std::vector<uint16_t>;
using Feat16Pixel = std::array<int32_t, CNN_C>;
using Feat16Frame = std::vector<Feat16Pixel>;
using Raw4Pixel = std::array<int32_t, 4>;
using Raw4Frame = std::vector<Raw4Pixel>;

static int32_t tb_q31_round(int32_t x, int32_t q31, int8_t exp)
{
    int64_t product = (int64_t)x * (int64_t)q31;
    int shift = 31 - (int)exp;
    if (shift <= 0)
        return (int32_t)(product << (-shift));
    if (shift >= 63)
        return 0;
    bool neg = product < 0;
    uint64_t mag = neg ? (uint64_t)(-product) : (uint64_t)product;
    uint64_t q = mag >> shift;
    uint64_t r = mag & ((((uint64_t)1) << shift) - 1);
    uint64_t half = ((uint64_t)1) << (shift - 1);
    if ((r > half) || ((r == half) && ((q & 1) != 0)))
        ++q;
    int64_t rounded = neg ? -(int64_t)q : (int64_t)q;
    return (int32_t)rounded;
}

static int32_t clamp_i32(int32_t x, int32_t lo, int32_t hi)
{
    return (x < lo) ? lo : ((x > hi) ? hi : x);
}

static int8_t weight_at(const body_weight_word_t weights[9][CNN_C], int k, int ic, int oc)
{
    ap_int<8> w = weights[k][ic].range(oc * 8 + 7, oc * 8);
    return (int8_t)(int)w;
}

static int8_t head_weight_at(int k, int ic, int oc)
{
    ap_int<8> w = L0_WBANK[k][ic].range(oc * 8 + 7, oc * 8);
    return (int8_t)(int)w;
}

static int8_t tail_weight_at(int k, int ic, int oc)
{
    ap_int<8> w = L5_WBANK[k][ic].range(oc * 8 + 7, oc * 8);
    return (int8_t)(int)w;
}

static int pack_idx(int y, int x)
{
    return y * CNN_PACK_W + x;
}

static int32_t feat_at(const Feat16Frame &f, int y, int x, int c)
{
    if (y < 0 || y >= CNN_PACK_H || x < 0 || x >= CNN_PACK_W)
        return 0;
    return f[pack_idx(y, x)][c];
}

static int32_t raw_q_u8(uint16_t raw)
{
    int32_t v = (raw > L0_POST_BLC_MAX) ? L0_POST_BLC_MAX : (int32_t)raw;
    int32_t q = tb_q31_round(v, (int32_t)L0_RAW_TO_HEAD_Q31, (int8_t)L0_RAW_TO_HEAD_EXP);
    return clamp_i32(q, L0_RAW_TO_HEAD_QMIN, L0_RAW_TO_HEAD_QMAX);
}

static uint16_t raw_at_packed(const RawFrame &raw, int py, int px, int ch)
{
    int ry = (py << 1) + ((ch >= 2) ? 1 : 0);
    int rx = (px << 1) + ((ch == 1 || ch == 3) ? 1 : 0);
    return raw[ry * CNN_RAW_W + rx];
}

static Feat16Frame golden_head(const RawFrame &raw)
{
    Feat16Frame out(CNN_PIXELS);
    for (int y = 0; y < CNN_PACK_H; ++y)
    {
        for (int x = 0; x < CNN_PACK_W; ++x)
        {
            for (int oc = 0; oc < CNN_C; ++oc)
            {
                int32_t acc = (int32_t)L0_BIAS[oc];
                for (int ic = 0; ic < 4; ++ic)
                {
                    for (int ky = 0; ky < 3; ++ky)
                    {
                        for (int kx = 0; kx < 3; ++kx)
                        {
                            int iy = y + ky - 1;
                            int ix = x + kx - 1;
                            int k = ky * 3 + kx;
                            int32_t xv = (iy < 0 || iy >= CNN_PACK_H || ix < 0 || ix >= CNN_PACK_W) ? 0 : raw_q_u8(raw_at_packed(raw, iy, ix, ic));
                            int32_t wv = (int32_t)head_weight_at(k, ic, oc);
                            acc += xv * wv;
                        }
                    }
                }
                int32_t relu_v = (acc < 0) ? 0 : acc;
                int32_t rq = tb_q31_round(relu_v, (int32_t)L0_HEAD_TO_B1_Q31[oc], (int8_t)L0_HEAD_TO_B1_EXP[oc]);
                out[pack_idx(y, x)][oc] = clamp_i32(rq, L1_INPUT_QMIN, L1_INPUT_QMAX);
            }
        }
    }
    return out;
}

static int32_t body_acc(const Feat16Frame &in, const body_weight_word_t weights[9][CNN_C], const bias_t bias[CNN_C], int y, int x, int oc)
{
    int32_t acc = (int32_t)bias[oc];
    for (int ic = 0; ic < CNN_C; ++ic)
    {
        for (int ky = 0; ky < 3; ++ky)
        {
            for (int kx = 0; kx < 3; ++kx)
            {
                int k = ky * 3 + kx;
                int32_t xv = feat_at(in, y + ky - 1, x + kx - 1, ic);
                int32_t wv = (int32_t)weight_at(weights, k, ic, oc);
                acc += xv * wv;
            }
        }
    }
    return acc;
}

static Feat16Frame golden_conv1_u8(const Feat16Frame &in, const body_weight_word_t weights[9][CNN_C], const bias_t bias[CNN_C], const q31_t q31[CNN_C], const qexp_t exp[CNN_C], int qmin, int qmax)
{
    Feat16Frame out(CNN_PIXELS);
    for (int y = 0; y < CNN_PACK_H; ++y)
    {
        for (int x = 0; x < CNN_PACK_W; ++x)
        {
            for (int c = 0; c < CNN_C; ++c)
            {
                int32_t acc = body_acc(in, weights, bias, y, x, c);
                int32_t relu_v = (acc < 0) ? 0 : acc;
                int32_t rq = tb_q31_round(relu_v, (int32_t)q31[c], (int8_t)exp[c]);
                out[pack_idx(y, x)][c] = clamp_i32(rq, qmin, qmax);
            }
        }
    }
    return out;
}

static Feat16Frame golden_conv2_add_next(const Feat16Frame &conv2_in, const Feat16Frame &skip, const body_weight_word_t weights[9][CNN_C], const bias_t bias[CNN_C], const q31_t main_q31[CNN_C], const qexp_t main_exp[CNN_C], q31_t next_q31, qexp_t next_exp, int next_qmin, int next_qmax)
{
    Feat16Frame out(CNN_PIXELS);
    for (int y = 0; y < CNN_PACK_H; ++y)
    {
        for (int x = 0; x < CNN_PACK_W; ++x)
        {
            int p = pack_idx(y, x);
            for (int c = 0; c < CNN_C; ++c)
            {
                int32_t acc = body_acc(conv2_in, weights, bias, y, x, c);
                int32_t main_units = tb_q31_round(acc, (int32_t)main_q31[c], (int8_t)main_exp[c]);
                int32_t residual_wide = skip[p][c] + main_units;
                int32_t next_units = tb_q31_round(residual_wide, (int32_t)next_q31, (int8_t)next_exp);
                out[p][c] = clamp_i32(next_units, next_qmin, next_qmax);
            }
        }
    }
    return out;
}

static Raw4Frame golden_tail(const RawFrame &raw, const Feat16Frame &tail_in)
{
    Raw4Frame out(CNN_PIXELS);
    for (int y = 0; y < CNN_PACK_H; ++y)
    {
        for (int x = 0; x < CNN_PACK_W; ++x)
        {
            int p = pack_idx(y, x);
            for (int oc = 0; oc < 4; ++oc)
            {
                int32_t acc = (int32_t)L5_BIAS[oc];
                for (int ic = 0; ic < CNN_C; ++ic)
                {
                    for (int ky = 0; ky < 3; ++ky)
                    {
                        for (int kx = 0; kx < 3; ++kx)
                        {
                            int k = ky * 3 + kx;
                            int32_t xv = feat_at(tail_in, y + ky - 1, x + kx - 1, ic);
                            int32_t wv = (int32_t)tail_weight_at(k, ic, oc);
                            acc += xv * wv;
                        }
                    }
                }
                int32_t residual_dn = tb_q31_round(acc, (int32_t)L5_TAIL_TO_RAW_DN_Q31[oc], (int8_t)L5_TAIL_TO_RAW_DN_EXP[oc]);
                int32_t raw_skip_dn = (int32_t)raw_at_packed(raw, y, x, oc);
                int32_t raw_sum_dn = raw_skip_dn + residual_dn;
                out[p][oc] = clamp_i32(raw_sum_dn, 0, L0_POST_BLC_MAX);
            }
        }
    }
    return out;
}

static RawFrame unpack_golden(const Raw4Frame &packed)
{
    RawFrame out(CNN_RAW_W * CNN_RAW_H);
    for (int py = 0; py < CNN_PACK_H; ++py)
    {
        for (int px = 0; px < CNN_PACK_W; ++px)
        {
            int p = pack_idx(py, px);
            out[(py * 2) * CNN_RAW_W + px * 2] = (uint16_t)packed[p][0];
            out[(py * 2) * CNN_RAW_W + px * 2 + 1] = (uint16_t)packed[p][1];
            out[(py * 2 + 1) * CNN_RAW_W + px * 2] = (uint16_t)packed[p][2];
            out[(py * 2 + 1) * CNN_RAW_W + px * 2 + 1] = (uint16_t)packed[p][3];
        }
    }
    return out;
}

static RawFrame make_raw()
{
    RawFrame raw(CNN_RAW_W * CNN_RAW_H);
    uint32_t s = 0x13579BDFu;
    for (int i = 0; i < CNN_RAW_W * CNN_RAW_H; ++i)
    {
        s ^= s << 13;
        s ^= s >> 17;
        s ^= s << 5;
        raw[i] = (uint16_t)(s % (L0_POST_BLC_MAX + 1));
    }
    raw[0] = 0;
    raw[1] = L0_POST_BLC_MAX;
    return raw;
}

static RawFrame golden_network(const RawFrame &raw)
{
    Feat16Frame l0 = golden_head(raw);
    Feat16Frame l1 = golden_conv1_u8(l0, L1_WBANK, L1_BIAS, L1_TO_CONV2_Q31, L1_TO_CONV2_EXP, L2_INPUT_QMIN, L2_INPUT_QMAX);
    Feat16Frame l2 = golden_conv2_add_next(l1, l0, L2_WBANK, L2_BIAS, L2_MAIN_TO_SKIP_Q31, L2_MAIN_TO_SKIP_EXP, L2_RESIDUAL_TO_L3_INPUT_Q31, L2_RESIDUAL_TO_L3_INPUT_EXP, L2_RESIDUAL_TO_L3_INPUT_QMIN, L2_RESIDUAL_TO_L3_INPUT_QMAX);
    Feat16Frame l3 = golden_conv1_u8(l2, L3_WBANK, L3_BIAS, L3_TO_CONV2_Q31, L3_TO_CONV2_EXP, L4_INPUT_QMIN, L4_INPUT_QMAX);
    Feat16Frame l4 = golden_conv2_add_next(l3, l2, L4_WBANK, L4_BIAS, L4_MAIN_TO_SKIP_Q31, L4_MAIN_TO_SKIP_EXP, L4_RESIDUAL_TO_L5_INPUT_Q31, L4_RESIDUAL_TO_L5_INPUT_EXP, L4_RESIDUAL_TO_L5_INPUT_QMIN, L4_RESIDUAL_TO_L5_INPUT_QMAX);
    Raw4Frame packed = golden_tail(raw, l4);
    return unpack_golden(packed);
}

int main()
{
    RawFrame raw = make_raw();
    RawFrame expected = golden_network(raw);
    hls::stream<axis_raw10_t> raw_in("raw_in");
    hls::stream<axis_raw10_t> raw_out("raw_out");
    for (int i = 0; i < CNN_RAW_W * CNN_RAW_H; ++i)
    {
        int x = i % CNN_RAW_W;
        axis_raw10_t p;
        p.data = raw[i];
        p.user = (i == 0) ? 1 : 0;
        p.last = (x == CNN_RAW_W - 1) ? 1 : 0;
        raw_in.write(p);
    }
    local_resnet_micro_top(raw_in, raw_out);
    long long mismatches = 0;
    for (int i = 0; i < CNN_RAW_W * CNN_RAW_H; ++i)
    {
        axis_raw10_t got = raw_out.read();
        int32_t actual = (int32_t)got.data;
        int32_t golden = (int32_t)expected[i];
        if (actual != golden)
        {
            if (mismatches < 32)
                std::cout << "[MISMATCH] i=" << i << " actual=" << actual << " expected=" << golden << std::endl;
            ++mismatches;
        }
    }
    std::cout << "[TB] checked=" << CNN_RAW_W * CNN_RAW_H << " mismatches=" << mismatches << std::endl;
    std::cout << ((mismatches == 0) ? "[PASS] W8A8 v9 HLS arithmetic matches golden." : "[FAIL] HLS arithmetic mismatch.") << std::endl;
    return (mismatches == 0) ? 0 : 1;
}
