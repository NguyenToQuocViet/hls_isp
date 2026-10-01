#include "isp_cnn_denoise.h"
#include "isp_cnn_denoise_params.h"

acc_t q31_round(acc_t x, q31_t q31, qexp_t exp)
{
#pragma HLS INLINE
    ap_int<64> product = (ap_int<64>)x * (ap_int<64>)q31;
    int shift = 31 - (int)exp;
    if (shift <= 0)
        return (acc_t)(product << (-shift));
    if (shift >= 63)
        return 0;
    bool neg = product.range(63, 63);
    ap_uint<64> mag = neg ? (ap_uint<64>)((ap_int<64>)-product) : (ap_uint<64>)product;
    ap_uint<64> q = mag >> shift;
    ap_uint<64> r = mag & ((((ap_uint<64>)1) << shift) - 1);
    ap_uint<64> half = ((ap_uint<64>)1) << (shift - 1);
    if ((r > half) || ((r == half) && ((q & 1) != 0)))
        ++q;
    ap_int<64> rounded = neg ? (ap_int<64>)(-(ap_int<64>)q) : (ap_int<64>)q;
    return (acc_t)rounded;
}

ufeature_t sat_u8(acc_t x)
{
#pragma HLS INLINE
    if (x > 255)
        return 255;
    if (x < 0)
        return 0;
    return (ufeature_t)x;
}

sfeature_t sat_s8(acc_t x)
{
#pragma HLS INLINE
    if (x > 127)
        return 127;
    if (x < -127)
        return -127;
    return (sfeature_t)x;
}

sfeature_t bits_to_s8(ap_uint<8> b)
{
#pragma HLS INLINE
    return (sfeature_t)b;
}

ufeature_t bits_to_u8(ap_uint<8> b)
{
#pragma HLS INLINE
    return (ufeature_t)b;
}

ap_uint<8> s8_to_bits(sfeature_t x)
{
#pragma HLS INLINE
    return x.range(7, 0);
}

ap_uint<8> u8_to_bits(ufeature_t x)
{
#pragma HLS INLINE
    return x.range(7, 0);
}

void fork_feature16(hls::stream<IspPixelPacket<128>> &in, hls::stream<IspPixelPacket<128>> &main_out,
                    hls::stream<IspPixelPacket<128>> &skip_out)
{
FORK_LOOP:
    for (int p = 0; p < CNN_PIXELS; ++p)
    {
#pragma HLS PIPELINE II = 1
        IspPixelPacket<128> v = in.read();
        main_out.write(v);
        skip_out.write(v);
    }
}

