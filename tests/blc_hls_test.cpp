/*
Project: Adaptive Directional BPC and BLC
Module: BLC HLS Tests
Description: Check HLS pixel equivalence and full-frame stream behavior.
Author: Viet Nguyen To Quoc
*/

#include "isp_blc.hpp"
#include "blc.hpp"

#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <vector>

int passed = 0;
int failed = 0;

void check_case(
    std::string name,
    std::uint16_t input,
    std::size_t row,
    std::size_t col,
    blc::BlcConfig config
) {
    BlcConfig hls_config{
        ap_ufixed<10, 10>(config.black_level_r),
        ap_ufixed<10, 10>(config.black_level_gr),
        ap_ufixed<10, 10>(config.black_level_gb),
        ap_ufixed<10, 10>(config.black_level_b)
    };

    std::uint16_t expected = blc::blc_pixel(
        input,
        row,
        col,
        config
    );

    std::uint16_t actual = ::blc_pixel(
        ap_ufixed<10, 10>(input),
        ap_uint<11>(row),
        ap_uint<11>(col),
        hls_config
    ).to_uint();

    if (actual == expected) {
        passed++;
    } else {
        failed++;

        std::cout << name
                  << ": FAIL input=" << input
                  << " row=" << row
                  << " col=" << col
                  << " BL={"
                  << config.black_level_r << ","
                  << config.black_level_gr << ","
                  << config.black_level_gb << ","
                  << config.black_level_b << "}"
                  << " expected=" << expected
                  << " got=" << actual
                  << "\n";
    }
}

void test_blc_pixel() {
    //Directed CFA and saturation boundaries
    blc::BlcConfig config{20, 30, 40, 50};
    std::uint16_t black_levels[4]{20, 30, 40, 50};

    for (int phase = 0; phase < 4; phase++) {
        std::size_t row = phase / 2;
        std::size_t col = phase % 2;
        std::string name = "phase " + std::to_string(phase);

        check_case(
            name + " zero",
            0,
            row,
            col,
            config
        );

        check_case(
            name + " BL-1",
            black_levels[phase] - 1,
            row,
            col,
            config
        );

        check_case(
            name + " BL",
            black_levels[phase],
            row,
            col,
            config
        );

        check_case(
            name + " BL+1",
            black_levels[phase] + 1,
            row,
            col,
            config
        );

        check_case(
            name + " RAW10 max",
            1023,
            row,
            col,
            config
        );
    }

    check_case(
        "zero BL passthrough",
        1023,
        0,
        0,
        {0, 0, 0, 0}
    );

    check_case(
        "max BL saturation",
        1023,
        0,
        0,
        {1023, 1023, 1023, 1023}
    );

    //Fixed-seed RAW10 equivalence sweep
    std::uint32_t seed = 20260914;
    int random_cases = 1000;

    std::mt19937 rng(seed);

    std::uniform_int_distribution<int> raw10_dist(0, 1023);
    std::uniform_int_distribution<int> row_dist(0, FRAME_HEIGHT - 1);
    std::uniform_int_distribution<int> col_dist(0, FRAME_WIDTH - 1);

    for (int i = 0; i < random_cases; i++) {
        std::uint16_t input = raw10_dist(rng);
        std::size_t row = row_dist(rng);
        std::size_t col = col_dist(rng);

        blc::BlcConfig random_config{
            std::uint16_t(raw10_dist(rng)),
            std::uint16_t(raw10_dist(rng)),
            std::uint16_t(raw10_dist(rng)),
            std::uint16_t(raw10_dist(rng))
        };

        check_case(
            "random " + std::to_string(i),
            input,
            row,
            col,
            random_config
        );
    }

    std::cout << "blc_pixel: seed=" << seed
              << ", random=" << random_cases
              << "\n";
}

void test_isp_blc_top() {
    blc::BlcConfig config{20, 30, 40, 50};

    std::size_t pixels =
        std::size_t(FRAME_WIDTH) * FRAME_HEIGHT;

    std::vector<std::uint16_t> frame(pixels);
    std::vector<std::uint16_t> expected;

    hls::stream<ap_axiu<16, 1, 0, 0>> input_stream;
    hls::stream<ap_axiu<16, 1, 0, 0>> output_stream;

    //Full-frame RAW10 pattern and CFA boundaries
    for (std::size_t row = 0; row < FRAME_HEIGHT; row++) {
        for (std::size_t col = 0; col < FRAME_WIDTH; col++) {
            frame[row * FRAME_WIDTH + col] =
                (row * 13 + col * 7) % 1024;
        }
    }

    frame[0] = 0;
    frame[1] = 30;
    frame[FRAME_WIDTH] = 41;
    frame[FRAME_WIDTH + 1] = 49;
    frame[pixels - 1] = 1023;

    blc::blc_frame(
        frame,
        expected,
        FRAME_WIDTH,
        FRAME_HEIGHT,
        config
    );

    //Drive one AXI beat per pixel
    for (std::size_t i = 0; i < pixels; i++) {
        ap_axiu<16, 1, 0, 0> packet{};

        packet.data = frame[i];
        packet.keep = 3;
        packet.strb = 3;
        packet.user = (i == 0);
        packet.last = (i % FRAME_WIDTH == FRAME_WIDTH - 1);

        input_stream.write(packet);
    }

    isp_blc_top(
        input_stream,
        output_stream,
        ap_ufixed<10, 10>(config.black_level_r),
        ap_ufixed<10, 10>(config.black_level_gr),
        ap_ufixed<10, 10>(config.black_level_gb),
        ap_ufixed<10, 10>(config.black_level_b)
    );

    //Compare payload and preserved sidebands
    for (std::size_t i = 0; i < pixels; i++) {
        if (output_stream.empty()) {
            failed++;

            std::cout << "isp_blc_top: FAIL missing output at pixel="
                      << i
                      << "\n";

            break;
        }

        ap_axiu<16, 1, 0, 0> packet = output_stream.read();

        bool match =
            packet.data.to_uint() == expected[i] &&
            packet.keep == 3 &&
            packet.strb == 3 &&
            packet.user == (i == 0) &&
            packet.last == (i % FRAME_WIDTH == FRAME_WIDTH - 1);

        if (match) {
            passed++;
        } else {
            failed++;

            if (failed <= 10) {
                std::cout << "isp_blc_top: FAIL pixel=" << i
                          << " expected=" << expected[i]
                          << " got=" << packet.data.to_uint()
                          << " user=" << packet.user
                          << " last=" << packet.last
                          << " keep=" << packet.keep
                          << " strb=" << packet.strb
                          << "\n";
            }
        }
    }

    if (!output_stream.empty()) {
        failed++;
        std::cout << "isp_blc_top: FAIL extra output\n";
    }
}

int finish_tests() {
    std::cout << "Summary: " << passed
              << " passed, " << failed
              << " failed\n";

    return failed == 0 ? 0 : 1;
}

int main() {
    test_blc_pixel();
    test_isp_blc_top();

    return finish_tests();
}