#include "../ltm_gamma.hpp"
#include "ltm_lut.h"

void read_and_log(hls::stream<ltm_in_t>& s_axis,
                  hls::stream<log_t>& out_log, hls::stream<rgb_pack_t>& out_rgb)
{
    img_size_t total_pixels = (img_size_t)HEIGHT * (img_size_t)WIDTH;
    for (img_size_t i = 0; i < total_pixels; i++)
    {
#pragma HLS PIPELINE II = 1
        ltm_in_t p = s_axis.read();

        ap_uint<12> r_val = p.data.range(11, 0);
        ap_uint<12> g_val = p.data.range(23, 12);
        ap_uint<12> b_val = p.data.range(35, 24);

        ap_ufixed<18, 12> r_f = r_val * (ap_ufixed<16, 0>)0.299f;
        ap_ufixed<18, 12> g_f = g_val * (ap_ufixed<16, 0>)0.587f;
        ap_ufixed<18, 12> b_f = b_val * (ap_ufixed<16, 0>)0.114f;
        ap_ufixed<18, 12> y_sum = r_f + g_f + b_f + (ap_ufixed<18, 12>)0.5f;
        ap_uint<12> y_int = y_sum;
        if (y_int > 4095)
            y_int = 4095;

        log_t log_y = LOG_LUT[y_int];

        out_log.write(log_y);

        rgb_pack_t rgb;
        rgb.r = r_val;
        rgb.g = g_val;
        rgb.b = b_val;
        rgb.log_y = log_y;
        rgb.y_int = y_int;
        out_rgb.write(rgb);
    }
}

