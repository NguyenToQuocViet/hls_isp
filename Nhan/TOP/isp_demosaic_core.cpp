#include "isp_cores.h"

typedef ap_ufixed<12, 12> demosaic_input_t;
typedef ap_ufixed<12, 12, AP_RND, AP_SAT> demosaic_output_t;
typedef ap_ufixed<16, 12> green_value_t;

struct green_pixel_t {
    demosaic_input_t raw;
    green_value_t g;
};

struct green_state_t {
    int input_col;
    int warmup_count;
    bool started;
    int output_row;
    int output_col;
    ap_uint<1> dir_left;
    ap_uint<1> prev_dir_left_cache;
    ap_uint<1> prev_dir_center_cache;
    ap_uint<1> next_row_center_seed;
    ap_uint<1> dir_write_bank;
};

struct rgb_state_t {
    int input_col;
    int warmup_count;
    bool started;
    int output_row;
    int output_col;
};

static void advance_position(int &row, int &col) {
    #pragma HLS INLINE

    if (col == ISP_FRAME_WIDTH - 1) {
        col = 0;
        row = (row == ISP_FRAME_HEIGHT - 1) ? 0 : row + 1;
    } else {
        col = col + 1;
    }
}

static bool interpolate_green_step(
    const raw12_packet_t &p_in,
    int edge_threshold,
    int edge_mag,
    green_state_t &state,
    green_pixel_t &p_green
) {
    #pragma HLS INLINE

    static demosaic_input_t raw_line_1[ISP_FRAME_WIDTH];
    static demosaic_input_t raw_line_2[ISP_FRAME_WIDTH];
    #pragma HLS BIND_STORAGE variable=raw_line_1 type=RAM_S2P impl=BRAM
    #pragma HLS BIND_STORAGE variable=raw_line_2 type=RAM_S2P impl=BRAM

    static demosaic_input_t raw_window[3][3];
    #pragma HLS ARRAY_PARTITION variable=raw_window complete dim=0

    static ap_uint<1> dir_bank0[ISP_FRAME_WIDTH];
    static ap_uint<1> dir_bank1[ISP_FRAME_WIDTH];
    #pragma HLS BIND_STORAGE variable=dir_bank0 type=RAM_S2P impl=LUTRAM
    #pragma HLS BIND_STORAGE variable=dir_bank1 type=RAM_S2P impl=LUTRAM
    #pragma HLS DEPENDENCE variable=dir_bank0 inter false
    #pragma HLS DEPENDENCE variable=dir_bank1 inter false

    const int input_col = p_in.user ? 0 : state.input_col;
    const demosaic_input_t raw_in = (demosaic_input_t)p_in.data;
    const demosaic_input_t raw_m1 = raw_line_1[input_col];
    const demosaic_input_t raw_m2 = raw_line_2[input_col];

    raw_line_2[input_col] = raw_m1;
    raw_line_1[input_col] = raw_in;

    raw_window[0][0] = raw_window[0][1];
    raw_window[0][1] = raw_window[0][2];
    raw_window[0][2] = raw_m2;
    raw_window[1][0] = raw_window[1][1];
    raw_window[1][1] = raw_window[1][2];
    raw_window[1][2] = raw_m1;
    raw_window[2][0] = raw_window[2][1];
    raw_window[2][1] = raw_window[2][2];
    raw_window[2][2] = raw_in;

    state.input_col = p_in.last ? 0 : input_col + 1;

    bool emit_green = state.started;
    if (!state.started) {
        if (state.warmup_count == ISP_FRAME_WIDTH + 1) {
            state.started = true;
            emit_green = true;
        } else {
            state.warmup_count = state.warmup_count + 1;
        }
    }

    if (!emit_green) {
        return false;
    }

    const bool row_even = ((state.output_row & 1) == 0);
    const bool col_even = ((state.output_col & 1) == 0);
    const bool rb_site = (row_even == col_even);
    const bool top_boundary = (state.output_row == 0);
    const bool bottom_boundary =
        (state.output_row == ISP_FRAME_HEIGHT - 1);
    const bool left_boundary = (state.output_col == 0);
    const bool right_boundary =
        (state.output_col == ISP_FRAME_WIDTH - 1);

    // Mirror-101 cho mau RAW/G.
    const int left_slot = left_boundary ? 2 : 0;
    const int right_slot = right_boundary ? 0 : 2;
    const int top_slot = top_boundary ? 2 : 0;
    const int bottom_slot = bottom_boundary ? 0 : 2;

    const demosaic_input_t raw_center = raw_window[1][1];
    const demosaic_input_t g_left = raw_window[1][left_slot];
    const demosaic_input_t g_right = raw_window[1][right_slot];
    const demosaic_input_t g_top = raw_window[top_slot][1];
    const demosaic_input_t g_bottom = raw_window[bottom_slot][1];

    const int dir_read_addr = right_boundary
        ? state.output_col : state.output_col + 1;
    ap_uint<1> dir_read_value;
    if (state.dir_write_bank == 0) {
        dir_read_value = dir_bank1[dir_read_addr];
    } else {
        dir_read_value = dir_bank0[dir_read_addr];
    }

    ap_uint<1> prev_dir_left = 0;
    ap_uint<1> prev_dir_center = 0;
    ap_uint<1> prev_dir_right = 0;

    if (state.output_col == 0) {
        state.dir_left = 0;
    }

    if (!top_boundary) {
        if (state.output_col == 0) {
            prev_dir_center = state.next_row_center_seed;
            prev_dir_right = dir_read_value;
            prev_dir_left = prev_dir_center;
        } else {
            prev_dir_left = state.prev_dir_left_cache;
            prev_dir_center = state.prev_dir_center_cache;
            prev_dir_right = right_boundary
                ? prev_dir_center : dir_read_value;
        }
    }

    state.prev_dir_left_cache = prev_dir_center;
    state.prev_dir_center_cache = prev_dir_right;

    green_value_t g_result;
    ap_uint<1> dir_chosen;

    if (!rb_site) {
        g_result = raw_center;
        dir_chosen = state.dir_left;
    } else {
        const green_value_t g_h =
            (green_value_t)((g_left + g_right) >> 1);
        const green_value_t g_v =
            (green_value_t)((g_top + g_bottom) >> 1);

        const demosaic_input_t grad_h = (g_left > g_right)
            ? (demosaic_input_t)(g_left - g_right)
            : (demosaic_input_t)(g_right - g_left);
        const demosaic_input_t grad_v = (g_top > g_bottom)
            ? (demosaic_input_t)(g_top - g_bottom)
            : (demosaic_input_t)(g_bottom - g_top);
        const demosaic_input_t diff = (grad_h > grad_v)
            ? (demosaic_input_t)(grad_h - grad_v)
            : (demosaic_input_t)(grad_v - grad_h);
        const demosaic_input_t grad_max =
            (grad_h > grad_v) ? grad_h : grad_v;

        if (grad_max < (demosaic_input_t)edge_mag) {
            g_result = (g_h + g_v) >> 1;
            dir_chosen = state.dir_left;
        } else if (diff > (demosaic_input_t)edge_threshold) {
            dir_chosen = (grad_h > grad_v) ? 1 : 0;
            g_result = dir_chosen ? g_v : g_h;
        } else {
            ap_uint<3> vote;
            if (top_boundary) {
                vote = state.dir_left;
            } else {
                vote = prev_dir_left + prev_dir_center
                     + prev_dir_right + state.dir_left;
            }
            dir_chosen = (vote > 2) ? 1 : 0;
            g_result = dir_chosen ? g_v : g_h;
        }
    }

    if (state.dir_write_bank == 0) {
        dir_bank0[state.output_col] = dir_chosen;
    } else {
        dir_bank1[state.output_col] = dir_chosen;
    }

    if (state.output_col == 0) {
        state.next_row_center_seed = dir_chosen;
    }
    if (right_boundary) {
        state.dir_write_bank = state.dir_write_bank ^ 1;
    }
    state.dir_left = dir_chosen;

    p_green.raw = raw_center;
    p_green.g = g_result;
    advance_position(state.output_row, state.output_col);
    return true;
}

