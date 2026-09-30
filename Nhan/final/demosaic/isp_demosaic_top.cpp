#include "isp_demosaic_top.hpp"
using namespace hls;

// Luong xu ly cua core:
//   1) line buffer RAW tao cua so 3x3;
//   2) noi suy G va giu lai RAW tai tam cua so;
//   3) line buffer {RAW,G} tao cua so 3x3 thu hai;
//   4) noi suy R/B, dong goi RGB va chi write khi pixel da hop le.

static void advance_position(
    int &row,
    int &col
)
{
    #pragma HLS INLINE

    if (col == FRAME_WIDTH - 1) {
        col = 0;
        row = (row == FRAME_HEIGHT - 1) ? 0 : row + 1;
    } else {
        col = col + 1;
    }
}

// Tang 1:
//   RAW Bayer -> {RAW trung tam, G da noi suy}
//
// Hai line buffer chi co mot lan doc va mot lan ghi moi chu ky.
// Cua so 3x3 nam hoan toan trong register.
static bool interpolate_green_step(
    const IspPixelPacket<12> &p_in,
    int edge_threshold,
    int edge_mag,
    bool frame_start,
    green_pixel_t &p_green
)
{
    #pragma HLS INLINE

    static ap_uint<12> raw_line_1[MAX_WIDTH];
    static ap_uint<12> raw_line_2[MAX_WIDTH];
    #pragma HLS BIND_STORAGE variable=raw_line_1 type=RAM_S2P impl=BRAM latency=2
    #pragma HLS BIND_STORAGE variable=raw_line_2 type=RAM_S2P impl=BRAM latency=2

    static ap_uint<12> raw_window[3][3];
    #pragma HLS ARRAY_PARTITION variable=raw_window complete dim=0

    // Hai direction line buffer ping-pong, moi phan tu chi 1 bit:
    //   - mot bank chi doc direction cua hang truoc;
    //   - bank con lai chi ghi direction cua hang hien tai.
    // Cuoi moi hang, hai bank doi vai tro.
    static ap_uint<1> dir_bank0[MAX_WIDTH];
    static ap_uint<1> dir_bank1[MAX_WIDTH];
    #pragma HLS BIND_STORAGE variable=dir_bank0 type=RAM_S2P impl=LUTRAM
    #pragma HLS BIND_STORAGE variable=dir_bank1 type=RAM_S2P impl=LUTRAM
    #pragma HLS DEPENDENCE variable=dir_bank0 inter false
    #pragma HLS DEPENDENCE variable=dir_bank1 inter false

    static int input_col = 0;
    static int warmup_count = 0;
    static bool green_started = false;

    // Toa do cua pixel ma tang G sap xuat ra.
    static int green_row = 0;
    static int green_col = 0;

    static ap_uint<1> dir_left = 0;
    static ap_uint<1> prev_dir_left_cache = 0;
    static ap_uint<1> prev_dir_center_cache = 0;
    static ap_uint<1> next_row_center_seed = 0;
    // 0: ghi bank0, doc bank1. 1: ghi bank1, doc bank0.
    static ap_uint<1> dir_write_bank = 0;

    #pragma HLS RESET variable=input_col
    #pragma HLS RESET variable=warmup_count
    #pragma HLS RESET variable=green_started
    #pragma HLS RESET variable=green_row
    #pragma HLS RESET variable=green_col
    #pragma HLS RESET variable=dir_left
    #pragma HLS RESET variable=prev_dir_left_cache
    #pragma HLS RESET variable=prev_dir_center_cache
    #pragma HLS RESET variable=next_row_center_seed
    #pragma HLS RESET variable=dir_write_bank
    
    //reset dau moi frame
    if (frame_start) {
        input_col = 0;
        warmup_count = 0;
        green_started = false;
        green_row = 0;
        green_col = 0;
        dir_left = 0;
        prev_dir_left_cache = 0;
        prev_dir_center_cache = 0;
        next_row_center_seed = 0;
        dir_write_bank = 0;
    }
    
    ap_uint<12> raw_in = p_in.data;

    // Doc gia tri cu truoc khi dich cot doc xuong hai line buffer.
    ap_uint<12> raw_m1 = raw_line_1[input_col];
    ap_uint<12> raw_m2 = raw_line_2[input_col];

    raw_line_2[input_col] = raw_m1;
    raw_line_1[input_col] = raw_in;

    // Dịch trực tiếp cửa sổ RAW sang trái.
    raw_window[0][0] = raw_window[0][1];
    raw_window[0][1] = raw_window[0][2];
    raw_window[0][2] = raw_m2;

    raw_window[1][0] = raw_window[1][1];
    raw_window[1][1] = raw_window[1][2];
    raw_window[1][2] = raw_m1;

    raw_window[2][0] = raw_window[2][1];
    raw_window[2][1] = raw_window[2][2];
    raw_window[2][2] = raw_in;

    input_col = (input_col == (FRAME_WIDTH - 1))? 0 : input_col + 1;

    // Sau width + 1 input, cua so dau tien co du du lieu cho G(0,0).
    bool emit_green = green_started;
    if (!green_started) {
        if (warmup_count == FRAME_WIDTH + 1) {
            green_started = true;
            emit_green = true;
        } else {
            warmup_count = warmup_count + 1;
        }
    }

    if (!emit_green) {
        return false;
    }

    bool row_even = ((green_row & 1) == 0);
    bool col_even = ((green_col & 1) == 0);
    bool is_rb_site = (row_even == col_even);

    bool top_boundary = (green_row == 0);
    bool bottom_boundary = (green_row == FRAME_HEIGHT - 1);
    bool left_boundary = (green_col == 0);
    bool right_boundary = (green_col == FRAME_WIDTH - 1);

    // Mirror-101 giong code cu:
    // x=-1 -> x=1, x=width -> x=width-2,
    // y=-1 -> y=1, y=height -> y=height-2.
    int left_slot = left_boundary ? 2 : 0;
    int right_slot = right_boundary ? 0 : 2;
    int top_slot = top_boundary ? 2 : 0;
    int bottom_slot = bottom_boundary ? 0 : 2;

    ap_uint<12> raw_center = raw_window[1][1];
    ap_uint<12> G_left     = raw_window[1][left_slot];
    ap_uint<12> G_right    = raw_window[1][right_slot];
    ap_uint<12> G_top      = raw_window[top_slot][1];
    ap_uint<12> G_bot      = raw_window[bottom_slot][1];

    // Mot read-ahead duy nhat tu bank cua hang truoc moi chu ky.
    // Tai cot cuoi, prev_dir_right se mirror tu prev_dir_center nen gia tri
    // doc tai cot cuoi khong duoc dung.
    int dir_read_addr = right_boundary ? green_col : green_col + 1;
    ap_uint<1> dir_read_value;
    if (dir_write_bank == 0) {
        dir_read_value = dir_bank1[dir_read_addr];
    } else {
        dir_read_value = dir_bank0[dir_read_addr];
    }

    // Lay ba direction cua hang truoc tu hai cache va read-ahead.
    ap_uint<1> prev_dir_left = 0;
    ap_uint<1> prev_dir_center = 0;
    ap_uint<1> prev_dir_right = 0;

    if (green_col == 0) {
        dir_left = 0;
    }

    if (!top_boundary) {
        if (green_col == 0) {
            prev_dir_center = next_row_center_seed;
            prev_dir_right = dir_read_value;
            prev_dir_left = prev_dir_center;
        } else {
            prev_dir_left = prev_dir_left_cache;
            prev_dir_center = prev_dir_center_cache;
            if (green_col == FRAME_WIDTH - 1) {
                prev_dir_right = prev_dir_center;
            } else {
                prev_dir_right = dir_read_value;
            }
        }
    }

    prev_dir_left_cache = prev_dir_center;
    prev_dir_center_cache = prev_dir_right;

    g_pixel G_result;
    ap_uint<1> dir_chosen;

    if (!is_rb_site) {
        // Pixel G co san trong anh Bayer.
        G_result = raw_center;
        dir_chosen = dir_left;
    } else {
        g_pixel G_H = (g_pixel)((G_left + G_right) >> 1);
        g_pixel G_V = (g_pixel)((G_top + G_bot) >> 1);

        ap_uint<12> grad_h = (G_left > G_right)
                       ? (ap_uint<12>)(G_left - G_right)
                       : (ap_uint<12>)(G_right - G_left);
        ap_uint<12> grad_v = (G_top > G_bot)
                       ? (ap_uint<12>)(G_top - G_bot)
                       : (ap_uint<12>)(G_bot - G_top);
        ap_uint<12> diff = (grad_h > grad_v)
                     ? (ap_uint<12>)(grad_h - grad_v)
                     : (ap_uint<12>)(grad_v - grad_h);
        ap_uint<12> grad_max = (grad_h > grad_v) ? grad_h : grad_v;

        if (grad_max < (ap_uint<12>)edge_mag) {
            G_result = (G_H + G_V) >> 1;
            dir_chosen = dir_left;
        } else if (diff > (ap_uint<12>)edge_threshold) {
            dir_chosen = (grad_h > grad_v) ? 1 : 0;
            G_result = dir_chosen ? G_V : G_H;
        } else {
            ap_uint<3> vote;
            if (top_boundary) {
                // Hang 0 khong co lich su direction cua hang truoc.
                vote = dir_left;
            } else {
                vote = prev_dir_left + prev_dir_center
                     + prev_dir_right + dir_left;
            }
            dir_chosen = (vote > 2) ? 1 : 0;
            G_result = dir_chosen ? G_V : G_H;
        }
    }

    // Ghi direction hien tai vao bank con lai, tach biet voi bank dang doc.
    if (dir_write_bank == 0) {
        dir_bank0[green_col] = dir_chosen;
    } else {
        dir_bank1[green_col] = dir_chosen;
    }

    // Giu direction cot 0 trong register de dau hang sau chi can mot lan doc
    // (cot 1) ma van co du prev_center va prev_right.
    if (green_col == 0) {
        next_row_center_seed = dir_chosen;
    }

    if (right_boundary) {
        dir_write_bank = dir_write_bank ^ 1;
    }
    dir_left = dir_chosen;

    p_green.raw = raw_center;
    p_green.g = G_result;

    advance_position(green_row, green_col);
    return true;
}

