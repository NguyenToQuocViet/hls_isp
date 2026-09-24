/*
Project: Adaptive Directional BPC and BLC
Module: Adaptive BPC Reference Tests
Description: Check adaptive directional BPC reference behavior with directed cases.
Author: Viet Nguyen To Quoc
*/

#include "bpc_adaptive.hpp"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

int passed = 0;
int failed = 0;

int width = 1920;
int height = 1080;

struct Neighborhood {
    std::uint16_t center;
    std::uint16_t h_left;
    std::uint16_t h_right;
    std::uint16_t v_up;
    std::uint16_t v_down;
    std::uint16_t d1_up_left;
    std::uint16_t d1_down_right;
    std::uint16_t d2_up_right;
    std::uint16_t d2_down_left;
};

void record_case(
    std::string name,
    bool match
) {
    if (match) {
        passed++;
        std::cout << name << ": PASS\n";
    } else {
        failed++;
        std::cout << name << ": FAIL\n";
    }
}

void fill_window(
    std::vector<std::uint16_t>& frame,
    int row,
    int col,
    std::uint16_t value
) {
    for (int row_offset = -2; row_offset <= 2; row_offset++) {
        for (int col_offset = -2; col_offset <= 2; col_offset++) {
            frame[(row + row_offset) * width + col + col_offset] = value;
        }
    }
}

void fill_window_pattern(
    std::vector<std::uint16_t>& frame,
    int row,
    int col
) {
    for (int row_offset = -2; row_offset <= 2; row_offset++) {
        for (int col_offset = -2; col_offset <= 2; col_offset++) {
            int pattern =
                (row_offset + 2) * 5 +
                col_offset + 2;

            frame[(row + row_offset) * width + col + col_offset] =
                pattern % 2 == 0 ? 0 : 1023;
        }
    }
}

void set_neighborhood(
    std::vector<std::uint16_t>& frame,
    int row,
    int col,
    Neighborhood neighborhood
) {
    int index = row * width + col;

    frame[index] = neighborhood.center;

    frame[index - 2] = neighborhood.h_left;
    frame[index + 2] = neighborhood.h_right;

    frame[index - 2 * width] = neighborhood.v_up;
    frame[index + 2 * width] = neighborhood.v_down;

    frame[index - 2 * width - 2] = neighborhood.d1_up_left;
    frame[index + 2 * width + 2] = neighborhood.d1_down_right;

    frame[index - 2 * width + 2] = neighborhood.d2_up_right;
    frame[index + 2 * width - 2] = neighborhood.d2_down_left;
}

void check_prepared_pixel(
    std::string name,
    std::vector<std::uint16_t>& frame,
    int row,
    int col,
    adaptive_bpc::BpcConfig config,
    std::uint16_t expected_value,
    bool expected_detected
) {
    adaptive_bpc::BpcPixelResult actual =
        adaptive_bpc::bpc_pixel(
            frame,
            row,
            col,
            config
        );

    bool match =
        actual.value == expected_value &&
        actual.detected == expected_detected;

    if (!match) {
        std::cout << name
                  << ": expected={"
                  << expected_value << ","
                  << expected_detected << "}"
                  << " got={"
                  << actual.value << ","
                  << actual.detected << "}"
                  << "\n";
    }

    record_case(name, match);
}

void check_pixel(
    std::string name,
    std::vector<std::uint16_t>& frame,
    int row,
    int col,
    Neighborhood neighborhood,
    adaptive_bpc::BpcConfig config,
    std::uint16_t expected_value,
    bool expected_detected
) {
    fill_window(
        frame,
        row,
        col,
        777
    );

    set_neighborhood(
        frame,
        row,
        col,
        neighborhood
    );

    check_prepared_pixel(
        name,
        frame,
        row,
        col,
        config,
        expected_value,
        expected_detected
    );
}

void check_border(
    std::string name,
    std::vector<std::uint16_t>& frame,
    int row,
    int col,
    std::uint16_t value
) {
    frame[row * width + col] = value;

    adaptive_bpc::BpcPixelResult actual =
        adaptive_bpc::bpc_pixel(
            frame,
            row,
            col,
            {0, 0, 0, 0, 0}
        );

    bool match =
        actual.value == value &&
        !actual.detected;

    if (!match) {
        std::cout << name
                  << ": expected={"
                  << value << ",0}"
                  << " got={"
                  << actual.value << ","
                  << actual.detected << "}"
                  << "\n";
    }

    record_case(name, match);
}