void feature16_to_windows_core(hls::stream<IspPixelPacket<128>> &in, hls::stream<feat16_window_t> &win_out, int instance_id)
{
#pragma HLS FUNCTION_INSTANTIATE variable = instance_id
    feature_bits_t lb0[CNN_C][CNN_PACK_W];
    feature_bits_t lb1[CNN_C][CNN_PACK_W];
    feature_bits_t lb2[CNN_C][CNN_PACK_W];

#pragma HLS ARRAY_PARTITION variable = lb0 complete dim = 1
#pragma HLS ARRAY_PARTITION variable = lb1 complete dim = 1
#pragma HLS ARRAY_PARTITION variable = lb2 complete dim = 1
#pragma HLS BIND_STORAGE variable = lb0 type = ram_1p impl = bram
#pragma HLS BIND_STORAGE variable = lb1 type = ram_1p impl = bram
#pragma HLS BIND_STORAGE variable = lb2 type = ram_1p impl = bram

    feature_bits_t hwin[CNN_C][3][3];
#pragma HLS ARRAY_PARTITION variable = hwin complete dim = 0

INIT_WIN:

    int y = 0, x = 0;

MAIN_INPUT:
    for (int p = 0; p < CNN_PIXELS; ++p)
    {
#pragma HLS PIPELINE II = 1
        IspPixelPacket<128> pkt = in.read();
        const int bank = y % 3;

        if (x == 0)
        {
            if (y >= 2)
            {
                feat16_window_t wp;
                wp.x = CNN_PACK_W - 1;
                wp.y = y - 2;

                for (int c = 0; c < CNN_C; ++c)
                {
#pragma HLS UNROLL
                    wp.taps[c] = 0;
                    hwin[c][0][0] = hwin[c][0][1];
                    hwin[c][0][1] = hwin[c][0][2];
                    hwin[c][0][2] = 0;

                    hwin[c][1][0] = hwin[c][1][1];
                    hwin[c][1][1] = hwin[c][1][2];
                    hwin[c][1][2] = 0;

                    hwin[c][2][0] = hwin[c][2][1];
                    hwin[c][2][1] = hwin[c][2][2];
                    hwin[c][2][2] = 0;

                    for (int ky = 0; ky < 3; ++ky)
                    {
#pragma HLS UNROLL
                        for (int kx = 0; kx < 3; ++kx)
                        {
#pragma HLS UNROLL
                            int idx = (ky * 3 + kx) * 8;
                            wp.taps[c].range(idx + 7, idx) = hwin[c][ky][kx];
                        }
                    }
                }
                win_out.write(wp);
            }

            // reset horizontal history for the new row
            for (int c = 0; c < CNN_C; ++c)
            {
#pragma HLS UNROLL
                for (int ky = 0; ky < 3; ++ky)
                {
#pragma HLS UNROLL
                    for (int kx = 0; kx < 3; ++kx)
                    {
#pragma HLS UNROLL
                        hwin[c][ky][kx] = 0;
                    }
                }
            }
        }

        feature_bits_t top[CNN_C];
        feature_bits_t mid[CNN_C];
        feature_bits_t cur[CNN_C];
#pragma HLS ARRAY_PARTITION variable = top complete
#pragma HLS ARRAY_PARTITION variable = mid complete
#pragma HLS ARRAY_PARTITION variable = cur complete

        for (int c = 0; c < CNN_C; ++c)
        {
#pragma HLS UNROLL
            cur[c] = pkt.data.range(c * 8 + 7, c * 8);

            if (bank == 0)
            {
                top[c] = (y >= 2) ? lb1[c][x] : (feature_bits_t)0;
                mid[c] = (y >= 1) ? lb2[c][x] : (feature_bits_t)0;
                lb0[c][x] = cur[c];
            }
            else if (bank == 1)
            {
                top[c] = (y >= 2) ? lb2[c][x] : (feature_bits_t)0;
                mid[c] = (y >= 1) ? lb0[c][x] : (feature_bits_t)0;
                lb1[c][x] = cur[c];
            }
            else
            {
                top[c] = (y >= 2) ? lb0[c][x] : (feature_bits_t)0;
                mid[c] = (y >= 1) ? lb1[c][x] : (feature_bits_t)0;
                lb2[c][x] = cur[c];
            }

            hwin[c][0][0] = hwin[c][0][1];
            hwin[c][0][1] = hwin[c][0][2];
            hwin[c][0][2] = top[c];

            hwin[c][1][0] = hwin[c][1][1];
            hwin[c][1][1] = hwin[c][1][2];
            hwin[c][1][2] = mid[c];

            hwin[c][2][0] = hwin[c][2][1];
            hwin[c][2][1] = hwin[c][2][2];
            hwin[c][2][2] = cur[c];
        }

        if (y >= 1 && x >= 1)
        {
            feat16_window_t wp;
            wp.x = x - 1;
            wp.y = y - 1;

            for (int c = 0; c < CNN_C; ++c)
            {
#pragma HLS UNROLL
                wp.taps[c] = 0;
                for (int ky = 0; ky < 3; ++ky)
                {
#pragma HLS UNROLL
                    for (int kx = 0; kx < 3; ++kx)
                    {
#pragma HLS UNROLL
                        int idx = (ky * 3 + kx) * 8;
                        wp.taps[c].range(idx + 7, idx) = hwin[c][ky][kx];
                    }
                }
            }
            win_out.write(wp);
        }

        if (x == CNN_PACK_W - 1)
        {
            x = 0;
            ++y;
        }
        else
        {
            ++x;
        }
    }

RIGHT_LAST:
    {
        feat16_window_t wp;
        wp.x = CNN_PACK_W - 1;
        wp.y = CNN_PACK_H - 2;

        for (int c = 0; c < CNN_C; ++c)
        {
#pragma HLS UNROLL
            wp.taps[c] = 0;
            hwin[c][0][0] = hwin[c][0][1];
            hwin[c][0][1] = hwin[c][0][2];
            hwin[c][0][2] = 0;

            hwin[c][1][0] = hwin[c][1][1];
            hwin[c][1][1] = hwin[c][1][2];
            hwin[c][1][2] = 0;

            hwin[c][2][0] = hwin[c][2][1];
            hwin[c][2][1] = hwin[c][2][2];
            hwin[c][2][2] = 0;

            for (int ky = 0; ky < 3; ++ky)
            {
#pragma HLS UNROLL
                for (int kx = 0; kx < 3; ++kx)
                {
#pragma HLS UNROLL
                    int idx = (ky * 3 + kx) * 8;
                    wp.taps[c].range(idx + 7, idx) = hwin[c][ky][kx];
                }
            }
        }
        win_out.write(wp);
    }

    for (int c = 0; c < CNN_C; ++c)
    {
#pragma HLS UNROLL
        for (int ky = 0; ky < 3; ++ky)
        {
#pragma HLS UNROLL
            for (int kx = 0; kx < 3; ++kx)
            {
#pragma HLS UNROLL
                hwin[c][ky][kx] = 0;
            }
        }
    }

    const int vbank = CNN_PACK_H % 3;

BOTTOM_X:
    for (int bx = 0; bx < CNN_PACK_W; ++bx)
    {
#pragma HLS PIPELINE II = 1
        feature_bits_t top[CNN_C];
        feature_bits_t mid[CNN_C];
#pragma HLS ARRAY_PARTITION variable = top complete
#pragma HLS ARRAY_PARTITION variable = mid complete

        for (int c = 0; c < CNN_C; ++c)
        {
#pragma HLS UNROLL
            if (vbank == 0)
            {
                top[c] = lb1[c][bx];
                mid[c] = lb2[c][bx];
            }
            else if (vbank == 1)
            {
                top[c] = lb2[c][bx];
                mid[c] = lb0[c][bx];
            }
            else
            {
                top[c] = lb0[c][bx];
                mid[c] = lb1[c][bx];
            }

            hwin[c][0][0] = hwin[c][0][1];
            hwin[c][0][1] = hwin[c][0][2];
            hwin[c][0][2] = top[c];

            hwin[c][1][0] = hwin[c][1][1];
            hwin[c][1][1] = hwin[c][1][2];
            hwin[c][1][2] = mid[c];

            hwin[c][2][0] = hwin[c][2][1];
            hwin[c][2][1] = hwin[c][2][2];
            hwin[c][2][2] = 0;
        }

        if (bx >= 1)
        {
            feat16_window_t wp;
            wp.x = bx - 1;
            wp.y = CNN_PACK_H - 1;

            for (int c = 0; c < CNN_C; ++c)
            {
#pragma HLS UNROLL
                wp.taps[c] = 0;
                for (int ky = 0; ky < 3; ++ky)
                {
#pragma HLS UNROLL
                    for (int kx = 0; kx < 3; ++kx)
                    {
#pragma HLS UNROLL
                        int idx = (ky * 3 + kx) * 8;
                        wp.taps[c].range(idx + 7, idx) = hwin[c][ky][kx];
                    }
                }
            }
            win_out.write(wp);
        }
    }

BOTTOM_RIGHT:
    {
        feat16_window_t wp;
        wp.x = CNN_PACK_W - 1;
        wp.y = CNN_PACK_H - 1;

        for (int c = 0; c < CNN_C; ++c)
        {
#pragma HLS UNROLL
            wp.taps[c] = 0;
            hwin[c][0][0] = hwin[c][0][1];
            hwin[c][0][1] = hwin[c][0][2];
            hwin[c][0][2] = 0;
            hwin[c][1][0] = hwin[c][1][1];
            hwin[c][1][1] = hwin[c][1][2];
            hwin[c][1][2] = 0;
            hwin[c][2][0] = hwin[c][2][1];
            hwin[c][2][1] = hwin[c][2][2];
            hwin[c][2][2] = 0;

            for (int ky = 0; ky < 3; ++ky)
            {
#pragma HLS UNROLL
                for (int kx = 0; kx < 3; ++kx)
                {
#pragma HLS UNROLL
                    int idx = (ky * 3 + kx) * 8;
                    wp.taps[c].range(idx + 7, idx) = hwin[c][ky][kx];
                }
            }
        }
        win_out.write(wp);
    }
}

