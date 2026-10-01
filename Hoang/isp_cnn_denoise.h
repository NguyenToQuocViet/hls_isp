#ifndef LOCAL_RESNET_MICRO_H
#define LOCAL_RESNET_MICRO_H

#include <ap_int.h>
#include <ap_axi_sdata.h>
#include <hls_stream.h>

#define FREE_RUNNING 0

static const int CNN_RAW_W = 1920, CNN_RAW_H = 1080, CNN_PACK_W = CNN_RAW_W / 2, CNN_PACK_H = CNN_RAW_H / 2;
static const int TB_RAW_PIXELS = CNN_RAW_H * CNN_RAW_W;
static const int TB_PACK_PIXELS = CNN_PACK_W * CNN_PACK_H;
static const int CNN_PIXELS = CNN_PACK_W * CNN_PACK_H;
static const int CNN_C = 16, CNN_K = 3, CNN_PAR_IN = 4, CNN_GROUPS = CNN_C / CNN_PAR_IN;

static const int L0_MAX_RAW_W = CNN_RAW_W, L0_MAX_RAW_H = CNN_RAW_H, L0_MAX_PACK_W = CNN_PACK_W, L0_MAX_PACK_H = CNN_PACK_H;
static const int L0_CIN = 4, L0_COUT = 16, L0_K = 3, L0_POST_BLC_MAX = 959;

typedef ap_axiu<10, 1, 0, 0> axis_raw10_t;

template <int DATA_BITS>
struct IspPixelPacket
{
    ap_uint<DATA_BITS> data;
    ap_uint<1> user;
    ap_uint<1> last;
};

typedef ap_uint<8> feature_bits_t;
typedef ap_uint<8> ufeature_t;
typedef ap_int<8> sfeature_t;
typedef ap_int<8> weight_t;
typedef ap_int<32> acc_t;
typedef ap_int<32> bias_t;
typedef ap_int<32> q31_t;
typedef ap_int<8> qexp_t;

typedef ap_uint<128> weight_word_t;
typedef ap_uint<128> body_weight_word_t;
typedef ap_uint<32> tail_weight_word_t;

struct ch_window_t
{
    ap_uint<72> taps;
    ap_uint<2> ch;
    ap_uint<10> x;
    ap_uint<10> y;
};

struct feat16_window_t
{
    ap_uint<72> taps[CNN_C];
    ap_uint<10> x;
    ap_uint<10> y;
};

struct acc16_pkt_t
{
    ap_uint<512> data;
    ap_uint<1> user;
    ap_uint<1> last;
};

struct acc4_pkt_t
{
    ap_uint<128> data;
    ap_uint<1> user;
    ap_uint<1> last;
};

acc_t q31_round(acc_t x, q31_t q31, qexp_t exp);
ufeature_t sat_u8(acc_t x);
sfeature_t sat_s8(acc_t x);
sfeature_t bits_to_s8(ap_uint<8> b);
ufeature_t bits_to_u8(ap_uint<8> b);
ap_uint<8> s8_to_bits(sfeature_t x);
ap_uint<8> u8_to_bits(ufeature_t x);
void fork_feature16(hls::stream<IspPixelPacket<128>> &in, hls::stream<IspPixelPacket<128>> &main_out, hls::stream<IspPixelPacket<128>> &skip_out);
void feature16_to_windows_core(hls::stream<IspPixelPacket<128>> &in, hls::stream<feat16_window_t> &win_out, int instance_id);

void cnn_l0_streaming(hls::stream<axis_raw10_t> &stream_in, hls::stream<IspPixelPacket<128>> &stream_out, hls::stream<IspPixelPacket<40>> &global_skip_out, int height, int width);
void cnn_l0_axis_core(hls::stream<axis_raw10_t> &stream_in, hls::stream<IspPixelPacket<128>> &stream_out, hls::stream<IspPixelPacket<40>> &global_skip_out);
void cnn_l0_core(hls::stream<IspPixelPacket<10>> &stream_in, hls::stream<IspPixelPacket<128>> &stream_out, hls::stream<IspPixelPacket<40>> &global_skip_out);

void cnn_l1_conv1(hls::stream<IspPixelPacket<128>> &in, hls::stream<IspPixelPacket<128>> &out);
void cnn_l2_conv2_add(hls::stream<IspPixelPacket<128>> &in, hls::stream<IspPixelPacket<128>> &skip, hls::stream<IspPixelPacket<128>> &out);
void cnn_l3_conv1(hls::stream<IspPixelPacket<128>> &in, hls::stream<IspPixelPacket<128>> &out);
void cnn_l4_conv2_add(hls::stream<IspPixelPacket<128>> &in, hls::stream<IspPixelPacket<128>> &skip, hls::stream<IspPixelPacket<128>> &out);
void cnn_resblock0(hls::stream<IspPixelPacket<128>> &in, hls::stream<IspPixelPacket<128>> &out);
void cnn_resblock1(hls::stream<IspPixelPacket<128>> &in, hls::stream<IspPixelPacket<128>> &out);

void cnn_l5_tail(hls::stream<IspPixelPacket<128>> &in, hls::stream<IspPixelPacket<40>> &global_skip, hls::stream<IspPixelPacket<40>> &packed_out);
void packed4_to_raw10(hls::stream<IspPixelPacket<40>> &packed_in, hls::stream<IspPixelPacket<10>> &raw_out);

void local_resnet_micro_top(hls::stream<axis_raw10_t> &raw_in, hls::stream<axis_raw10_t> &raw_out);
void isp_cnn_denoise_top(hls::stream<IspPixelPacket<10>> &raw_in, hls::stream<IspPixelPacket<10>> &raw_out);

#endif