void test_direction_and_prediction(
    std::vector<std::uint16_t>& frame
) {
    //Direction selection, tie priority and wide pair sum
    adaptive_bpc::BpcConfig config{0, 0, 0, 10, 10};

    check_pixel(
        "direction H",
        frame,
        100,
        100,
        {1023, 100, 104, 200, 210, 300, 320, 400, 430},
        config,
        102,
        true
    );

    check_pixel(
        "direction V",
        frame,
        100,
        100,
        {1023, 100, 110, 200, 204, 300, 320, 400, 430},
        config,
        202,
        true
    );

    check_pixel(
        "direction D1",
        frame,
        100,
        100,
        {1023, 100, 110, 200, 220, 300, 304, 400, 430},
        config,
        302,
        true
    );

    check_pixel(
        "direction D2",
        frame,
        100,
        100,
        {1023, 100, 110, 200, 220, 300, 330, 400, 404},
        config,
        402,
        true
    );

    check_pixel(
        "tie all prefers H",
        frame,
        100,
        100,
        {1023, 100, 104, 200, 204, 300, 304, 400, 404},
        config,
        102,
        true
    );

    check_pixel(
        "tie V D1 D2 prefers V",
        frame,
        100,
        100,
        {1023, 100, 110, 200, 204, 300, 304, 400, 404},
        config,
        202,
        true
    );

    check_pixel(
        "tie D1 D2 prefers D1",
        frame,
        100,
        100,
        {1023, 100, 110, 200, 220, 300, 304, 400, 404},
        config,
        302,
        true
    );

    check_pixel(
        "prediction odd sum floors",
        frame,
        100,
        100,
        {1023, 100, 101, 200, 210, 300, 320, 400, 430},
        config,
        100,
        true
    );

    check_pixel(
        "prediction upper odd sum",
        frame,
        100,
        100,
        {0, 1022, 1023, 0, 10, 0, 20, 0, 30},
        config,
        1022,
        true
    );

    check_pixel(
        "prediction maximum even sum",
        frame,
        100,
        100,
        {0, 1023, 1023, 0, 1, 0, 2, 0, 3},
        config,
        1023,
        true
    );

    check_pixel(
        "direction reversed operands",
        frame,
        100,
        100,
        {1023, 104, 100, 200, 210, 300, 320, 400, 430},
        config,
        102,
        true
    );
}

void test_detection_boundary(
    std::vector<std::uint16_t>& frame
) {
    //P=800, G_min=8, T=10+(800>>4)+(8>>3)=61
    Neighborhood neighborhood{
        800,
        796,
        804,
        796,
        804,
        796,
        804,
        796,
        804
    };

    adaptive_bpc::BpcConfig config{10, 10, 10, 4, 3};

    check_pixel(
        "center equals prediction",
        frame,
        100,
        100,
        neighborhood,
        config,
        800,
        false
    );

    neighborhood.center = 861;

    check_pixel(
        "hot residual equals threshold",
        frame,
        100,
        100,
        neighborhood,
        config,
        861,
        false
    );

    neighborhood.center = 862;

    check_pixel(
        "hot residual exceeds threshold",
        frame,
        100,
        100,
        neighborhood,
        config,
        800,
        true
    );

    neighborhood.center = 739;

    check_pixel(
        "dead residual equals threshold",
        frame,
        100,
        100,
        neighborhood,
        config,
        739,
        false
    );

    neighborhood.center = 738;

    check_pixel(
        "dead residual exceeds threshold",
        frame,
        100,
        100,
        neighborhood,
        config,
        800,
        true
    );
}

void test_adaptive_terms(
    std::vector<std::uint16_t>& frame
) {
    //Shift truncation, endpoint shifts and wide threshold
    check_pixel(
        "signal term 15 shift 4",
        frame,
        100,
        100,
        {16, 15, 15, 15, 15, 15, 15, 15, 15},
        {0, 0, 0, 4, 10},
        15,
        true
    );

    check_pixel(
        "signal term 16 shift 4",
        frame,
        100,
        100,
        {17, 16, 16, 16, 16, 16, 16, 16, 16},
        {0, 0, 0, 4, 10},
        17,
        false
    );

    check_pixel(
        "activity term 7 shift 3",
        frame,
        100,
        100,
        {101, 97, 104, 200, 220, 300, 330, 400, 440},
        {0, 0, 0, 10, 3},
        100,
        true
    );

    check_pixel(
        "activity term 8 shift 3",
        frame,
        100,
        100,
        {101, 96, 104, 200, 220, 300, 330, 400, 440},
        {0, 0, 0, 10, 3},
        101,
        false
    );

    check_pixel(
        "signal shift zero equality",
        frame,
        100,
        100,
        {200, 100, 100, 100, 100, 100, 100, 100, 100},
        {0, 0, 0, 0, 10},
        200,
        false
    );

    check_pixel(
        "signal shift zero exceed",
        frame,
        100,
        100,
        {201, 100, 100, 100, 100, 100, 100, 100, 100},
        {0, 0, 0, 0, 10},
        100,
        true
    );

    check_pixel(
        "wide adaptive threshold",
        frame,
        100,
        100,
        {1023, 0, 1023, 0, 1023, 0, 1023, 0, 1023},
        {1023, 1023, 1023, 0, 0},
        1023,
        false
    );

    check_pixel(
        "maximum base threshold equality",
        frame,
        100,
        100,
        {1023, 0, 0, 0, 0, 0, 0, 0, 0},
        {1023, 1023, 1023, 10, 10},
        1023,
        false
    );
}