// Tang 2:
//   {RAW, G} -> RGB
static bool interpolate_red_blue_step(
    const green_pixel_t &p_green,
    bool frame_start,   
    IspPixelPacket<36> &p_out
)
{
    #pragma HLS INLINE

    static green_pixel_t green_line_1[MAX_WIDTH];
    static green_pixel_t green_line_2[MAX_WIDTH];
    #pragma HLS BIND_STORAGE variable=green_line_1 type=RAM_S2P impl=BRAM
    #pragma HLS BIND_STORAGE variable=green_line_2 type=RAM_S2P impl=BRAM

    static green_pixel_t green_window[3][3];
    #pragma HLS ARRAY_PARTITION variable=green_window complete dim=0

    static int input_col = 0;
    static int warmup_count = 0;
    static bool rgb_started = false;

    // Toa do cua pixel RGB sap xuat ra.
    static int rgb_row = 0;
    static int rgb_col = 0;

    #pragma HLS RESET variable=input_col
    #pragma HLS RESET variable=warmup_count
    #pragma HLS RESET variable=rgb_started
    #pragma HLS RESET variable=rgb_row
    #pragma HLS RESET variable=rgb_col

    if (frame_start) {
        input_col = 0;
        warmup_count = 0;
        rgb_started = false;
        rgb_row = 0;
        rgb_col = 0;
    }

    green_pixel_t green_m1 = green_line_1[input_col];
    green_pixel_t green_m2 = green_line_2[input_col];

    green_line_2[input_col] = green_m1;
    green_line_1[input_col] = p_green;

    // Dịch trực tiếp cửa sổ {RAW,G} sang trái.
    green_window[0][0] = green_window[0][1];
    green_window[0][1] = green_window[0][2];
    green_window[0][2] = green_m2;

    green_window[1][0] = green_window[1][1];
    green_window[1][1] = green_window[1][2];
    green_window[1][2] = green_m1;

    green_window[2][0] = green_window[2][1];
    green_window[2][1] = green_window[2][2];
    green_window[2][2] = p_green;

    input_col = (input_col == FRAME_WIDTH - 1) ? 0 : input_col + 1;

    // Them width + 1 pixel {RAW,G} de co cua so RGB dau tien.
    bool emit_rgb = rgb_started;
    if (!rgb_started) {
        if (warmup_count == FRAME_WIDTH + 1) {
            rgb_started = true;
            emit_rgb = true;
        } else {
            warmup_count = warmup_count + 1;
        }
    }

    if (!emit_rgb) {
        return false;
    }

    bool row_even = ((rgb_row & 1) == 0);
    bool col_even = ((rgb_col & 1) == 0);

    bool top_boundary = (rgb_row == 0);
    bool bottom_boundary = (rgb_row == FRAME_HEIGHT - 1);
    bool left_boundary = (rgb_col == 0);
    bool right_boundary = (rgb_col == FRAME_WIDTH - 1);

    int left_slot = left_boundary ? 2 : 0;
    int right_slot = right_boundary ? 0 : 2;
    int top_slot = top_boundary ? 2 : 0;
    int bottom_slot = bottom_boundary ? 0 : 2;

    ap_uint<12> raw_center = green_window[1][1].raw;
    g_pixel G_center_u = green_window[1][1].g;
    ap_fixed<17,13> G_center = G_center_u;

    ap_fixed<17,13> avg_B;
    ap_fixed<17,13> avg_R;
    ap_fixed<17,13> avg_G;
    ap_fixed<17,13> avg_G_for_B;
    ap_fixed<17,13> avg_G_for_R;
    ap_fixed<19,15> calc_b;
    ap_fixed<19,15> calc_r;
    o_pixel r_out;
    o_pixel g_out;
    o_pixel b_out;

    if (row_even && col_even) {
        // R site: noi suy B theo bon diem cheo.
        ap_uint<13> raw_top_pair =
            green_window[top_slot][left_slot].raw
          + green_window[top_slot][right_slot].raw;
        ap_uint<13> raw_bottom_pair =
            green_window[bottom_slot][left_slot].raw
          + green_window[bottom_slot][right_slot].raw;
        ap_uint<14> raw_diagonal_sum;
        #pragma HLS BIND_OP variable=raw_diagonal_sum op=add impl=fabric latency=1
        raw_diagonal_sum = raw_top_pair + raw_bottom_pair;
        avg_B = (ap_fixed<17,13>)(raw_diagonal_sum >> 2);

        ap_ufixed<17,13> g_top_pair =
            green_window[top_slot][left_slot].g
          + green_window[top_slot][right_slot].g;
        ap_ufixed<17,13> g_bottom_pair =
            green_window[bottom_slot][left_slot].g
          + green_window[bottom_slot][right_slot].g;
        ap_ufixed<18,14> g_diagonal_sum;
        #pragma HLS BIND_OP variable=g_diagonal_sum op=add impl=fabric latency=1
        g_diagonal_sum = g_top_pair + g_bottom_pair;
        avg_G = (ap_fixed<17,13>)(g_diagonal_sum >> 2);

        calc_b = G_center + avg_B - avg_G;

        r_out = raw_center;
        g_out = (o_pixel)G_center;
        b_out = (calc_b < 0) ? (o_pixel)0 : (o_pixel)calc_b;
    }
    else if (row_even && !col_even) {
        // G tren hang R: R ngang, B doc.
        avg_R = (ap_fixed<17,13>)(
            (green_window[1][left_slot].raw
           + green_window[1][right_slot].raw) >> 1);
        avg_G_for_R = (ap_fixed<17,13>)(
            (green_window[1][left_slot].g
           + green_window[1][right_slot].g) >> 1);
        calc_r = G_center + avg_R - avg_G_for_R;

        avg_B = (ap_fixed<17,13>)(
            (green_window[top_slot][1].raw
           + green_window[bottom_slot][1].raw) >> 1);
        avg_G_for_B = (ap_fixed<17,13>)(
            (green_window[top_slot][1].g
           + green_window[bottom_slot][1].g) >> 1);
        calc_b = G_center + avg_B - avg_G_for_B;

        r_out = (calc_r < 0) ? (o_pixel)0 : (o_pixel)calc_r;
        g_out = (o_pixel)G_center;
        b_out = (calc_b < 0) ? (o_pixel)0 : (o_pixel)calc_b;
    }
    else if (!row_even && col_even) {
        // G tren hang B: R doc, B ngang.
        avg_R = (ap_fixed<17,13>)(
            (green_window[top_slot][1].raw
           + green_window[bottom_slot][1].raw) >> 1);
        avg_G_for_R = (ap_fixed<17,13>)(
            (green_window[top_slot][1].g
           + green_window[bottom_slot][1].g) >> 1);
        calc_r = G_center + avg_R - avg_G_for_R;

        avg_B = (ap_fixed<17,13>)(
            (green_window[1][left_slot].raw
           + green_window[1][right_slot].raw) >> 1);
        avg_G_for_B = (ap_fixed<17,13>)(
            (green_window[1][left_slot].g
           + green_window[1][right_slot].g) >> 1);
        calc_b = G_center + avg_B - avg_G_for_B;

        r_out = (calc_r < 0) ? (o_pixel)0 : (o_pixel)calc_r;
        g_out = (o_pixel)G_center;
        b_out = (calc_b < 0) ? (o_pixel)0 : (o_pixel)calc_b;
    }
    else {
        // B site: noi suy R theo bon diem cheo.
        ap_uint<13> raw_top_pair =
            green_window[top_slot][left_slot].raw
          + green_window[top_slot][right_slot].raw;
        ap_uint<13> raw_bottom_pair =
            green_window[bottom_slot][left_slot].raw
          + green_window[bottom_slot][right_slot].raw;
        ap_uint<14> raw_diagonal_sum;
        #pragma HLS BIND_OP variable=raw_diagonal_sum op=add impl=fabric latency=1
        raw_diagonal_sum = raw_top_pair + raw_bottom_pair;
        avg_R = (ap_fixed<17,13>)(raw_diagonal_sum >> 2);

        ap_ufixed<17,13> g_top_pair =
            green_window[top_slot][left_slot].g
          + green_window[top_slot][right_slot].g;
        ap_ufixed<17,13> g_bottom_pair =
            green_window[bottom_slot][left_slot].g
          + green_window[bottom_slot][right_slot].g;
        ap_ufixed<18,14> g_diagonal_sum;
        #pragma HLS BIND_OP variable=g_diagonal_sum op=add impl=fabric latency=1
        g_diagonal_sum = g_top_pair + g_bottom_pair;
        avg_G = (ap_fixed<17,13>)(g_diagonal_sum >> 2);

        calc_r = G_center + avg_R - avg_G;

        r_out = (calc_r < 0) ? (o_pixel)0 : (o_pixel)calc_r;
        g_out = (o_pixel)G_center;
        b_out = raw_center;
    }

    // Dong goi AXIS. TKEEP/TSTRB bao toan bo 36 bit RGB la hop le.
    p_out.data.range(11, 0) = r_out;
    p_out.data.range(23, 12) = g_out;
    p_out.data.range(35, 24) = b_out;
    p_out.user = (rgb_row == 0 && rgb_col == 0) ? 1 : 0;
    p_out.last = (rgb_col == FRAME_WIDTH - 1) ? 1 : 0;

    advance_position(rgb_row, rgb_col);
    return true;
}

