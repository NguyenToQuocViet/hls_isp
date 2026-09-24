/*
Project: Adaptive Directional BPC and BLC
Module: BLC Reference Tests
Description: Check Black Level Correction reference behavior with directed cases.
Author: Viet Nguyen To Quoc
*/

#include "blc.hpp"

#include <array>
#include <iostream>
#include <string>
#include <vector>

int passed = 0;
int failed = 0;

void expect_equal(
    std::string name,
    std::size_t actual,
    std::size_t expected
) {
    if (actual == expected) {
        passed++;
        std::cout << name << ": PASS\n";
    } else {
        failed++;
        std::cout << name << ": FAIL (expected " << expected
                  << ", got " << actual << ")\n";
    }
}

void test_cluster(
    std::string name,
    std::size_t row,
    std::size_t col,
    blc::BlcConfig config,
    std::array<std::uint16_t, 4> input,
    std::array<std::uint16_t, 4> expected
) {
    expect_equal(
        name + " R",
        blc::blc_pixel(input[0], row, col, config),
        expected[0]
    );

    expect_equal(
        name + " Gr",
        blc::blc_pixel(input[1], row, col + 1, config),
        expected[1]
    );

    expect_equal(
        name + " Gb",
        blc::blc_pixel(input[2], row + 1, col, config),
        expected[2]
    );

    expect_equal(
        name + " B",
        blc::blc_pixel(input[3], row + 1, col + 1, config),
        expected[3]
    );
}

int main() {
    blc::BlcConfig config_1{20, 30, 40, 50};
    blc::BlcConfig config_2{0, 100, 200, 300};
    blc::BlcConfig config_3{1023, 7, 60, 1000};
    blc::BlcConfig config_4{64, 66, 65, 68};

    //1920x1080 corner RGGB clusters
    test_cluster(
        "top-left",
        0,
        0,
        config_1,
        {120, 328, 900, 1000},
        {100, 298, 860, 950}
    );

    test_cluster(
        "top-right",
        0,
        1918,
        config_2,
        {0, 1000, 400, 500},
        {0, 900, 200, 200}
    );

    test_cluster(
        "bottom-left",
        1078,
        0,
        config_3,
        {1023, 100, 900, 1023},
        {0, 93, 840, 23}
    );

    test_cluster(
        "bottom-right",
        1078,
        1918,
        config_4,
        {500, 700, 1000, 1023},
        {436, 634, 935, 955}
    );

    //Fixed-position RGGB clusters
    test_cluster(
        "position-1 (126,348)",
        126,
        348,
        config_1,
        {19, 30, 41, 1023},
        {0, 0, 1, 973}
    );

    test_cluster(
        "position-2 (364,1350)",
        364,
        1350,
        config_2,
        {123, 99, 200, 301},
        {123, 0, 0, 1}
    );

    test_cluster(
        "position-3 (738,92)",
        738,
        92,
        config_3,
        {1022, 8, 59, 1001},
        {0, 1, 0, 1}
    );

    test_cluster(
        "position-4 (946,1672)",
        946,
        1672,
        config_4,
        {65, 67, 66, 69},
        {1, 1, 1, 1}
    );

    //Unsigned underflow saturation
    expect_equal(
        "underflow R",
        blc::blc_pixel(0, 0, 0, config_1),
        0
    );

    expect_equal(
        "underflow Gr",
        blc::blc_pixel(99, 0, 1, config_2),
        0
    );

    expect_equal(
        "underflow Gb",
        blc::blc_pixel(59, 1, 0, config_3),
        0
    );

    expect_equal(
        "underflow B",
        blc::blc_pixel(67, 1, 1, config_4),
        0
    );

    //5x5 RGGB frame
    std::vector<std::uint16_t> frame_input{
        120, 29, 20, 31, 1023,
        39, 50, 41, 51, 0,
        21, 30, 19, 1023, 100,
        200, 49, 40, 1023, 70,
        0, 100, 1023, 0, 20
    };

    std::vector<std::uint16_t> frame_expected{
        100, 0, 0, 1, 1003,
        0, 0, 1, 1, 0,
        1, 0, 0, 993, 80,
        160, 0, 0, 973, 30,
        0, 70, 1003, 0, 0
    };

    std::vector<std::uint16_t> frame_output;

    blc::blc_frame(
        frame_input,
        frame_output,
        5,
        5,
        config_1
    );

    expect_equal(
        "blc_frame 5x5 size",
        frame_output.size(),
        frame_expected.size()
    );

    if (frame_output.size() == frame_expected.size()) {
        for (std::size_t i = 0; i < frame_expected.size(); i++) {
            expect_equal(
                "blc_frame 5x5 pixel " + std::to_string(i),
                frame_output[i],
                frame_expected[i]
            );
        }
    }

    //Output vector reuse
    std::vector<std::uint16_t> one_pixel{120};

    blc::blc_frame(
        one_pixel,
        frame_output,
        1,
        1,
        config_1
    );

    expect_equal(
        "blc_frame 1x1 size after reuse",
        frame_output.size(),
        1
    );

    if (frame_output.size() == 1) {
        expect_equal(
            "blc_frame 1x1 pixel",
            frame_output[0],
            100
        );
    }

    std::cout << "Summary: " << passed
              << " passed, " << failed
              << " failed\n";

    return failed == 0 ? 0 : 1;
}