static void internal_raw10_to_axis(hls::stream<IspPixelPacket<10>> &in, hls::stream<axis_raw10_t> &out)
{
INTERNAL_TO_AXIS:
    for (int p = 0; p < CNN_RAW_W * CNN_RAW_H; ++p)
    {
#pragma HLS PIPELINE II = 1
        IspPixelPacket<10> v = in.read();
        axis_raw10_t a;
        a.data = v.data;
        a.user = v.user;
        a.last = v.last;
        out.write(a);
    }
}

void local_resnet_micro_top(hls::stream<axis_raw10_t> &raw_in, hls::stream<axis_raw10_t> &raw_out)
{
#pragma HLS INTERFACE axis port = raw_in register = false
#pragma HLS INTERFACE axis port = raw_out
#pragma HLS INTERFACE s_axilite port = return bundle = CTRL
#if LOCAL_RESNET_PARAMS_VALID != 1
#error "LocalResNet-Micro parameters are invalid."
#endif
#if LOCAL_RESNET_RESBLOCKS != 2
#error "This HLS graph is locked to LocalResNet-Micro B2."
#endif
    hls::stream<IspPixelPacket<128>> head_out("head_out");
    hls::stream<IspPixelPacket<128>> rb0_out("rb0_out");
    hls::stream<IspPixelPacket<128>> rb1_out("rb1_out");
    hls::stream<IspPixelPacket<40>> global_skip("global_skip");
    hls::stream<IspPixelPacket<40>> tail_packed("tail_packed");
    hls::stream<IspPixelPacket<10>> raw_internal_out("raw_internal_out");
#pragma HLS STREAM variable = head_out depth = 1024
#pragma HLS STREAM variable = rb0_out depth = 64
#pragma HLS STREAM variable = rb1_out depth = 64
#pragma HLS STREAM variable = global_skip depth = 16384
#pragma HLS STREAM variable = tail_packed depth = 2048
#pragma HLS STREAM variable = raw_internal_out depth = 64
#pragma HLS DATAFLOW
    cnn_l0_axis_core(raw_in, head_out, global_skip);
    cnn_resblock0(head_out, rb0_out);
    cnn_resblock1(rb0_out, rb1_out);
    cnn_l5_tail(rb1_out, global_skip, tail_packed);
    packed4_to_raw10(tail_packed, raw_internal_out);
    internal_raw10_to_axis(raw_internal_out, raw_out);
}