void isp_demosaic(
    hls::stream<IspPixelPacket<12>>& stream_in,
    hls::stream<IspPixelPacket<36>>& stream_out
) {
    #pragma HLS INLINE off

    const int frame_pixels = FRAME_WIDTH * FRAME_HEIGHT;
    const int window_delay = FRAME_WIDTH + 1;
    const int total_steps = frame_pixels + 2 * window_delay;
    int green_count = 0;
    int output_count = 0;
    
    //flush noi bo
    for(int step = 0; step < total_steps; ++step) {
        #pragma HLS PIPELINE II=1 style=flp

        IspPixelPacket<12> input_pixel;
        
        if(step < frame_pixels) {
            input_pixel = stream_in.read();
        }
        else {
            input_pixel.data = 0;
            input_pixel.user = 0;
            input_pixel.last = 0;
        }

        green_pixel_t p_green;
        bool green_ready = interpolate_green_step(
            input_pixel,
            THRESHOLD_T,
            EDGE_MAG_T,
            step == 0,
            p_green
        );
        

        if (green_ready) {
            IspPixelPacket<36> p_out;
            bool rgb_ready = interpolate_red_blue_step(
                p_green,
                green_count == 0,
                p_out
            );
            
            ++green_count;
            
            if (rgb_ready && (output_count < frame_pixels)) {
                stream_out.write(p_out);
                ++output_count;
            }
        }
    }
}