void conv1(hls::stream<log_t>& src, hls::stream<weight_a_t>& out_a,
           hls::stream<weight_b_t>& out_b)
{
    hls::LineBuffer<6, WIDTH, log_t> lb;
    hls::LineBuffer<6, WIDTH, var_t> lb_sq;

    ap_fixed<18, 7> col_sum_I_reg[7] = {0, 0, 0, 0, 0, 0, 0};
    ap_fixed<20, 10> col_sum_II_reg[7] = {0, 0, 0, 0, 0, 0, 0};
#pragma HLS ARRAY_PARTITION variable = col_sum_I_reg complete dim = 1
#pragma HLS ARRAY_PARTITION variable = col_sum_II_reg complete dim = 1

    img_size_t total_pixels = (img_size_t)HEIGHT * (img_size_t)WIDTH;
    img_size_t total_cycles = total_pixels + 3 * (img_size_t)WIDTH + 3;

    dim_t r_in = 0, c_in = 0;
    dim_t r_out = 0, c_out = 0;

    for (img_size_t i = 0; i < total_cycles; i++)
    {
#pragma HLS PIPELINE II = 1
        log_t pixel_in = 0;
        var_t pixel_sq_in = 0;

        // Counters handled internally

        if (i < total_pixels)
        {
            pixel_in = src.read();
            pixel_sq_in = (var_t)pixel_in * (var_t)pixel_in;
        }

        log_t col_data[7];
        var_t col_data_sq[7];
        for (int row = 0; row < 6; row++)
        {
            col_data[row] = lb.getval(row, c_in);
            col_data_sq[row] = lb_sq.getval(row, c_in);
        }
        col_data[6] = pixel_in;
        col_data_sq[6] = pixel_sq_in;

        if (i < total_pixels)
        {
            lb.shift_pixels_up(c_in);
            lb.insert_bottom_row(pixel_in, c_in);
            lb_sq.shift_pixels_up(c_in);
            lb_sq.insert_bottom_row(pixel_sq_in, c_in);
        }

        // Compute vertical sum of the new column
        s_dim_t r_center = (s_dim_t)r_in - 3;
        ap_fixed<18, 7> new_col_sum_I = 0;
        ap_fixed<20, 10> new_col_sum_II = 0;

        for (int wi = 0; wi < 7; wi++)
        {
            s_dim_t clamped_r = r_center + (s_dim_t)(wi - 3);
            if (clamped_r < 0)
                clamped_r = 0;
            else if (clamped_r >= (s_dim_t)HEIGHT)
                clamped_r = (s_dim_t)HEIGHT - 1;
            
            int base_row = (r_in < HEIGHT) ? ((int)r_in - 6) : (HEIGHT - 6);
            int wr = clamped_r - base_row;

            new_col_sum_I += col_data[wr];
            new_col_sum_II += col_data_sq[wr];
        }

        // Shift the column sums
        for (int wj = 0; wj < 6; wj++)
        {
            col_sum_I_reg[wj] = col_sum_I_reg[wj + 1];
            col_sum_II_reg[wj] = col_sum_II_reg[wj + 1];
        }
        col_sum_I_reg[6] = new_col_sum_I;
        col_sum_II_reg[6] = new_col_sum_II;

        ap_int<24> i_out = (ap_int<24>)i - (3 * (ap_int<24>)WIDTH + 3);
        if (i_out >= 0)
        {
            ap_fixed<21, 10> sum_I = 0;
            ap_fixed<23, 13> sum_II = 0;

            for (int wj = 0; wj < 7; wj++)
            {
                s_dim_t clamped_c = (s_dim_t)c_out + (s_dim_t)(wj - 3);
                if (clamped_c < 0)
                    clamped_c = 0;
                else if (clamped_c >= (s_dim_t)WIDTH)
                    clamped_c = (s_dim_t)WIDTH - 1;
                int wc = clamped_c - (s_dim_t)c_out + 3;

                sum_I += col_sum_I_reg[wc];
                sum_II += col_sum_II_reg[wc];
            }

            log_t mean_I = sum_I * (ap_ufixed<16, 0>)0.020408f;
            var_t mean_II = sum_II * (ap_ufixed<16, 0>)0.020408f;

            var_t var_I = mean_II - (var_t)(mean_I * mean_I);
            if (var_I < 0)
                var_I = 0;

            ap_fixed<20, 10> var_ext = var_I;
            ap_fixed<20, 10> eps1 = 0.001;
            ap_fixed<20, 10> eps_base = 0.5;

            ap_fixed<20, 10> eps_map = eps_base / (var_ext + eps1);
            ap_fixed<20, 10> a_ext = var_ext / (var_ext + eps_map);
            if (a_ext > (ap_fixed<20, 10>)0.6)
                a_ext = 0.6;

            weight_a_t out_a_val = (weight_a_t)a_ext;
            weight_b_t out_b_val = mean_I - (weight_b_t)(out_a_val * mean_I);

            out_a.write(out_a_val);
            out_b.write(out_b_val);

            c_out++;
            if (c_out == WIDTH)
            {
                c_out = 0;
                r_out++;
            }
        }

        c_in++;
        if (c_in == WIDTH)
        {
            c_in = 0;
            r_in++;
        }
    }
}