static bool interpolate_red_blue_step(
    const green_pixel_t &p_green,
    rgb_state_t &state,
    rgb36_packet_t &p_out
) {
    #pragma HLS INLINE

    static green_pixel_t green_line_1[ISP_FRAME_WIDTH];
    static green_pixel_t green_line_2[ISP_FRAME_WIDTH];
    #pragma HLS BIND_STORAGE variable=green_line_1 type=RAM_S2P impl=BRAM
    #pragma HLS BIND_STORAGE variable=green_line_2 type=RAM_S2P impl=BRAM

    static green_pixel_t green_window[3][3];
    #pragma HLS ARRAY_PARTITION variable=green_window complete dim=0

    const green_pixel_t green_m1 = green_line_1[state.input_col];
    const green_pixel_t green_m2 = green_line_2[state.input_col];

    green_line_2[state.input_col] = green_m1;
    green_line_1[state.input_col] = p_green;

    green_window[0][0] = green_window[0][1];
    green_window[0][1] = green_window[0][2];
    green_window[0][2] = green_m2;
    green_window[1][0] = green_window[1][1];
    green_window[1][1] = green_window[1][2];
    green_window[1][2] = green_m1;
    green_window[2][0] = green_window[2][1];
    green_window[2][1] = green_window[2][2];
    green_window[2][2] = p_green;

    state.input_col = (state.input_col == ISP_FRAME_WIDTH - 1)
        ? 0 : state.input_col + 1;

    bool emit_rgb = state.started;
    if (!state.started) {
        if (state.warmup_count == ISP_FRAME_WIDTH + 1) {
            state.started = true;
            emit_rgb = true;
        } else {
            state.warmup_count = state.warmup_count + 1;
        }
    }

    if (!emit_rgb) {
        return false;
    }

    const bool row_even = ((state.output_row & 1) == 0);
    const bool col_even = ((state.output_col & 1) == 0);
    const bool top_boundary = (state.output_row == 0);
    const bool bottom_boundary =
        (state.output_row == ISP_FRAME_HEIGHT - 1);
    const bool left_boundary = (state.output_col == 0);
    const bool right_boundary =
        (state.output_col == ISP_FRAME_WIDTH - 1);

    const int left_slot = left_boundary ? 2 : 0;
    const int right_slot = right_boundary ? 0 : 2;
    const int top_slot = top_boundary ? 2 : 0;
    const int bottom_slot = bottom_boundary ? 0 : 2;

    const demosaic_input_t raw_center = green_window[1][1].raw;
    const green_value_t g_center_u = green_window[1][1].g;
    const ap_fixed<17, 13> g_center = g_center_u;

    ap_fixed<17, 13> avg_b;
    ap_fixed<17, 13> avg_r;
    ap_fixed<17, 13> avg_g;
    ap_fixed<17, 13> avg_g_for_b;
    ap_fixed<17, 13> avg_g_for_r;
    ap_fixed<19, 15> calc_b;
    ap_fixed<19, 15> calc_r;
    demosaic_output_t r_out;
    demosaic_output_t g_out;
    demosaic_output_t b_out;

    if (row_even && col_even) {
        avg_b = (ap_fixed<17, 13>)(
            (green_window[top_slot][left_slot].raw
           + green_window[top_slot][right_slot].raw
           + green_window[bottom_slot][left_slot].raw
           + green_window[bottom_slot][right_slot].raw) >> 2);
        avg_g = (ap_fixed<17, 13>)(
            (green_window[top_slot][left_slot].g
           + green_window[top_slot][right_slot].g
           + green_window[bottom_slot][left_slot].g
           + green_window[bottom_slot][right_slot].g) >> 2);
        calc_b = g_center + avg_b - avg_g;
        r_out = raw_center;
        g_out = (demosaic_output_t)g_center;
        b_out = (calc_b < 0)
            ? (demosaic_output_t)0 : (demosaic_output_t)calc_b;
    } else if (row_even && !col_even) {
        avg_r = (ap_fixed<17, 13>)(
            (green_window[1][left_slot].raw
           + green_window[1][right_slot].raw) >> 1);
        avg_g_for_r = (ap_fixed<17, 13>)(
            (green_window[1][left_slot].g
           + green_window[1][right_slot].g) >> 1);
        calc_r = g_center + avg_r - avg_g_for_r;
        avg_b = (ap_fixed<17, 13>)(
            (green_window[top_slot][1].raw
           + green_window[bottom_slot][1].raw) >> 1);
        avg_g_for_b = (ap_fixed<17, 13>)(
            (green_window[top_slot][1].g
           + green_window[bottom_slot][1].g) >> 1);
        calc_b = g_center + avg_b - avg_g_for_b;
        r_out = (calc_r < 0)
            ? (demosaic_output_t)0 : (demosaic_output_t)calc_r;
        g_out = (demosaic_output_t)g_center;
        b_out = (calc_b < 0)
            ? (demosaic_output_t)0 : (demosaic_output_t)calc_b;
    } else if (!row_even && col_even) {
        avg_r = (ap_fixed<17, 13>)(
            (green_window[top_slot][1].raw
           + green_window[bottom_slot][1].raw) >> 1);
        avg_g_for_r = (ap_fixed<17, 13>)(
            (green_window[top_slot][1].g
           + green_window[bottom_slot][1].g) >> 1);
        calc_r = g_center + avg_r - avg_g_for_r;
        avg_b = (ap_fixed<17, 13>)(
            (green_window[1][left_slot].raw
           + green_window[1][right_slot].raw) >> 1);
        avg_g_for_b = (ap_fixed<17, 13>)(
            (green_window[1][left_slot].g
           + green_window[1][right_slot].g) >> 1);
        calc_b = g_center + avg_b - avg_g_for_b;
        r_out = (calc_r < 0)
            ? (demosaic_output_t)0 : (demosaic_output_t)calc_r;
        g_out = (demosaic_output_t)g_center;
        b_out = (calc_b < 0)
            ? (demosaic_output_t)0 : (demosaic_output_t)calc_b;
    } else {
        avg_r = (ap_fixed<17, 13>)(
            (green_window[top_slot][left_slot].raw
           + green_window[top_slot][right_slot].raw
           + green_window[bottom_slot][left_slot].raw
           + green_window[bottom_slot][right_slot].raw) >> 2);
        avg_g = (ap_fixed<17, 13>)(
            (green_window[top_slot][left_slot].g
           + green_window[top_slot][right_slot].g
           + green_window[bottom_slot][left_slot].g
           + green_window[bottom_slot][right_slot].g) >> 2);
        calc_r = g_center + avg_r - avg_g;
        r_out = (calc_r < 0)
            ? (demosaic_output_t)0 : (demosaic_output_t)calc_r;
        g_out = (demosaic_output_t)g_center;
        b_out = raw_center;
    }

    p_out.data = 0;
    p_out.user =
        (state.output_row == 0 && state.output_col == 0) ? 1 : 0;
    p_out.last =
        (state.output_col == ISP_FRAME_WIDTH - 1) ? 1 : 0;
    rgb_set(
        p_out,
        (ap_uint<12>)r_out,
        (ap_uint<12>)g_out,
        (ap_uint<12>)b_out
    );

    advance_position(state.output_row, state.output_col);
    return true;
}

