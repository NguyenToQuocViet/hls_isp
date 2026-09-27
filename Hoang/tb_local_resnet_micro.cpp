#include "isp_cnn_denoise.h"
#include "isp_cnn_denoise_params.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

// ============================================================================
// Compilation & Simulation Flags dùng chung
// ============================================================================

#define COSIM_FAST_TOP

#ifndef TB_READ_TIMEOUT_LIMIT
#define TB_READ_TIMEOUT_LIMIT 10000000ULL
#endif

#ifndef TB_SAMPLE_LOG_LIMIT
#define TB_SAMPLE_LOG_LIMIT 12
#endif

#ifndef TB_MAX_MISMATCH_LOG
#define TB_MAX_MISMATCH_LOG 64
#endif

static const int TB_FRAMES = 2; // Luôn kiểm tra 2 frame liên tục

using RawFrame = std::vector<uint16_t>;
using Feat16Pixel = std::array<int8_t, CNN_C>;
using Feat16Frame = std::vector<Feat16Pixel>;
using Feat4Pixel = std::array<int8_t, 4>;
using Feat4Frame = std::vector<Feat4Pixel>;

struct LayerStats
{
    uint64_t hash = 1469598103934665603ULL;
    long long tokens = 0;
    long long user_count = 0;
    long long last_count = 0;
    long long sideband_errors = 0;
    int min_v = 999;
    int max_v = -999;
};

struct CheckStats
{
    long long checked_values = 0;
    long long mismatches = 0;
};

static std::ofstream g_log;
static int g_mismatch_printed = 0;

static void log_line(const std::string &s)
{
    std::cout << s << std::endl;
    if (g_log.is_open())
    {
        g_log << s << std::endl;
        g_log.flush();
    }
}

// ============================================================================
// Helpers dùng chung (Handshake, Băm, Math, Quantization)
// ============================================================================
template <typename T>
static bool safe_read(
    hls::stream<T> &s,
    T &out_val,
    const std::string &layer,
    int frame,
    int pixel_idx,
    int total_pixels)
{
    uint64_t wait_count = 0;
    while (s.empty())
    {
        if (++wait_count > TB_READ_TIMEOUT_LIMIT)
        {
            std::ostringstream err;
            err << "\n[DEADLOCK TIMEOUT] Layer [" << layer << "][F" << frame << "] Treo tai pixel " << pixel_idx << "/" << total_pixels << " (DUT khong day tiep du lieu). Dung chuong trinh!";
            log_line(err.str());
            return false;
        }
    }
    out_val = s.read();
    return true;
}

static inline uint32_t xorshift32(uint32_t &s)
{
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return s;
}

static inline void hash_byte(LayerStats &st, uint8_t v)
{
    st.hash ^= static_cast<uint64_t>(v);
    st.hash *= 1099511628211ULL;
}

static inline int8_t u8_to_i8(ap_uint<8> b)
{
    ap_int<8> s = b;
    return static_cast<int8_t>(static_cast<int>(s));
}

static inline ap_uint<8> i8_to_u8(int8_t v)
{
    ap_int<8> s = v;
    return static_cast<ap_uint<8>>(s);
}

static int32_t tb_q31_round(int32_t x, int32_t q31, int8_t exp)
{
    ap_int<64> product = static_cast<ap_int<64>>(x) * static_cast<ap_int<64>>(q31);
    int shift = 31 - static_cast<int>(exp);

    if (shift <= 0)
        return static_cast<int32_t>(static_cast<ap_int<32>>(product << (-shift)));
    if (shift >= 63)
        return 0;

    bool neg = product[63];
    ap_uint<64> mag = neg ? static_cast<ap_uint<64>>(-product) : static_cast<ap_uint<64>>(product);
    ap_uint<64> q = mag >> shift;
    ap_uint<64> r = mag & ((static_cast<ap_uint<64>>(1) << shift) - 1);
    ap_uint<64> half = static_cast<ap_uint<64>>(1) << (shift - 1);

    if ((r > half) || ((r == half) && ((q & 1) != 0)))
        ++q;

    ap_int<64> rounded = neg ? static_cast<ap_int<64>>(-static_cast<ap_int<64>>(q)) : static_cast<ap_int<64>>(q);
    return static_cast<int32_t>(static_cast<ap_int<32>>(rounded));
}

static inline int8_t tb_sat8(int32_t x)
{
    if (x > 127)
        return 127;
    if (x < -127)
        return -127;
    return static_cast<int8_t>(x);
}

static inline int8_t tb_quantize_raw(uint16_t raw)
{
    int32_t v = (raw > L0_POST_BLC_MAX) ? L0_POST_BLC_MAX : raw;
    int32_t q = tb_q31_round(v, static_cast<int32_t>(L0_RAW_Q31), static_cast<int8_t>(L0_RAW_EXP));
    if (q < 0)
        q = 0;
    if (q > 127)
        q = 127;
    return static_cast<int8_t>(q);
}

static inline int8_t l0_weight_at(int k, int ic, int oc)
{
    return u8_to_i8(L0_WBANK[k][ic].range(oc * 8 + 7, oc * 8));
}

static RawFrame make_raw_frame_sized(uint32_t seed, int frame_id, int total_raw)
{
    RawFrame f(total_raw);
    uint32_t s = seed;
    for (int i = 0; i < total_raw; ++i)
    {
        uint32_t r = xorshift32(s);
        uint16_t v = static_cast<uint16_t>(r % (L0_POST_BLC_MAX + 1));
        if ((i % 131071) == 0)
            v = 0;
        if ((i % 131071) == 1)
            v = L0_POST_BLC_MAX;
        f[i] = v;
    }
    std::ostringstream os;
    os << "[TB] Generated frame F" << frame_id << " (" << total_raw << " pixels) seed=0x" << std::hex << seed << std::dec;
    log_line(os.str());
    return f;
}

static void push_raw_frame_axis_sized(const RawFrame &f, hls::stream<axis_raw10_t> &s, int width, int height)
{
    int total_raw = width * height;
    for (int i = 0; i < total_raw; ++i)
    {
        int y = i / width;
        int x = i - y * width;

        axis_raw10_t p;
        p.data = f[i];
        p.user = (i == 0) ? 1 : 0;
        p.last = (x == width - 1) ? 1 : 0;
        s.write(p);
    }
}