void isp_cnn_denoise_top(hls::stream<IspPixelPacket<10>> &raw_in, hls::stream<IspPixelPacket<10>> &raw_out)
{
#pragma HLS INTERFACE axis port = raw_in register = false
#pragma HLS INTERFACE axis port = raw_out
#if LOCAL_RESNET_PARAMS_VALID != 1
#error "LocalResNet-Micro parameters are invalid."
#endif
#if LOCAL_RESNET_RESBLOCKS != 2
#error "This HLS graph is locked to LocalResNet-Micro B2."
#endif
    hls::stream<IspPixelPacket<128>> head_out("head_out");
    hls::stream<IspPixelPacket<128>> rb0_out("rb0_out");
    hls::stream<IspPixelPacket<128>> rb1_out("rb1_out");
    hls::stream<IspPixelPacket<40>> global_skip("global_skip");
    hls::stream<IspPixelPacket<40>> tail_packed("tail_packed");
#pragma HLS STREAM variable = head_out depth = 1024
#pragma HLS STREAM variable = rb0_out depth = 64
#pragma HLS STREAM variable = rb1_out depth = 64
#pragma HLS STREAM variable = global_skip depth = 16384
#pragma HLS STREAM variable = tail_packed depth = 2048
#pragma HLS DATAFLOW
    cnn_l0_core(raw_in, head_out, global_skip);
    cnn_resblock0(head_out, rb0_out);
    cnn_resblock1(rb0_out, rb1_out);
    cnn_l5_tail(rb1_out, global_skip, tail_packed);
    packed4_to_raw10(tail_packed, raw_out);
}