void isp_demosaic_process(
    hls::stream<raw12_packet_t> &stream_in,
    hls::stream<rgb36_packet_t> &stream_out,
    int edge_threshold,
    int edge_mag,
    frame_count_t frame_count
) {
    #pragma HLS INLINE off

    const unsigned real_pixels =
        (unsigned)frame_count * (unsigned)ISP_FRAME_PIXELS;
    const unsigned total_steps = real_pixels + ISP_DEMOSAIC_DELAY;

    green_state_t green_state = {};
    rgb_state_t rgb_state = {};

DEMOSAIC_STREAM_LOOP:
    for (unsigned step = 0; step < total_steps; ++step) {
        #pragma HLS PIPELINE II=1

        raw12_packet_t p_in;
        if (step < real_pixels) {
            p_in = stream_in.read();
        } else {
            //padding de day toan bo du lieu con trong pipeline
            const unsigned padding_index = step - real_pixels;
            p_in.data = 0;
            p_in.user = (padding_index == 0) ? 1 : 0;
            p_in.last =
                ((padding_index % ISP_FRAME_WIDTH) == ISP_FRAME_WIDTH - 1)
                ? 1 : 0;
        }

        green_pixel_t p_green;
        const bool green_valid = interpolate_green_step(
            p_in,
            edge_threshold,
            edge_mag,
            green_state,
            p_green
        );

        if (green_valid) {
            rgb36_packet_t p_out;
            const bool rgb_valid = interpolate_red_blue_step(
                p_green,
                rgb_state,
                p_out
            );
            if (rgb_valid) {
                stream_out.write(p_out);
            }
        }
    }
}