// Bắt và kiểm tra sideband của stream 128-bit
static Feat16Frame capture_feat16_sized(
    hls::stream<IspPixelPacket<128>> &s,
    const std::string &layer,
    int frame,
    int pack_w,
    int pack_h,
    LayerStats &st)
{
    int total_pack = pack_w * pack_h;
    Feat16Frame out(total_pack);

    for (int p = 0; p < total_pack; ++p)
    {
        IspPixelPacket<128> v;
        if (!safe_read(s, v, layer, frame, p, total_pack))
        {
            if (g_log.is_open())
                g_log.close();
            std::exit(1);
        }

        int y = p / pack_w;
        int x = p - y * pack_w;

        ++st.tokens;
        st.user_count += static_cast<int>(v.user);
        st.last_count += static_cast<int>(v.last);

        bool expected_user = (p == 0);
        bool expected_last = (x == pack_w - 1);
        if (static_cast<bool>(v.user) != expected_user || static_cast<bool>(v.last) != expected_last)
        {
            ++st.sideband_errors;
        }

        for (int c = 0; c < CNN_C; ++c)
        {
            int8_t q = u8_to_i8(v.data.range(c * 8 + 7, c * 8));
            out[p][c] = q;
            int iv = static_cast<int>(q);
            if (iv < st.min_v)
                st.min_v = iv;
            if (iv > st.max_v)
                st.max_v = iv;
            hash_byte(st, static_cast<uint8_t>(q));
        }
    }
    return out;
}

// Bắt và kiểm tra sideband của stream global skip 32-bit
static Feat4Frame capture_feat4_sized(
    hls::stream<IspPixelPacket<32>> &s,
    const std::string &layer,
    int frame,
    int pack_w,
    int pack_h,
    LayerStats &st)
{
    int total_pack = pack_w * pack_h;
    Feat4Frame out(total_pack);

    for (int p = 0; p < total_pack; ++p)
    {
        IspPixelPacket<32> v;
        if (!safe_read(s, v, layer, frame, p, total_pack))
        {
            if (g_log.is_open())
                g_log.close();
            std::exit(1);
        }

        int y = p / pack_w;
        int x = p - y * pack_w;

        ++st.tokens;
        st.user_count += static_cast<int>(v.user);
        st.last_count += static_cast<int>(v.last);

        bool expected_user = (p == 0);
        bool expected_last = (x == pack_w - 1);
        if (static_cast<bool>(v.user) != expected_user || static_cast<bool>(v.last) != expected_last)
        {
            ++st.sideband_errors;
        }

        for (int c = 0; c < 4; ++c)
        {
            int8_t q = u8_to_i8(v.data.range(c * 8 + 7, c * 8));
            out[p][c] = q;
            int iv = static_cast<int>(q);
            if (iv < st.min_v)
                st.min_v = iv;
            if (iv > st.max_v)
                st.max_v = iv;
            hash_byte(st, static_cast<uint8_t>(q));
        }
    }
    return out;
}

static int8_t raw_packed_q_sized(const RawFrame &raw, int py, int px, int ch, int pack_w, int pack_h, int raw_w)
{
    if (py < 0 || py >= pack_h || px < 0 || px >= pack_w)
        return 0;
    int ry = (py << 1) + ((ch >= 2) ? 1 : 0);
    int rx = (px << 1) + ((ch == 1 || ch == 3) ? 1 : 0);
    return tb_quantize_raw(raw[ry * raw_w + rx]);
}

static int8_t golden_l0_value_sized(const RawFrame &raw, int y, int x, int oc, int pack_w, int pack_h, int raw_w)
{
    int32_t acc = static_cast<int32_t>(L0_BIAS[oc]);
    for (int ic = 0; ic < 4; ++ic)
    {
        for (int ky = 0; ky < 3; ++ky)
        {
            for (int kx = 0; kx < 3; ++kx)
            {
                int k = ky * 3 + kx;
                int8_t xv = raw_packed_q_sized(raw, y + ky - 1, x + kx - 1, ic, pack_w, pack_h, raw_w);
                int8_t wv = l0_weight_at(k, ic, oc);
                acc += static_cast<int32_t>(xv) * static_cast<int32_t>(wv);
            }
        }
    }
    if (acc < 0)
        acc = 0;
    return tb_sat8(tb_q31_round(acc, static_cast<int32_t>(L0_HEAD_Q31[oc]), static_cast<int8_t>(L0_HEAD_EXP[oc])));
}

// ============================================================================
// MODE: TEST_L0_STREAMING (Test cnn_l0_streaming độc lập với kích thước nhỏ)
// ============================================================================
#if defined(TEST_L0_STREAMING)