void conv2(hls::stream<weight_a_t>& src_a, hls::stream<weight_b_t>& src_b,
           hls::stream<weight_a_t>& out_mean_a,
           hls::stream<weight_b_t>& out_mean_b)
{
    hls::LineBuffer<6, WIDTH, weight_a_t> lb_a;
    hls::LineBuffer<6, WIDTH, weight_b_t> lb_b;

    ap_ufixed<14, 3> col_sum_a_reg[7] = {0, 0, 0, 0, 0, 0, 0};
    ap_fixed<17, 7> col_sum_b_reg[7] = {0, 0, 0, 0, 0, 0, 0};
#pragma HLS ARRAY_PARTITION variable = col_sum_a_reg complete dim = 1
#pragma HLS ARRAY_PARTITION variable = col_sum_b_reg complete dim = 1

    img_size_t total_pixels = (img_size_t)HEIGHT * (img_size_t)WIDTH;
    img_size_t total_cycles = total_pixels + 3 * (img_size_t)WIDTH + 3;

    dim_t r_in = 0, c_in = 0;
    dim_t r_out = 0, c_out = 0;

    for (img_size_t i = 0; i < total_cycles; i++)
    {
#pragma HLS PIPELINE II = 1
        weight_a_t pixel_a = 0;
        weight_b_t pixel_b = 0;

        // Counters handled internally

        if (i < total_pixels)
        {
            pixel_a = src_a.read();
            pixel_b = src_b.read();
        }

        weight_a_t col_data_a[7];
        weight_b_t col_data_b[7];
        for (int row = 0; row < 6; row++)
        {
            col_data_a[row] = lb_a.getval(row, c_in);
            col_data_b[row] = lb_b.getval(row, c_in);
        }
        col_data_a[6] = pixel_a;
        col_data_b[6] = pixel_b;

        if (i < total_pixels)
        {
            lb_a.shift_pixels_up(c_in);
            lb_a.insert_bottom_row(pixel_a, c_in);
            lb_b.shift_pixels_up(c_in);
            lb_b.insert_bottom_row(pixel_b, c_in);
        }

        s_dim_t r_center = (s_dim_t)r_in - 3;
        ap_ufixed<14, 3> new_col_sum_a = 0;
        ap_fixed<17, 7> new_col_sum_b = 0;

        for (int wi = 0; wi < 7; wi++)
        {
            s_dim_t clamped_r = r_center + (s_dim_t)(wi - 3);
            if (clamped_r < 0)
                clamped_r = 0;
            else if (clamped_r >= (s_dim_t)HEIGHT)
                clamped_r = (s_dim_t)HEIGHT - 1;
            
            int base_row = (r_in < HEIGHT) ? ((int)r_in - 6) : (HEIGHT - 6);
            int wr = clamped_r - base_row;

            new_col_sum_a += col_data_a[wr];
            new_col_sum_b += col_data_b[wr];
        }

        for (int wj = 0; wj < 6; wj++)
        {
            col_sum_a_reg[wj] = col_sum_a_reg[wj + 1];
            col_sum_b_reg[wj] = col_sum_b_reg[wj + 1];
        }
        col_sum_a_reg[6] = new_col_sum_a;
        col_sum_b_reg[6] = new_col_sum_b;

        ap_int<24> i_out = (ap_int<24>)i - (3 * (ap_int<24>)WIDTH + 3);
        if (i_out >= 0)
        {
            ap_ufixed<17, 6> sum_a = 0;
            ap_fixed<20, 10> sum_b = 0;

            for (int wj = 0; wj < 7; wj++)
            {
                s_dim_t clamped_c = (s_dim_t)c_out + (s_dim_t)(wj - 3);
                if (clamped_c < 0)
                    clamped_c = 0;
                else if (clamped_c >= (s_dim_t)WIDTH)
                    clamped_c = (s_dim_t)WIDTH - 1;
                int wc = clamped_c - (s_dim_t)c_out + 3;

                sum_a += col_sum_a_reg[wc];
                sum_b += col_sum_b_reg[wc];
            }

            weight_a_t mean_a = sum_a * (ap_ufixed<16, 0>)0.020408f;
            weight_b_t mean_b = sum_b * (ap_ufixed<16, 0>)0.020408f;

            out_mean_a.write(mean_a);
            out_mean_b.write(mean_b);

            c_out++;
            if (c_out == WIDTH)
            {
                c_out = 0;
                r_out++;
            }
        }

        c_in++;
        if (c_in == WIDTH)
        {
            c_in = 0;
            r_in++;
        }
    }
}