void test_cfa_thresholds(
    std::vector<std::uint16_t>& frame
) {
    //Each phase must select only its configured base threshold
    Neighborhood neighborhood{
        101,
        100,
        100,
        100,
        100,
        100,
        100,
        100,
        100
    };

    check_pixel(
        "CFA R low threshold",
        frame,
        100,
        100,
        neighborhood,
        {0, 1023, 1023, 10, 10},
        100,
        true
    );

    check_pixel(
        "CFA Gr low threshold",
        frame,
        100,
        101,
        neighborhood,
        {1023, 0, 1023, 10, 10},
        100,
        true
    );

    check_pixel(
        "CFA Gb low threshold",
        frame,
        101,
        100,
        neighborhood,
        {1023, 0, 1023, 10, 10},
        100,
        true
    );

    check_pixel(
        "CFA B low threshold",
        frame,
        101,
        101,
        neighborhood,
        {1023, 1023, 0, 10, 10},
        100,
        true
    );

    check_pixel(
        "CFA R high threshold",
        frame,
        100,
        100,
        neighborhood,
        {1023, 0, 0, 10, 10},
        101,
        false
    );

    check_pixel(
        "CFA Gr high threshold",
        frame,
        100,
        101,
        neighborhood,
        {0, 1023, 0, 10, 10},
        101,
        false
    );

    check_pixel(
        "CFA Gb high threshold",
        frame,
        101,
        100,
        neighborhood,
        {0, 1023, 0, 10, 10},
        101,
        false
    );

    check_pixel(
        "CFA B high threshold",
        frame,
        101,
        101,
        neighborhood,
        {0, 0, 1023, 10, 10},
        101,
        false
    );
}

void test_neighborhood_addressing(
    std::vector<std::uint16_t>& frame
) {
    //Non-same-CFA window samples must not affect the result
    Neighborhood neighborhood{
        1023,
        100,
        104,
        200,
        210,
        300,
        320,
        400,
        430
    };

    adaptive_bpc::BpcConfig config{0, 0, 0, 10, 10};

    fill_window(
        frame,
        100,
        100,
        0
    );

    set_neighborhood(
        frame,
        100,
        100,
        neighborhood
    );

    check_prepared_pixel(
        "unused window samples zero",
        frame,
        100,
        100,
        config,
        102,
        true
    );

    fill_window_pattern(
        frame,
        100,
        100
    );

    set_neighborhood(
        frame,
        100,
        100,
        neighborhood
    );

    check_prepared_pixel(
        "unused window samples alternating",
        frame,
        100,
        100,
        config,
        102,
        true
    );
}

void test_border(
    std::vector<std::uint16_t>& frame
) {
    //Two-pixel border bypass and first legal interior centers
    check_border("border top-left", frame, 0, 0, 0);
    check_border("border top-right", frame, 0, width - 1, 1023);
    check_border("border bottom-left", frame, height - 1, 0, 1023);
    check_border("border bottom-right", frame, height - 1, width - 1, 0);

    check_border("border row 0", frame, 0, 100, 1023);
    check_border("border row 1", frame, 1, 100, 0);
    check_border("border row height-2", frame, height - 2, 100, 1023);
    check_border("border row height-1", frame, height - 1, 100, 0);

    check_border("border col 0", frame, 100, 0, 1023);
    check_border("border col 1", frame, 100, 1, 0);
    check_border("border col width-2", frame, 100, width - 2, 1023);
    check_border("border col width-1", frame, 100, width - 1, 0);

    Neighborhood neighborhood{
        1023,
        100,
        100,
        100,
        100,
        100,
        100,
        100,
        100
    };

    adaptive_bpc::BpcConfig config{0, 0, 0, 10, 10};

    check_pixel(
        "interior top-left",
        frame,
        2,
        2,
        neighborhood,
        config,
        100,
        true
    );

    check_pixel(
        "interior top-right",
        frame,
        2,
        width - 3,
        neighborhood,
        config,
        100,
        true
    );

    check_pixel(
        "interior bottom-left",
        frame,
        height - 3,
        2,
        neighborhood,
        config,
        100,
        true
    );

    check_pixel(
        "interior bottom-right",
        frame,
        height - 3,
        width - 3,
        neighborhood,
        config,
        100,
        true
    );
}