static const int L0_TEST_RAW_W = 64;
static const int L0_TEST_RAW_H = 32;
static const int L0_TEST_PACK_W = L0_TEST_RAW_W >> 1; // 32
static const int L0_TEST_PACK_H = L0_TEST_RAW_H >> 1; // 16
static const int L0_TEST_TOTAL_RAW = L0_TEST_RAW_W * L0_TEST_RAW_H;
static const int L0_TEST_TOTAL_PACK = L0_TEST_PACK_W * L0_TEST_PACK_H;
int main()
{
    g_log.open("tb_test_l0_streaming_2frames.log", std::ios::out | std::ios::trunc);

    log_line("============================================================");
    log_line(" LocalResNet-Micro TESTBENCH: cnn_l0_streaming");
    log_line(" Mode: TEST_L0_STREAMING (Scaled Fast Verification)");
    log_line(" Dimensions: RAW " + std::to_string(L0_TEST_RAW_W) + "x" + std::to_string(L0_TEST_RAW_H) + " -> PACK " + std::to_string(L0_TEST_PACK_W) + "x" + std::to_string(L0_TEST_PACK_H));
    log_line("============================================================");

    RawFrame frames[TB_FRAMES] = {
        make_raw_frame_sized(0x13579BDFu, 0, L0_TEST_TOTAL_RAW),
        make_raw_frame_sized(0x2468ACE1u, 1, L0_TEST_TOTAL_RAW)};

    hls::stream<axis_raw10_t> stream_in("l0_tb_axis_in");
    hls::stream<IspPixelPacket<128>> stream_out("l0_tb_stream_out");
    hls::stream<IspPixelPacket<32>> global_skip_out("l0_tb_gs_out");

    long long total_mismatches = 0;

    for (int f = 0; f < TB_FRAMES; ++f)
    {
        log_line("\n>>> [TB] Dang nap Frame F" + std::to_string(f) + " vao cnn_l0_streaming...");

        push_raw_frame_axis_sized(frames[f], stream_in, L0_TEST_RAW_W, L0_TEST_RAW_H);

        // Gọi module DUT cần test với size nhỏ
        cnn_l0_streaming(stream_in, stream_out, global_skip_out, L0_TEST_RAW_H, L0_TEST_RAW_W);

        // Capture và kiểm tra tín hiệu sideband (user/last/token count)
        LayerStats st_l0, st_gs;
        Feat16Frame l0_out = capture_feat16_sized(stream_out, "L0_OUT", f, L0_TEST_PACK_W, L0_TEST_PACK_H, st_l0);
        Feat4Frame gs_out = capture_feat4_sized(global_skip_out, "GS_OUT", f, L0_TEST_PACK_W, L0_TEST_PACK_H, st_gs);
        std::ostringstream os_l0, os_gs;
        os_l0 << "[CHECK][F" << f << "][L0_STREAM] tokens=" << st_l0.tokens << "/" << L0_TEST_TOTAL_PACK << " user=" << st_l0.user_count << " (exp: 1)" << " last=" << st_l0.last_count << "/" << L0_TEST_PACK_H << " sideband_err=" << st_l0.sideband_errors;
        log_line(os_l0.str());

        os_gs << "[CHECK][F" << f << "][GLOBAL_SKIP] tokens=" << st_gs.tokens << "/" << L0_TEST_TOTAL_PACK << " user=" << st_gs.user_count << " (exp: 1)" << " last=" << st_gs.last_count << "/" << L0_TEST_PACK_H << " sideband_err=" << st_gs.sideband_errors;
        log_line(os_gs.str());

        total_mismatches += (st_l0.sideband_errors + st_gs.sideband_errors);

        // Kiểm tra đối chiếu giá trị Golden Model cho L0
        long long l0_val_err = 0;
        for (int y = 0; y < L0_TEST_PACK_H; ++y)
        {
            for (int x = 0; x < L0_TEST_PACK_W; ++x)
            {
                int p = y * L0_TEST_PACK_W + x;
                for (int c = 0; c < CNN_C; ++c)
                {
                    int a = l0_out[p][c];
                    int e = golden_l0_value_sized(frames[f], y, x, c, L0_TEST_PACK_W, L0_TEST_PACK_H, L0_TEST_RAW_W);
                    if (a != e)
                    {
                        ++l0_val_err;
                    }
                }
            }
        }

        // Kiểm tra đối chiếu Global Skip
        long long gs_val_err = 0;
        for (int y = 0; y < L0_TEST_PACK_H; ++y)
        {
            for (int x = 0; x < L0_TEST_PACK_W; ++x)
            {
                int p = y * L0_TEST_PACK_W + x;
                for (int c = 0; c < 4; ++c)
                {
                    int a = gs_out[p][c];
                    int e = raw_packed_q_sized(frames[f], y, x, c, L0_TEST_PACK_W, L0_TEST_PACK_H, L0_TEST_RAW_W);
                    if (a != e)
                    {
                        ++gs_val_err;
                    }
                }
            }
        }

        log_line("[CHECK][F" + std::to_string(f) + "] L0 Value Mismatches = " + std::to_string(l0_val_err));
        log_line("[CHECK][F" + std::to_string(f) + "] GS Value Mismatches = " + std::to_string(gs_val_err));
        total_mismatches += (l0_val_err + gs_val_err);

        if (st_l0.tokens == L0_TEST_TOTAL_PACK && st_l0.user_count == 1 &&
            st_l0.last_count == L0_TEST_PACK_H && l0_val_err == 0 && gs_val_err == 0)
        {
            log_line("[RESULT][F" + std::to_string(f) + "] PASS: Frame hoan tat dung chuan.");
        }
        else
        {
            log_line("[RESULT][F" + std::to_string(f) + "] FAIL: Phat hien loi trong frame nay!");
        }
    }

    log_line("============================================================");
    if (total_mismatches == 0)
    {
        log_line("[TESTBENCH][PASS] cnn_l0_streaming CHAY DUNG 2 FRAMES LIEN TUC!");
    }
    else
    {
        log_line("[TESTBENCH][FAIL] TONG SO LOI: " + std::to_string(total_mismatches));
    }
    log_line("============================================================");

    if (g_log.is_open())
        g_log.close();
    return (total_mismatches == 0) ? 0 : 1;
}

// ============================================================================
// MODE: COSIM_FAST_TOP (Chạy 2 frame Full-HD qua hàm Top phục vụ Co-Sim)
// ============================================================================
#elif defined(COSIM_FAST_TOP)

static RawFrame capture_raw_axis_fast(hls::stream<axis_raw10_t> &s, int frame, LayerStats &st)
{
    RawFrame out(TB_RAW_PIXELS);
    for (int p = 0; p < TB_RAW_PIXELS; ++p)
    {
        axis_raw10_t v;
        if (!safe_read(s, v, "TOP_RAW_OUT", frame, p, TB_RAW_PIXELS))
        {
            if (g_log.is_open())
                g_log.close();
            std::exit(1);
        }
        int y = p / CNN_RAW_W;
        int x = p - y * CNN_RAW_W;
        uint16_t q = static_cast<uint16_t>(v.data.range(9, 0));
        out[p] = q;

        ++st.tokens;
        st.user_count += static_cast<int>(v.user);
        st.last_count += static_cast<int>(v.last);

        if (static_cast<bool>(v.user) != (p == 0) || static_cast<bool>(v.last) != (x == CNN_RAW_W - 1))
        {
            ++st.sideband_errors;
        }
    }
    return out;
}