void recombine_and_pack(
    hls::stream<weight_a_t>& src_mean_a, hls::stream<weight_b_t>& src_mean_b,
    hls::stream<rgb_pack_t>& delay_rgb, hls::stream<ltm_out_t>& m_axis)
{

    bool is_first = true;
    dim_t r = 0, c = 0;
    // H * W iterations
    img_size_t total_pixels = (img_size_t)HEIGHT * (img_size_t)WIDTH;
    for (img_size_t i = 0; i < total_pixels; i++)
    {
#pragma HLS PIPELINE II = 1
        weight_a_t mean_a = src_mean_a.read();
        weight_b_t mean_b = src_mean_b.read();
        rgb_pack_t rgb = delay_rgb.read();

        // Counters handled internally

        ap_uint<12> r_val = rgb.r;
        ap_uint<12> g_val = rgb.g;
        ap_uint<12> b_val = rgb.b;

        ap_uint<12> y_int = rgb.y_int;
        log_t log_I = rgb.log_y;

        ap_fixed<16, 6> log_I_base =
            (ap_fixed<16, 6>)mean_a * (ap_fixed<16, 6>)log_I +
            (ap_fixed<16, 6>)mean_b;

        log_t log_I_detail = log_I - log_I_base;

        ap_fixed<20, 12> idx_float =
            (ap_fixed<20, 12>)log_I_base * (ap_fixed<20, 12>)123.0f;
        ap_int<16> lut_idx = idx_float + (ap_fixed<20, 12>)0.5f;
        if (lut_idx < 0)
            lut_idx = 0;
        if (lut_idx > 1023)
            lut_idx = 1023;

        log_t log_I_mapped_base = REINHARD_LUT[lut_idx];

        ap_fixed<14, 3> w =
            (ap_fixed<14, 3>)1.56 -
            (log_I_mapped_base * (ap_fixed<14, 3>)(0.26 / 8.317766));
        log_t log_I_final = log_I_mapped_base + w * log_I_detail;

        ap_fixed<24, 12> exp_idx_float =
            (ap_fixed<24, 12>)log_I_final * (ap_fixed<24, 12>)123.0f;
        ap_int<16> exp_lut_idx = exp_idx_float + (ap_fixed<24, 12>)0.5f;
        if (exp_lut_idx < 0)
            exp_lut_idx = 0;
        if (exp_lut_idx > 1023)
            exp_lut_idx = 1023;

        exp_out_t I_final = EXP_LUT[exp_lut_idx];

        
        ap_ufixed<24, 12> y_divisor = (y_int == 0) ? (ap_ufixed<24, 12>)1.0f : (ap_ufixed<24, 12>)y_int;
        ap_ufixed<24, 12> gain = I_final / y_divisor;

        ap_ufixed<28, 16> r_out_raw = rgb.r * gain;
        ap_ufixed<28, 16> g_out_raw = rgb.g * gain;
        ap_ufixed<28, 16> b_out_raw = rgb.b * gain;

        rgb_out_t r_out = r_out_raw;
        rgb_out_t g_out = g_out_raw;
        rgb_out_t b_out = b_out_raw;

        ltm_out_t p_out;
        p_out.data.range(11, 0) = r_out;
        p_out.data.range(23, 12) = g_out;
        p_out.data.range(35, 24) = b_out;



        p_out.user = is_first ? 1 : 0;
        p_out.last = (c == (dim_t)(WIDTH - 1)) ? 1 : 0;

        is_first = false;
        m_axis.write(p_out);

        c++;
        if (c == WIDTH)
        {
            c = 0;
            r++;
        }
    }
}


void isp_ltm_top(hls::stream<ltm_in_t>& s_axis, hls::stream<ltm_out_t>& m_axis)
{

#pragma HLS INTERFACE axis port = s_axis
#pragma HLS INTERFACE axis port = m_axis
#pragma HLS INTERFACE s_axilite port = return
#pragma HLS BIND_STORAGE variable=LOG_LUT impl=bram
#pragma HLS BIND_STORAGE variable=REINHARD_LUT impl=bram
#pragma HLS BIND_STORAGE variable=EXP_LUT impl=bram

#pragma HLS DATAFLOW

    hls::stream<log_t> stream_log("stream_log");

    hls::stream<rgb_pack_t> stream_rgb("stream_rgb");
#pragma HLS STREAM variable = stream_rgb depth = 11700

    hls::stream<weight_a_t> stream_a("stream_a");

    hls::stream<weight_b_t> stream_b("stream_b");

    hls::stream<weight_a_t> stream_mean_a("stream_mean_a");
    hls::stream<weight_b_t> stream_mean_b("stream_mean_b");


    read_and_log(s_axis, stream_log, stream_rgb);
    conv1(stream_log, stream_a, stream_b);
    conv2(stream_a, stream_b, stream_mean_a, stream_mean_b);
    recombine_and_pack(stream_mean_a, stream_mean_b, stream_rgb, m_axis);
}