static void input_adapter(
    hls::stream<ap_axiu<16, 1, 0, 0>>& input,
    hls::stream<IspPixelPacket<12>>& output
) {
    #pragma HLS INLINE off
    
    for(int y = 0; y < FRAME_HEIGHT; ++y) {
        for(int x = 0; x < FRAME_WIDTH; ++x) {
            #pragma HLS LOOP_FLATTEN
            #pragma HLS PIPELINE II=1
            
            ap_axiu<16, 1, 0, 0> axi_pixel = input.read();
            IspPixelPacket<12> pixel;
            pixel.data = axi_pixel.data.range(11, 0);
            pixel.user = axi_pixel.user;
            pixel.last = axi_pixel.last;
            output.write(pixel);
        }
    }
}

static void output_adapter(
    hls::stream<IspPixelPacket<36>>& input,
    hls::stream<ap_axiu<40, 1, 0, 0>>& output
) {
    #pragma HLS INLINE off
    
    for(int y = 0; y < FRAME_HEIGHT; ++y) {
        for(int x = 0; x < FRAME_WIDTH; ++x) {
            #pragma HLS LOOP_FLATTEN
            #pragma HLS PIPELINE II=1
            IspPixelPacket<36> pixel = input.read();
            ap_axiu<40, 1, 0, 0> axi_pixel;
            axi_pixel.data = 0;
            axi_pixel.data.range(35, 0) = pixel.data;
            axi_pixel.keep = -1;
            axi_pixel.strb = -1;
            axi_pixel.user = pixel.user;
            axi_pixel.last = pixel.last;
            output.write(axi_pixel);
        }
    }
}

void isp_demosaic_top(
    hls::stream<ap_axiu<16, 1, 0, 0>>& input,
    hls::stream<ap_axiu<40, 1, 0, 0>>& output
) {

    #pragma HLS INTERFACE mode=axis port=input
    #pragma HLS INTERFACE mode=axis port=output
    #pragma HLS INTERFACE mode=s_axilite port=return bundle=CTRL

    #pragma HLS DATAFLOW

    hls::stream<IspPixelPacket<12>> input_pixels;
    hls::stream<IspPixelPacket<36>> output_pixels;

    #pragma HLS STREAM variable=input_pixels depth=2
    #pragma HLS STREAM variable=output_pixels depth=2
    
    input_adapter(input, input_pixels);
    isp_demosaic(input_pixels, output_pixels);
    output_adapter(output_pixels, output);
}