int main()
{
    g_log.open("tb_cosim_fast_top_2frames.log", std::ios::out | std::ios::trunc);
    log_line("============================================================");
    log_line(" LocalResNet-Micro COSIM TOP 2-FRAME FULL-HD TESTBENCH");
    log_line(" Mode: COSIM_FAST_TOP");
    log_line("============================================================");

    RawFrame frames[TB_FRAMES] = {
        make_raw_frame_sized(0x13579BDFu, 0, TB_RAW_PIXELS),
        make_raw_frame_sized(0x2468ACE1u, 1, TB_RAW_PIXELS)};

    hls::stream<axis_raw10_t> raw_in("full_raw_in");
    hls::stream<axis_raw10_t> raw_out("full_raw_out");
    long long total_mismatches = 0;

    // Queue 2 frames lien tuc vao CUNG mot input stream truoc khi DUT bat dau xu ly.
    // Bien gioi tren stream:
    //   ... F0 pixel cuoi -> F1 pixel dau ...
    // Khong co khoang refill input tu phia testbench giua 2 frame.
    log_line("[TB] Pushing Frame F0 continuously to input stream...");
    push_raw_frame_axis_sized(frames[0], raw_in, CNN_RAW_W, CNN_RAW_H);

    log_line("[TB] Pushing Frame F1 immediately after F0...");
    push_raw_frame_axis_sized(frames[1], raw_in, CNN_RAW_W, CNN_RAW_H);

    log_line("[TB] Total queued RAW pixels = " + std::to_string(raw_in.size()) + " (expected " + std::to_string(TB_FRAMES * TB_RAW_PIXELS) + ")");

    // Top hien tai van consume 1 frame moi invocation.
    // Goi 2 lan tren CUNG raw_in/raw_out, nhung KHONG push them du lieu o giua.
    for (int f = 0; f < TB_FRAMES; ++f)
    {
        log_line("[TB] Running Top for Frame F" + std::to_string(f) + "...");

        local_resnet_micro_top(raw_in, raw_out);

        LayerStats st;
        RawFrame got = capture_raw_axis_fast(raw_out, f, st);
        total_mismatches += st.sideband_errors;

        if (st.tokens != TB_RAW_PIXELS || st.user_count != 1 || st.last_count != CNN_RAW_H)
        {
            ++total_mismatches;
            log_line("[CHECK][F" + std::to_string(f) + "] FAIL: Sideband hoac so pixel khong hop le!");
        }
        else
        {
            log_line("[CHECK][F" + std::to_string(f) + "] PASS: Frame stream thanh cong.");
        }

        log_line("[TB] Remaining RAW input tokens after F" + std::to_string(f) + " = " + std::to_string(raw_in.size()));
        RawFrame().swap(got);
    }

    if (!raw_in.empty())
    {
        ++total_mismatches;
        log_line("[CHECK] FAIL: Input stream van con " + std::to_string(raw_in.size()) + " RAW pixels sau 2 frame.");
    }
    else
    {
        log_line("[CHECK] PASS: Input stream da consume het ca 2 frame.");
    }

    if (g_log.is_open())
        g_log.close();
    return (total_mismatches == 0) ? 0 : 1;
}

// ============================================================================
// MODE: RUN_ALL_LAYER (Chạy layer-by-layer kiểm tra chi tiết toán học)
// ============================================================================
#elif defined(RUN_ALL_LAYER)

// ============================================================================
// RUN_ALL_LAYER: Golden-model verification for L0 -> L9
// ============================================================================

static inline int8_t body_weight_at(
    const body_weight_word_t weights[9][CNN_C],
    int k, int ic, int oc)
{
    return u8_to_i8(weights[k][ic].range(oc * 8 + 7, oc * 8));
}

static inline int8_t tail_weight_at(int k, int ic, int oc)
{
    return u8_to_i8(L9_WBANK[k][ic].range(oc * 8 + 7, oc * 8));
}

static inline int pack_index(int y, int x)
{
    return y * CNN_PACK_W + x;
}

static inline int8_t feat16_at(
    const Feat16Frame &f,
    int y, int x, int c)
{
    if (y < 0 || y >= CNN_PACK_H || x < 0 || x >= CNN_PACK_W)
        return 0;

    return f[pack_index(y, x)][c];
}

// ============================================================================
// Golden arithmetic
// ============================================================================

static int32_t body_conv_acc(
    const Feat16Frame &in,
    const body_weight_word_t weights[9][CNN_C],
    const bias_t bias[CNN_C],
    int y, int x, int oc)
{
    int32_t acc = static_cast<int32_t>(bias[oc]);

    for (int ic = 0; ic < CNN_C; ++ic)
    {
        for (int ky = 0; ky < 3; ++ky)
        {
            for (int kx = 0; kx < 3; ++kx)
            {
                const int k = ky * 3 + kx;
                const int8_t xv = feat16_at(in, y + ky - 1, x + kx - 1, ic);
                const int8_t wv = body_weight_at(weights, k, ic, oc);

                acc += static_cast<int32_t>(xv) * static_cast<int32_t>(wv);
            }
        }
    }

    return acc;
}

static int8_t golden_body_conv1_value(
    const Feat16Frame &in,
    const body_weight_word_t weights[9][CNN_C],
    const bias_t bias[CNN_C],
    const q31_t q31[CNN_C],
    const qexp_t exp[CNN_C],
    int y, int x, int oc)
{
    int32_t acc = body_conv_acc(in, weights, bias, y, x, oc);

    // DUT relu_requant16(): ReLU BEFORE requantization
    if (acc < 0)
        acc = 0;

    return tb_sat8(tb_q31_round(acc, static_cast<int32_t>(q31[oc]), static_cast<int8_t>(exp[oc])));
}