bool detections_match(
    std::vector<adaptive_bpc::Detection>& actual,
    std::vector<adaptive_bpc::Detection>& expected
) {
    if (actual.size() != expected.size()) {
        return false;
    }

    for (std::size_t i = 0; i < expected.size(); i++) {
        if (
            actual[i].row != expected[i].row ||
            actual[i].col != expected[i].col ||
            actual[i].original != expected[i].original ||
            actual[i].replacement != expected[i].replacement
        ) {
            return false;
        }
    }

    return true;
}

void test_frame_flat_and_reuse() {
    //Flat frame must clear and replace caller-owned outputs
    std::vector<std::uint16_t> input(width * height, 1000);
    std::vector<std::uint16_t> output{1, 2, 3};
    std::vector<adaptive_bpc::Detection> detections{{1, 2, 3, 4}};

    adaptive_bpc::bpc_frame(
        input,
        output,
        detections,
        {10, 20, 30, 4, 3}
    );

    record_case(
        "frame flat and output reuse",
        output == input &&
        detections.empty()
    );
}

void test_frame_isolated_defects() {
    //Eight isolated hot/dead defects across all CFA phases
    std::vector<std::uint16_t> input(width * height, 1000);

    input[100 * width + 100] = 1023;
    input[100 * width + 301] = 1023;
    input[301 * width + 100] = 1023;
    input[301 * width + 301] = 1023;

    input[600 * width + 600] = 0;
    input[600 * width + 901] = 0;
    input[901 * width + 600] = 0;
    input[901 * width + 901] = 0;

    std::vector<std::uint16_t> original = input;
    std::vector<std::uint16_t> expected(width * height, 1000);
    std::vector<std::uint16_t> output;

    std::vector<adaptive_bpc::Detection> detections;

    std::vector<adaptive_bpc::Detection> expected_detections{
        {100, 100, 1023, 1000},
        {100, 301, 1023, 1000},
        {301, 100, 1023, 1000},
        {301, 301, 1023, 1000},
        {600, 600, 0, 1000},
        {600, 901, 0, 1000},
        {901, 600, 0, 1000},
        {901, 901, 0, 1000}
    };

    adaptive_bpc::bpc_frame(
        input,
        output,
        detections,
        {0, 0, 0, 10, 10}
    );

    bool match =
        output == expected &&
        input == original &&
        detections_match(
            detections,
            expected_detections
        );

    if (!match) {
        std::cout << "frame isolated defects: output="
                  << (output == expected)
                  << " input=" << (input == original)
                  << " detections=" << detections.size()
                  << "\n";
    }

    record_case(
        "frame isolated defects",
        match
    );
}

void test_frame_border_extremes() {
    //Sparse border extremes must remain unchanged
    std::vector<std::uint16_t> input(width * height, 1000);

    input[0] = 0;
    input[width - 1] = 1023;
    input[(height - 1) * width] = 1023;
    input[height * width - 1] = 0;

    input[100] = 1023;
    input[width + 301] = 0;
    input[(height - 2) * width + 600] = 1023;
    input[(height - 1) * width + 901] = 0;

    input[100 * width] = 1023;
    input[301 * width + 1] = 0;
    input[600 * width + width - 2] = 1023;
    input[901 * width + width - 1] = 0;

    std::vector<std::uint16_t> output;
    std::vector<adaptive_bpc::Detection> detections;

    adaptive_bpc::bpc_frame(
        input,
        output,
        detections,
        {0, 0, 0, 10, 10}
    );

    record_case(
        "frame sparse border extremes",
        output == input &&
        detections.empty()
    );
}

void test_frame_alias_rejection() {
    //Input and output must be distinct objects
    std::vector<std::uint16_t> frame(width * height, 1000);
    std::vector<adaptive_bpc::Detection> detections;

    bool rejected = false;

    try {
        adaptive_bpc::bpc_frame(
            frame,
            frame,
            detections,
            {0, 0, 0, 10, 10}
        );
    } catch (const std::invalid_argument&) {
        rejected = true;
    } catch (...) {
        rejected = false;
    }

    record_case(
        "frame alias rejection",
        rejected
    );
}

int finish_tests() {
    std::cout << "Summary: "
              << passed << " passed, "
              << failed << " failed\n";

    return failed == 0 ? 0 : 1;
}

int main() {
    std::vector<std::uint16_t> frame(
        width * height,
        0
    );

    test_direction_and_prediction(frame);
    test_detection_boundary(frame);
    test_adaptive_terms(frame);
    test_cfa_thresholds(frame);
    test_neighborhood_addressing(frame);
    test_border(frame);

    test_frame_flat_and_reuse();
    test_frame_isolated_defects();
    test_frame_border_extremes();
    test_frame_alias_rejection();

    return finish_tests();
}