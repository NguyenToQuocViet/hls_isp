#include "isp_demosaic.h"

using namespace hls;

// Luong xu ly cua core:
//   1) line buffer RAW tao cua so 3x3;
//   2) noi suy G va giu lai RAW tai tam cua so;
//   3) line buffer {RAW,G} tao cua so 3x3 thu hai;
//   4) noi suy R/B, dong goi RGB va chi write khi pixel da hop le.

// Pixel trung gian giua tang noi suy G va tang noi suy R/B.
struct green_pixel_t {
    i_pixel raw;
    g_pixel g;
};

static void advance_position(
    int &row,
    int &col,
    int width,
    int height
)
{
    #pragma HLS INLINE

    if (col == width - 1) {
        col = 0;
        row = (row == height - 1) ? 0 : row + 1;
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
    const video_in_t &p_in,
    int width,
    int height,
    int edge_threshold,
    int edge_mag,
    green_pixel_t &p_green
)
{
    #pragma HLS INLINE

    static i_pixel raw_line_1[MAX_WIDTH];
    static i_pixel raw_line_2[MAX_WIDTH];
    #pragma HLS BIND_STORAGE variable=raw_line_1 type=RAM_S2P impl=BRAM
    #pragma HLS BIND_STORAGE variable=raw_line_2 type=RAM_S2P impl=BRAM

    static i_pixel raw_window[3][3];
    #pragma HLS ARRAY_PARTITION variable=raw_window complete dim=0

    // Hai direction line buffer ping-pong, moi phan tu chi 1 bit:
    //   - mot bank chi doc direction cua hang truoc;
    //   - bank con lai chi ghi direction cua hang hien tai.
    // Cuoi moi hang, hai bank doi vai tro.
    static ap_uint<1> dir_bank0[MAX_WIDTH];
    static ap_uint<1> dir_bank1[MAX_WIDTH];
    #pragma HLS BIND_STORAGE variable=dir_bank0 type=RAM_S2P impl=LUTRAM
    #pragma HLS BIND_STORAGE variable=dir_bank1 type=RAM_S2P impl=LUTRAM
    // dir_write_bank chi doi tai EOL. Trong mot hang, moi bank hoac chi doc
    // hoac chi ghi; HLS khong tu suy ra duoc tinh loai tru cua hai nhanh.
    // Voi width >= 8, du lieu dau hang da duoc ghi xong truoc khi bank doi
    // vai tro, nen dependency khoang cach 1 ma scheduler bao la dependency gia.
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

    int current_input_col = p_in.user ? 0 : input_col;
    i_pixel raw_in = (i_pixel)axis_get_raw(p_in);

    // Doc gia tri cu truoc khi dich cot doc xuong hai line buffer.
    i_pixel raw_m1 = raw_line_1[current_input_col];
    i_pixel raw_m2 = raw_line_2[current_input_col];

    raw_line_2[current_input_col] = raw_m1;
    raw_line_1[current_input_col] = raw_in;

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

    if (p_in.last) {
        input_col = 0;
    } else {
        input_col = current_input_col + 1;
    }

    // Sau width + 1 input, cua so dau tien co du du lieu cho G(0,0).
    bool emit_green = green_started;
    if (!green_started) {
        if (warmup_count == width + 1) {
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
    bool bottom_boundary = (green_row == height - 1);
    bool left_boundary = (green_col == 0);
    bool right_boundary = (green_col == width - 1);

    // Mirror-101
    // x=-1 -> x=1, x=width -> x=width-2,
    // y=-1 -> y=1, y=height -> y=height-2.
    int left_slot = left_boundary ? 2 : 0;
    int right_slot = right_boundary ? 0 : 2;
    int top_slot = top_boundary ? 2 : 0;
    int bottom_slot = bottom_boundary ? 0 : 2;

    i_pixel raw_center = raw_window[1][1];
    i_pixel G_left     = raw_window[1][left_slot];
    i_pixel G_right    = raw_window[1][right_slot];
    i_pixel G_top      = raw_window[top_slot][1];
    i_pixel G_bot      = raw_window[bottom_slot][1];

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
            if (green_col == width - 1) {
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

        i_pixel grad_h = (G_left > G_right)
                       ? (i_pixel)(G_left - G_right)
                       : (i_pixel)(G_right - G_left);
        i_pixel grad_v = (G_top > G_bot)
                       ? (i_pixel)(G_top - G_bot)
                       : (i_pixel)(G_bot - G_top);
        i_pixel diff = (grad_h > grad_v)
                     ? (i_pixel)(grad_h - grad_v)
                     : (i_pixel)(grad_v - grad_h);
        i_pixel grad_max = (grad_h > grad_v) ? grad_h : grad_v;

        if (grad_max < (i_pixel)edge_mag) {
            G_result = (G_H + G_V) >> 1;
            dir_chosen = dir_left;
        } else if (diff > (i_pixel)edge_threshold) {
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

    advance_position(green_row, green_col, width, height);
    return true;
}

// Tang 2:
//   {RAW, G} -> RGB
static bool interpolate_red_blue_step(
    const green_pixel_t &p_green,
    int width,
    int height,
    video_out_t &p_out
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

    input_col = (input_col == width - 1) ? 0 : input_col + 1;

    // Them width + 1 pixel {RAW,G} de co cua so RGB dau tien.
    bool emit_rgb = rgb_started;
    if (!rgb_started) {
        if (warmup_count == width + 1) {
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
    bool bottom_boundary = (rgb_row == height - 1);
    bool left_boundary = (rgb_col == 0);
    bool right_boundary = (rgb_col == width - 1);

    int left_slot = left_boundary ? 2 : 0;
    int right_slot = right_boundary ? 0 : 2;
    int top_slot = top_boundary ? 2 : 0;
    int bottom_slot = bottom_boundary ? 0 : 2;

    i_pixel raw_center = green_window[1][1].raw;
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
        avg_B = (ap_fixed<17,13>)(
            (green_window[top_slot][left_slot].raw
           + green_window[top_slot][right_slot].raw
           + green_window[bottom_slot][left_slot].raw
           + green_window[bottom_slot][right_slot].raw) >> 2);

        avg_G = (ap_fixed<17,13>)(
            (green_window[top_slot][left_slot].g
           + green_window[top_slot][right_slot].g
           + green_window[bottom_slot][left_slot].g
           + green_window[bottom_slot][right_slot].g) >> 2);

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
        avg_R = (ap_fixed<17,13>)(
            (green_window[top_slot][left_slot].raw
           + green_window[top_slot][right_slot].raw
           + green_window[bottom_slot][left_slot].raw
           + green_window[bottom_slot][right_slot].raw) >> 2);

        avg_G = (ap_fixed<17,13>)(
            (green_window[top_slot][left_slot].g
           + green_window[top_slot][right_slot].g
           + green_window[bottom_slot][left_slot].g
           + green_window[bottom_slot][right_slot].g) >> 2);

        calc_r = G_center + avg_R - avg_G;

        r_out = (calc_r < 0) ? (o_pixel)0 : (o_pixel)calc_r;
        g_out = (o_pixel)G_center;
        b_out = raw_center;
    }

    // Dong goi AXIS. TKEEP/TSTRB bao toan bo 36 bit RGB la hop le.
    p_out.data = 0;
    p_out.keep = -1;
    p_out.strb = -1;
    p_out.user = (rgb_row == 0 && rgb_col == 0) ? 1 : 0;
    p_out.last = (rgb_col == width - 1) ? 1 : 0;
    p_out.id = 0;
    p_out.dest = 0;
    axis_set_rgb(
        p_out,
        (ap_uint<12>)r_out,
        (ap_uint<12>)g_out,
        (ap_uint<12>)b_out
    );

    advance_position(rgb_row, rgb_col, width, height);
    return true;
}

void isp_demosaicing_top(
    hls::stream<video_in_t> &stream_in,
    hls::stream<video_out_t> &stream_out,
    int width,
    int height,
    int edge_threshold,
    int edge_mag
)
{
    #pragma HLS INTERFACE axis port=stream_in register_mode=off //do master từ testbench do cosim sinh ra không giữ data_valid trước đó khi T_READY từ slave chưa bật
    #pragma HLS INTERFACE axis port=stream_out

    // AXI-Lite nam o wrapper ISP. Core demosaic chi nhan cac day cau hinh
    // truc tiep va wrapper phai giu chung on dinh trong luc stream dang chay.
    #pragma HLS INTERFACE ap_none port=width
    #pragma HLS INTERFACE ap_none port=height
    #pragma HLS INTERFACE ap_none port=edge_threshold
    #pragma HLS INTERFACE ap_none port=edge_mag
    #pragma HLS INTERFACE ap_ctrl_none port=return

    #pragma HLS PIPELINE II=1

    video_in_t p_in = stream_in.read();

    green_pixel_t p_green;
    bool green_ready = interpolate_green_step(
        p_in,
        width,
        height,
        edge_threshold,
        edge_mag,
        p_green
    );

    if (green_ready) {
        video_out_t p_out;
        bool rgb_ready = interpolate_red_blue_step(
            p_green,
            width,
            height,
            p_out
        );

        if (rgb_ready) {
            stream_out.write(p_out);
        }
    }
    
}