static int8_t golden_body_conv2_value(
    const Feat16Frame &in,
    const Feat16Frame &skip,
    const body_weight_word_t weights[9][CNN_C],
    const bias_t bias[CNN_C],
    const q31_t main_q31[CNN_C],
    const qexp_t main_exp[CNN_C],
    q31_t skip_q31,
    qexp_t skip_exp,
    int y, int x, int oc)
{
    const int32_t acc = body_conv_acc(in, weights, bias, y, x, oc);
    const int32_t main_units = tb_q31_round(acc, static_cast<int32_t>(main_q31[oc]), static_cast<int8_t>(main_exp[oc]));
    const int32_t skip_units = tb_q31_round(static_cast<int32_t>(skip[pack_index(y, x)][oc]), static_cast<int32_t>(skip_q31), static_cast<int8_t>(skip_exp));

    return tb_sat8(main_units + skip_units);
}

static int8_t golden_tail_value(
    const Feat16Frame &in,
    const Feat4Frame &global_skip,
    int y, int x, int oc)
{
    int32_t acc = static_cast<int32_t>(L9_BIAS[oc]);

    for (int ic = 0; ic < CNN_C; ++ic)
    {
        for (int ky = 0; ky < 3; ++ky)
        {
            for (int kx = 0; kx < 3; ++kx)
            {
                const int k = ky * 3 + kx;
                const int8_t xv = feat16_at(in, y + ky - 1, x + kx - 1, ic);
                const int8_t wv = tail_weight_at(k, ic, oc);

                acc += static_cast<int32_t>(xv) * static_cast<int32_t>(wv);
            }
        }
    }

    const int32_t main_units = tb_q31_round(acc, static_cast<int32_t>(L9_TAIL_Q31[oc]), static_cast<int8_t>(L9_TAIL_EXP[oc]));
    const int32_t skip_units = tb_q31_round(static_cast<int32_t>(global_skip[pack_index(y, x)][oc]), static_cast<int32_t>(L9_GLOBAL_SKIP_Q31), static_cast<int8_t>(L9_GLOBAL_SKIP_EXP));

    return tb_sat8(main_units + skip_units);
}

// ============================================================================
// Stream push helpers
// ============================================================================

static void push_raw_frame(
    const RawFrame &f,
    hls::stream<IspPixelPacket<10>> &s)
{
    for (int i = 0; i < TB_RAW_PIXELS; ++i)
    {
        const int y = i / CNN_RAW_W;
        const int x = i - y * CNN_RAW_W;

        IspPixelPacket<10> p;
        p.data = f[i];
        p.user = (i == 0) ? 1 : 0;
        p.last = (x == CNN_RAW_W - 1) ? 1 : 0;

        s.write(p);
    }
}

static void push_feat16_frame(
    const Feat16Frame &f,
    hls::stream<IspPixelPacket<128>> &s)
{
    for (int p = 0; p < TB_PACK_PIXELS; ++p)
    {
        const int x = p % CNN_PACK_W;

        IspPixelPacket<128> v;
        v.data = 0;

        for (int c = 0; c < CNN_C; ++c)
        {
            v.data.range(c * 8 + 7, c * 8) = i8_to_u8(f[p][c]);
        }

        v.user = (p == 0) ? 1 : 0;
        v.last = (x == CNN_PACK_W - 1) ? 1 : 0;

        s.write(v);
    }
}

static void push_feat4_frame(
    const Feat4Frame &f,
    hls::stream<IspPixelPacket<32>> &s)
{
    for (int p = 0; p < TB_PACK_PIXELS; ++p)
    {
        const int x = p % CNN_PACK_W;

        IspPixelPacket<32> v;
        v.data = 0;

        for (int c = 0; c < 4; ++c)
        {
            v.data.range(c * 8 + 7, c * 8) = i8_to_u8(f[p][c]);
        }

        v.user = (p == 0) ? 1 : 0;
        v.last = (x == CNN_PACK_W - 1) ? 1 : 0;

        s.write(v);
    }
}

// ============================================================================
// Verification helpers
// ============================================================================

static void log_value_mismatch(
    const std::string &layer,
    int frame,
    int y,
    int x,
    int c,
    int actual,
    int expected)
{
    if (g_mismatch_printed >= TB_MAX_MISMATCH_LOG)
        return;

    std::ostringstream os;
    os << "[MISMATCH]" << "[F" << frame << "]" << "[" << layer << "]" << " y=" << y << " x=" << x << " c=" << c << " actual=" << actual << " expected=" << expected << " diff=" << (actual - expected);

    log_line(os.str());
    ++g_mismatch_printed;
}

static long long check_l0_against_golden(
    const RawFrame &raw,
    const Feat16Frame &actual,
    int frame)
{
    long long mismatches = 0;

    for (int y = 0; y < CNN_PACK_H; ++y)
    {
        for (int x = 0; x < CNN_PACK_W; ++x)
        {
            const int p = pack_index(y, x);

            for (int c = 0; c < CNN_C; ++c)
            {
                const int a = static_cast<int>(actual[p][c]);
                const int e = static_cast<int>(golden_l0_value_sized(raw, y, x, c, CNN_PACK_W, CNN_PACK_H, CNN_RAW_W));

                if (a != e)
                {
                    ++mismatches;
                    log_value_mismatch("L0_HEAD", frame, y, x, c, a, e);
                }
            }
        }
    }

    return mismatches;
}

static long long check_global_skip_against_golden(
    const RawFrame &raw,
    const Feat4Frame &actual,
    int frame)
{
    long long mismatches = 0;

    for (int y = 0; y < CNN_PACK_H; ++y)
    {
        for (int x = 0; x < CNN_PACK_W; ++x)
        {
            const int p = pack_index(y, x);

            for (int c = 0; c < 4; ++c)
            {
                const int a = static_cast<int>(actual[p][c]);
                const int e = static_cast<int>(raw_packed_q_sized(raw, y, x, c, CNN_PACK_W, CNN_PACK_H, CNN_RAW_W));

                if (a != e)
                {
                    ++mismatches;
                    log_value_mismatch("GLOBAL_SKIP", frame, y, x, c, a, e);
                }
            }
        }
    }

    return mismatches;
}

static long long check_conv1_against_golden(
    const Feat16Frame &actual,
    const Feat16Frame &input,
    const body_weight_word_t weights[9][CNN_C],
    const bias_t bias[CNN_C],
    const q31_t q31[CNN_C],
    const qexp_t exp[CNN_C],
    const std::string &layer,
    int frame)
{
    long long mismatches = 0;

    for (int y = 0; y < CNN_PACK_H; ++y)
    {
        for (int x = 0; x < CNN_PACK_W; ++x)
        {
            const int p = pack_index(y, x);

            for (int c = 0; c < CNN_C; ++c)
            {
                const int a = static_cast<int>(actual[p][c]);
                const int e = static_cast<int>(golden_body_conv1_value(input, weights, bias, q31, exp, y, x, c));

                if (a != e)
                {
                    ++mismatches;
                    log_value_mismatch(layer, frame, y, x, c, a, e);
                }
            }
        }
    }

    return mismatches;
}

static long long check_conv2_against_golden(
    const Feat16Frame &actual,
    const Feat16Frame &input,
    const Feat16Frame &skip,
    const body_weight_word_t weights[9][CNN_C],
    const bias_t bias[CNN_C],
    const q31_t main_q31[CNN_C],
    const qexp_t main_exp[CNN_C],
    q31_t skip_q31,
    qexp_t skip_exp,
    const std::string &layer,
    int frame)
{
    long long mismatches = 0;

    for (int y = 0; y < CNN_PACK_H; ++y)
    {
        for (int x = 0; x < CNN_PACK_W; ++x)
        {
            const int p = pack_index(y, x);

            for (int c = 0; c < CNN_C; ++c)
            {
                const int a = static_cast<int>(actual[p][c]);
                const int e = static_cast<int>(golden_body_conv2_value(input, skip, weights, bias, main_q31, main_exp, skip_q31, skip_exp, y, x, c));

                if (a != e)
                {
                    ++mismatches;
                    log_value_mismatch(layer, frame, y, x, c, a, e);
                }
            }
        }
    }

    return mismatches;
}

static long long check_tail_against_golden(
    const Feat4Frame &actual,
    const Feat16Frame &input,
    const Feat4Frame &global_skip,
    int frame)
{
    long long mismatches = 0;

    for (int y = 0; y < CNN_PACK_H; ++y)
    {
        for (int x = 0; x < CNN_PACK_W; ++x)
        {
            const int p = pack_index(y, x);

            for (int c = 0; c < 4; ++c)
            {
                const int a = static_cast<int>(actual[p][c]);
                const int e = static_cast<int>(golden_tail_value(input, global_skip, y, x, c));

                if (a != e)
                {
                    ++mismatches;
                    log_value_mismatch("L9_TAIL", frame, y, x, c, a, e);
                }
            }
        }
    }

    return mismatches;
}

static long long check_feat16_sideband(
    const LayerStats &st,
    const std::string &layer,
    int frame)
{
    long long err = st.sideband_errors;

    if (st.tokens != TB_PACK_PIXELS)
    {
        ++err;
    }

    if (st.user_count != 1)
    {
        ++err;
    }

    if (st.last_count != CNN_PACK_H)
    {
        ++err;
    }

    std::ostringstream os;
    os << "[STREAM]" << "[F" << frame << "]" << "[" << layer << "]" << " tokens=" << st.tokens << "/" << TB_PACK_PIXELS << " user=" << st.user_count << "/1" << " last=" << st.last_count << "/" << CNN_PACK_H << " sideband_err=" << st.sideband_errors;

    log_line(os.str());

    return err;
}

static long long check_feat4_sideband(
    const LayerStats &st,
    const std::string &layer,
    int frame)
{
    // Same packet count and sideband expectation as packed 16-channel maps.
    return check_feat16_sideband(st, layer, frame);
}

static void log_layer_result(
    const std::string &layer,
    int frame,
    long long value_mismatches,
    long long stream_errors)
{
    std::ostringstream os;

    os << "[CHECK]" << "[F" << frame << "]" << "[" << layer << "]" << " value_mismatches=" << value_mismatches << " stream_errors=" << stream_errors << " -> " << ((value_mismatches == 0 && stream_errors == 0) ? "PASS" : "FAIL");

    log_line(os.str());
}

// ============================================================================
// RUN_ALL_LAYER main
// ============================================================================

int main()
{
    g_log.open("tb_run_all_layer_golden_2frames.log", std::ios::out | std::ios::trunc);

    log_line("============================================================");
    log_line(" LocalResNet-Micro RUN_ALL_LAYER GOLDEN VERIFICATION");
    log_line(" L0 -> L1 -> L2 -> L3 -> L4 -> L5 -> L6 -> L7 -> L8 -> L9");
    log_line(" Check: bit-exact values + token/user/last sideband");
    log_line("============================================================");

    RawFrame frames[TB_FRAMES] = {
        make_raw_frame_sized(0x13579BDFu, 0, TB_RAW_PIXELS), make_raw_frame_sized(0x2468ACE1u, 1, TB_RAW_PIXELS)};

    long long total_value_mismatches = 0;
    long long total_stream_errors = 0;

    for (int f = 0; f < TB_FRAMES; ++f)
    {
        log_line("");
        log_line("============================================================");
        log_line("[FRAME] Begin F" + std::to_string(f));
        log_line("============================================================");

        long long frame_value_mismatches = 0;
        long long frame_stream_errors = 0;

        // --------------------------------------------------------------------
        // L0 HEAD + GLOBAL SKIP
        // --------------------------------------------------------------------
        hls::stream<IspPixelPacket<10>> sraw("tb_raw");
        hls::stream<IspPixelPacket<128>> shead("tb_head");
        hls::stream<IspPixelPacket<32>> sgs("tb_gs");

        push_raw_frame(frames[f], sraw);
        cnn_l0_core(sraw, shead, sgs);

        LayerStats st0;
        LayerStats stgs;
        Feat16Frame l0 = capture_feat16_sized(shead, "L0_HEAD", f, CNN_PACK_W, CNN_PACK_H, st0);
        Feat4Frame gs = capture_feat4_sized(sgs, "GLOBAL_SKIP", f, CNN_PACK_W, CNN_PACK_H, stgs);
        long long l0_stream_err = check_feat16_sideband(st0, "L0_HEAD", f);
        long long gs_stream_err = check_feat4_sideband(stgs, "GLOBAL_SKIP", f);
        long long l0_value_err = check_l0_against_golden(frames[f], l0, f);
        long long gs_value_err = check_global_skip_against_golden(frames[f], gs, f);

        log_layer_result("L0_HEAD", f, l0_value_err, l0_stream_err);

        log_layer_result("GLOBAL_SKIP", f, gs_value_err, gs_stream_err);

        frame_value_mismatches += l0_value_err + gs_value_err;

        frame_stream_errors += l0_stream_err + gs_stream_err;

        // --------------------------------------------------------------------
        // L1: Conv1 of Residual Block 0
        // Input = L0
        // --------------------------------------------------------------------
        hls::stream<IspPixelPacket<128>> s1i("s1i");
        hls::stream<IspPixelPacket<128>> s1o("s1o");

        push_feat16_frame(l0, s1i);
        cnn_l1_conv1(s1i, s1o);

        LayerStats st1;
        Feat16Frame l1 = capture_feat16_sized(s1o, "L1", f, CNN_PACK_W, CNN_PACK_H, st1);
        long long l1_stream_err = check_feat16_sideband(st1, "L1", f);
        long long l1_value_err = check_conv1_against_golden(l1, l0, L1_WBANK, L1_BIAS, L1_Q31, L1_EXP, "L1", f);

        log_layer_result("L1", f, l1_value_err, l1_stream_err);

        frame_value_mismatches += l1_value_err;
        frame_stream_errors += l1_stream_err;

        // --------------------------------------------------------------------
        // L2: Conv2 + Residual Add of Block 0
        // main input = L1, skip = L0
        // --------------------------------------------------------------------
        hls::stream<IspPixelPacket<128>> s2i("s2i");
        hls::stream<IspPixelPacket<128>> s2skip("s2skip");
        hls::stream<IspPixelPacket<128>> s2o("s2o");

        push_feat16_frame(l1, s2i);
        push_feat16_frame(l0, s2skip);

        cnn_l2_conv2_add(s2i, s2skip, s2o);

        LayerStats st2;
        Feat16Frame l2 = capture_feat16_sized(s2o, "L2", f, CNN_PACK_W, CNN_PACK_H, st2);
        long long l2_stream_err = check_feat16_sideband(st2, "L2", f);
        long long l2_value_err = check_conv2_against_golden(l2, l1, l0, L2_WBANK, L2_BIAS, L2_MAIN_Q31, L2_MAIN_EXP, L2_SKIP_Q31, L2_SKIP_EXP, "L2", f);

        log_layer_result("L2", f, l2_value_err, l2_stream_err);

        frame_value_mismatches += l2_value_err;
        frame_stream_errors += l2_stream_err;

        // L0/L1 no longer required after L2 verification.
        Feat16Frame().swap(l0);
        Feat16Frame().swap(l1);

        // --------------------------------------------------------------------
        // L3: Conv1 of Residual Block 1
        // Input = L2
        // --------------------------------------------------------------------
        hls::stream<IspPixelPacket<128>> s3i("s3i");
        hls::stream<IspPixelPacket<128>> s3o("s3o");

        push_feat16_frame(l2, s3i);
        cnn_l3_conv1(s3i, s3o);

        LayerStats st3;
        Feat16Frame l3 = capture_feat16_sized(s3o, "L3", f, CNN_PACK_W, CNN_PACK_H, st3);
        long long l3_stream_err = check_feat16_sideband(st3, "L3", f);
        long long l3_value_err = check_conv1_against_golden(l3, l2, L3_WBANK, L3_BIAS, L3_Q31, L3_EXP, "L3", f);

        log_layer_result("L3", f, l3_value_err, l3_stream_err);

        frame_value_mismatches += l3_value_err;
        frame_stream_errors += l3_stream_err;

        // --------------------------------------------------------------------
        // L4: Conv2 + Residual Add of Block 1
        // main input = L3, skip = L2
        // --------------------------------------------------------------------
        hls::stream<IspPixelPacket<128>> s4i("s4i");
        hls::stream<IspPixelPacket<128>> s4skip("s4skip");
        hls::stream<IspPixelPacket<128>> s4o("s4o");

        push_feat16_frame(l3, s4i);
        push_feat16_frame(l2, s4skip);

        cnn_l4_conv2_add(s4i, s4skip, s4o);

        LayerStats st4;
        Feat16Frame l4 = capture_feat16_sized(s4o, "L4", f, CNN_PACK_W, CNN_PACK_H, st4);
        long long l4_stream_err = check_feat16_sideband(st4, "L4", f);
        long long l4_value_err = check_conv2_against_golden(l4, l3, l2, L4_WBANK, L4_BIAS, L4_MAIN_Q31, L4_MAIN_EXP, L4_SKIP_Q31, L4_SKIP_EXP, "L4", f);

        log_layer_result("L4", f, l4_value_err, l4_stream_err);

        frame_value_mismatches += l4_value_err;
        frame_stream_errors += l4_stream_err;

        Feat16Frame().swap(l2);
        Feat16Frame().swap(l3);

        // --------------------------------------------------------------------
        // L5: Conv1 of Residual Block 2
        // Input = L4
        // --------------------------------------------------------------------
        hls::stream<IspPixelPacket<128>> s5i("s5i");
        hls::stream<IspPixelPacket<128>> s5o("s5o");

        push_feat16_frame(l4, s5i);
        cnn_l5_conv1(s5i, s5o);

        LayerStats st5;
        Feat16Frame l5 = capture_feat16_sized(s5o, "L5", f, CNN_PACK_W, CNN_PACK_H, st5);
        long long l5_stream_err = check_feat16_sideband(st5, "L5", f);
        long long l5_value_err = check_conv1_against_golden(l5, l4, L5_WBANK, L5_BIAS, L5_Q31, L5_EXP, "L5", f);

        log_layer_result("L5", f, l5_value_err, l5_stream_err);

        frame_value_mismatches += l5_value_err;
        frame_stream_errors += l5_stream_err;

        // --------------------------------------------------------------------
        // L6: Conv2 + Residual Add of Block 2
        // main input = L5, skip = L4
        // --------------------------------------------------------------------
        hls::stream<IspPixelPacket<128>> s6i("s6i");
        hls::stream<IspPixelPacket<128>> s6skip("s6skip");
        hls::stream<IspPixelPacket<128>> s6o("s6o");

        push_feat16_frame(l5, s6i);
        push_feat16_frame(l4, s6skip);

        cnn_l6_conv2_add(s6i, s6skip, s6o);

        LayerStats st6;
        Feat16Frame l6 = capture_feat16_sized(s6o, "L6", f, CNN_PACK_W, CNN_PACK_H, st6);
        long long l6_stream_err = check_feat16_sideband(st6, "L6", f);
        long long l6_value_err = check_conv2_against_golden(l6, l5, l4, L6_WBANK, L6_BIAS, L6_MAIN_Q31, L6_MAIN_EXP, L6_SKIP_Q31, L6_SKIP_EXP, "L6", f);

        log_layer_result("L6", f, l6_value_err, l6_stream_err);

        frame_value_mismatches += l6_value_err;
        frame_stream_errors += l6_stream_err;

        Feat16Frame().swap(l4);
        Feat16Frame().swap(l5);

        // --------------------------------------------------------------------
        // L7: Conv1 of Residual Block 3
        // Input = L6
        // --------------------------------------------------------------------
        hls::stream<IspPixelPacket<128>> s7i("s7i");
        hls::stream<IspPixelPacket<128>> s7o("s7o");

        push_feat16_frame(l6, s7i);
        cnn_l7_conv1(s7i, s7o);

        LayerStats st7;
        Feat16Frame l7 = capture_feat16_sized(s7o, "L7", f, CNN_PACK_W, CNN_PACK_H, st7);
        long long l7_stream_err = check_feat16_sideband(st7, "L7", f);
        long long l7_value_err = check_conv1_against_golden(l7, l6, L7_WBANK, L7_BIAS, L7_Q31, L7_EXP, "L7", f);

        log_layer_result("L7", f, l7_value_err, l7_stream_err);

        frame_value_mismatches += l7_value_err;
        frame_stream_errors += l7_stream_err;

        // --------------------------------------------------------------------
        // L8: Conv2 + Residual Add of Block 3
        // main input = L7, skip = L6
        // --------------------------------------------------------------------
        hls::stream<IspPixelPacket<128>> s8i("s8i");
        hls::stream<IspPixelPacket<128>> s8skip("s8skip");
        hls::stream<IspPixelPacket<128>> s8o("s8o");

        push_feat16_frame(l7, s8i);
        push_feat16_frame(l6, s8skip);

        cnn_l8_conv2_add(s8i, s8skip, s8o);

        LayerStats st8;
        Feat16Frame l8 = capture_feat16_sized(s8o, "L8", f, CNN_PACK_W, CNN_PACK_H, st8);
        long long l8_stream_err = check_feat16_sideband(st8, "L8", f);
        long long l8_value_err = check_conv2_against_golden(l8, l7, l6, L8_WBANK, L8_BIAS, L8_MAIN_Q31, L8_MAIN_EXP, L8_SKIP_Q31, L8_SKIP_EXP, "L8", f);

        log_layer_result("L8", f, l8_value_err, l8_stream_err);

        frame_value_mismatches += l8_value_err;
        frame_stream_errors += l8_stream_err;

        Feat16Frame().swap(l6);
        Feat16Frame().swap(l7);

        // --------------------------------------------------------------------
        // L9 TAIL
        // main input = L8, global skip = quantized RAW RGGB
        // --------------------------------------------------------------------
        hls::stream<IspPixelPacket<128>> s9i("s9i");
        hls::stream<IspPixelPacket<32>> s9gs("s9gs");
        hls::stream<IspPixelPacket<32>> s9o("s9o");

        push_feat16_frame(l8, s9i);
        push_feat4_frame(gs, s9gs);

        cnn_l9_tail(s9i, s9gs, s9o);

        LayerStats st9;
        Feat4Frame l9 = capture_feat4_sized(s9o, "L9_TAIL", f, CNN_PACK_W, CNN_PACK_H, st9);
        long long l9_stream_err = check_feat4_sideband(st9, "L9_TAIL", f);
        long long l9_value_err = check_tail_against_golden(l9, l8, gs, f);

        log_layer_result("L9_TAIL", f, l9_value_err, l9_stream_err);

        frame_value_mismatches += l9_value_err;
        frame_stream_errors += l9_stream_err;

        // --------------------------------------------------------------------
        // Frame summary
        // --------------------------------------------------------------------
        total_value_mismatches += frame_value_mismatches;

        total_stream_errors += frame_stream_errors;

        log_line("------------------------------------------------------------");

        {
            std::ostringstream os;
            os << "[FRAME RESULT]" << "[F" << f << "]" << " value_mismatches=" << frame_value_mismatches << " stream_errors=" << frame_stream_errors << " -> " << ((frame_value_mismatches == 0 && frame_stream_errors == 0) ? "PASS" : "FAIL");

            log_line(os.str());
        }

        log_line("------------------------------------------------------------");
    }

    // ========================================================================
    // Final testbench result
    // ========================================================================
    log_line("");
    log_line("============================================================");

    {
        std::ostringstream os;
        os << "[FINAL]" << " total_value_mismatches=" << total_value_mismatches << " total_stream_errors=" << total_stream_errors;

        log_line(os.str());
    }

    if (total_value_mismatches == 0 &&
        total_stream_errors == 0)
    {

        log_line("[TESTBENCH][PASS] "
                 "ALL LAYERS L0-L9 MATCH GOLDEN MODEL BIT-EXACTLY.");
    }
    else
    {

        log_line("[TESTBENCH][FAIL] "
                 "HLS OUTPUT DOES NOT MATCH GOLDEN MODEL.");
    }

    log_line("============================================================");

    if (g_log.is_open())
        g_log.close();

    return (total_value_mismatches == 0 && total_stream_errors == 0) ? 0 : 1;
}

#else
#error "TEST_L0_STREAMING, COSIM_FAST_TOP or RUN_ALL_LAYER!"
#endif
