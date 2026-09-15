/*
Project: Adaptive Directional BPC and BLC
Module: BPC HLS Tests
Description: Check HLS pixel equivalence and full-frame stream behavior.
Author: Viet Nguyen To Quoc
*/

#include "isp_bpc.hpp"
#include "bpc_adaptive.hpp"
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <vector>

int passed = 0;
int failed = 0;
int printed_failures = 0;

constexpr int RANDOM_PIXEL_CASES = 8000;
constexpr int CONSTRAINED_PIXEL_CASES = 2000;
constexpr int MAX_FAILURE_LOGS = 10;
constexpr std::uint32_t PIXEL_SEED = 20260915;
constexpr std::uint32_t FRAME_SEED = 20260916;

struct PixelSamples {
    std::uint16_t center;
    std::uint16_t left;
    std::uint16_t right;
    std::uint16_t up;
    std::uint16_t down;
    std::uint16_t up_left;
    std::uint16_t up_right;
    std::uint16_t down_left;
    std::uint16_t down_right;
};

std::uint16_t random_raw12(std::mt19937& rng) {
    std::uniform_int_distribution<int> raw12_dist(0, 4095);
    int value = raw12_dist(rng);
    if ((value & 15) == 0) {
        std::uint16_t endpoints[4]{0, 1, 4094, 4095};
        return endpoints[(value >> 4) & 3];
    }
    return std::uint16_t(value);
}

adaptive_bpc::BpcConfig random_config(std::mt19937& rng) {
    std::uniform_int_distribution<int> shift_dist(0, 12);
    return {random_raw12(rng), random_raw12(rng), random_raw12(rng),
            std::uint8_t(shift_dist(rng)), std::uint8_t(shift_dist(rng))};
}

PixelSamples random_samples(std::mt19937& rng) {
    return {random_raw12(rng), random_raw12(rng), random_raw12(rng),
            random_raw12(rng), random_raw12(rng), random_raw12(rng),
            random_raw12(rng), random_raw12(rng), random_raw12(rng)};
}

void set_pair(PixelSamples& samples, int direction, std::uint16_t first, std::uint16_t second) {
    if (direction == 0) {
        samples.left = first;
        samples.right = second;
    } else if (direction == 1) {
        samples.up = first;
        samples.down = second;
    } else if (direction == 2) {
        samples.up_left = first;
        samples.down_right = second;
    } else {
        samples.up_right = first;
        samples.down_left = second;
    }
}

void write_reference_window(std::vector<std::uint16_t>& frame, int row, int col, PixelSamples samples) {
    frame[row * FRAME_WIDTH + col] = samples.center;
    frame[row * FRAME_WIDTH + col - 2] = samples.left;
    frame[row * FRAME_WIDTH + col + 2] = samples.right;
    frame[(row - 2) * FRAME_WIDTH + col] = samples.up;
    frame[(row + 2) * FRAME_WIDTH + col] = samples.down;
    frame[(row - 2) * FRAME_WIDTH + col - 2] = samples.up_left;
    frame[(row - 2) * FRAME_WIDTH + col + 2] = samples.up_right;
    frame[(row + 2) * FRAME_WIDTH + col - 2] = samples.down_left;
    frame[(row + 2) * FRAME_WIDTH + col + 2] = samples.down_right;
}

BpcWindow make_hls_window(PixelSamples samples) {
    return {ap_ufixed<12, 12>(samples.center), ap_ufixed<12, 12>(samples.left), ap_ufixed<12, 12>(samples.right),
            ap_ufixed<12, 12>(samples.up), ap_ufixed<12, 12>(samples.down), ap_ufixed<12, 12>(samples.up_left),
            ap_ufixed<12, 12>(samples.up_right), ap_ufixed<12, 12>(samples.down_left), ap_ufixed<12, 12>(samples.down_right)};
}

BpcConfig make_hls_config(adaptive_bpc::BpcConfig config) {
    return {ap_ufixed<12, 12>(config.base_threshold_r), ap_ufixed<12, 12>(config.base_threshold_g),
            ap_ufixed<12, 12>(config.base_threshold_b), ap_uint<4>(config.signal_shift), ap_uint<4>(config.activity_shift)};
}

void print_pixel_failure(std::string group, int case_index, int row, int col, PixelSamples samples,
                         adaptive_bpc::BpcConfig config, std::uint16_t expected, std::uint16_t actual) {
    if (printed_failures >= MAX_FAILURE_LOGS) return;
    printed_failures++;
    std::cout << group << " case=" << case_index << " row=" << row << " col=" << col << " samples={" << samples.center << "," << samples.left << "," << samples.right << "," << samples.up << "," << samples.down << "," << samples.up_left << "," << samples.up_right << "," << samples.down_left << "," << samples.down_right << "} config={" << config.base_threshold_r << "," << config.base_threshold_g << "," << config.base_threshold_b << "," << int(config.signal_shift) << "," << int(config.activity_shift) << "} expected=" << expected << " got=" << actual << "\n";
}

void check_pixel_equivalence(std::string group, int case_index, std::vector<std::uint16_t>& frame,
                             int row, int col, PixelSamples samples, adaptive_bpc::BpcConfig config) {
    write_reference_window(frame, row, col, samples);
    adaptive_bpc::BpcPixelResult reference_result = adaptive_bpc::bpc_pixel(frame, row, col, config);
    std::uint16_t hls_result = ::bpc_pixel(make_hls_window(samples), ap_uint<12>(row), ap_uint<12>(col), make_hls_config(config)).to_uint();
    if (hls_result == reference_result.value) {
        passed++;
    } else {
        failed++;
        print_pixel_failure(group, case_index, row, col, samples, config, reference_result.value, hls_result);
    }
}

void make_interior_coordinate(std::mt19937& rng, int phase, int& row, int& col) {
    std::uniform_int_distribution<int> row_dist(0, 537);
    std::uniform_int_distribution<int> col_dist(0, 957);
    row = 2 + 2 * row_dist(rng) + phase / 2;
    col = 2 + 2 * col_dist(rng) + phase % 2;
}

PixelSamples make_direction_case(std::mt19937& rng, int mode) {
    std::uniform_int_distribution<int> base_dist(0, 3000);
    int base = base_dist(rng);
    PixelSamples samples{4095, 0, 0, 0, 0, 0, 0, 0, 0};
    std::uint16_t first[4]{std::uint16_t(base + 100), std::uint16_t(base + 200), std::uint16_t(base + 300), std::uint16_t(base + 400)};
    std::uint16_t second[4]{std::uint16_t(base + 140), std::uint16_t(base + 230), std::uint16_t(base + 320), std::uint16_t(base + 410)};

    if (mode < 4) {
        second[mode] = first[mode] + 2;
    } else if (mode == 4) {
        for (int direction = 0; direction < 4; direction++) second[direction] = first[direction] + 4;
    } else if (mode == 5) {
        second[0] = first[0] + 40;
        for (int direction = 1; direction < 4; direction++) second[direction] = first[direction] + 4;
    } else {
        second[0] = first[0] + 40;
        second[1] = first[1] + 20;
        second[2] = first[2] + 4;
        second[3] = first[3] + 4;
    }

    for (int direction = 0; direction < 4; direction++) set_pair(samples, direction, first[direction], second[direction]);
    return samples;
}

void make_threshold_case(std::mt19937& rng, int variant, PixelSamples& samples, adaptive_bpc::BpcConfig& config) {
    std::uniform_int_distribution<int> prediction_dist(1024, 2048);
    std::uniform_int_distribution<int> threshold_dist(0, 128);
    std::uniform_int_distribution<int> shift_dist(4, 12);
    int prediction = prediction_dist(rng);
    int base_threshold = threshold_dist(rng);
    int signal_shift = shift_dist(rng);
    int threshold = base_threshold + (prediction >> signal_shift);
    int center = prediction;
    if (variant == 0) center = prediction + threshold;
    if (variant == 1) center = prediction + threshold + 1;
    if (variant == 2) center = prediction - threshold;
    if (variant == 3) center = prediction - threshold - 1;
    samples = {std::uint16_t(center), std::uint16_t(prediction), std::uint16_t(prediction),
               std::uint16_t(prediction), std::uint16_t(prediction), std::uint16_t(prediction),
               std::uint16_t(prediction), std::uint16_t(prediction), std::uint16_t(prediction)};
    config = {std::uint16_t(base_threshold), std::uint16_t(base_threshold), std::uint16_t(base_threshold),
              std::uint8_t(signal_shift), std::uint8_t(shift_dist(rng))};
}

void test_bpc_pixel() {
    //Fixed-seed broad and constrained equivalence
    std::mt19937 rng(PIXEL_SEED);
    std::vector<std::uint16_t> frame(std::size_t(FRAME_WIDTH) * FRAME_HEIGHT, 0);

    for (int i = 0; i < RANDOM_PIXEL_CASES; i++) {
        int row;
        int col;
        make_interior_coordinate(rng, i % 4, row, col);
        check_pixel_equivalence("pixel random", i, frame, row, col, random_samples(rng), random_config(rng));
    }

    for (int i = 0; i < CONSTRAINED_PIXEL_CASES; i++) {
        int row;
        int col;
        int mode = i % 8;
        make_interior_coordinate(rng, i % 4, row, col);
        PixelSamples samples;
        adaptive_bpc::BpcConfig config;
        if (mode < 7) {
            samples = make_direction_case(rng, mode);
            config = {0, 0, 0, 12, 12};
        } else {
            make_threshold_case(rng, (i / 8) % 4, samples, config);
        }
        check_pixel_equivalence("pixel constrained", i, frame, row, col, samples, config);
    }

    std::cout << "bpc_pixel: seed=" << PIXEL_SEED << " random=" << RANDOM_PIXEL_CASES << " constrained=" << CONSTRAINED_PIXEL_CASES << "\n";
}

void test_isp_bpc_top() {
    //One full-frame transaction for CSim and CoSim
    std::mt19937 rng(FRAME_SEED);
    std::size_t pixel_count = std::size_t(FRAME_WIDTH) * FRAME_HEIGHT;
    adaptive_bpc::BpcConfig config{16, 16, 16, 3, 0};
    std::vector<std::uint16_t> frame(pixel_count);
    std::vector<std::uint16_t> expected;
    std::vector<adaptive_bpc::Detection> detections;
    hls::stream<ap_axiu<16, 1, 0, 0>> input_stream;
    hls::stream<ap_axiu<16, 1, 0, 0>> output_stream;

    for (std::size_t i = 0; i < pixel_count; i++) frame[i] = random_raw12(rng);
    adaptive_bpc::bpc_frame(frame, expected, detections, config);

    //Input metadata differs from regenerated output coordinates
    for (std::size_t i = 0; i < pixel_count; i++) {
        ap_axiu<16, 1, 0, 0> packet{};
        packet.data = frame[i];
        packet.keep = 3;
        packet.strb = 3;
        packet.user = (i % 97) == 0;
        packet.last = (i % 257) == 0;
        input_stream.write(packet);
    }

    isp_bpc_top(input_stream, output_stream, ap_ufixed<12, 12>(config.base_threshold_r), ap_ufixed<12, 12>(config.base_threshold_g),
                ap_ufixed<12, 12>(config.base_threshold_b), ap_uint<4>(config.signal_shift), ap_uint<4>(config.activity_shift));

    for (std::size_t i = 0; i < pixel_count; i++) {
        if (output_stream.empty()) {
            failed++;
            if (printed_failures < MAX_FAILURE_LOGS) {
                printed_failures++;
                std::cout << "isp_bpc_top: missing output pixel=" << i << "\n";
            }
            break;
        }

        ap_axiu<16, 1, 0, 0> packet = output_stream.read();
        bool match = packet.data.to_uint() == expected[i] && packet.keep == 3 && packet.strb == 3 &&
                     packet.user == (i == 0) && packet.last == (i % FRAME_WIDTH == FRAME_WIDTH - 1);
        if (match) {
            passed++;
        } else {
            failed++;
            if (printed_failures < MAX_FAILURE_LOGS) {
                printed_failures++;
                std::cout << "isp_bpc_top: pixel=" << i << " expected=" << expected[i] << " got=" << packet.data.to_uint() << " user=" << packet.user << " last=" << packet.last << " keep=" << packet.keep << " strb=" << packet.strb << "\n";
            }
        }
    }

    if (!output_stream.empty()) {
        failed++;
        if (printed_failures < MAX_FAILURE_LOGS) {
            printed_failures++;
            std::cout << "isp_bpc_top: extra output\n";
        }
    }
    std::cout << "isp_bpc_top: seed=" << FRAME_SEED << " pixels=" << pixel_count << " detections=" << detections.size() << "\n";
}

int finish_tests() {
    std::cout << "Summary: " << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}

int main(int argc, char* argv[]) {
    std::string mode = argc > 1 ? argv[1] : "all";
    if (mode == "pixel" || mode == "all") test_bpc_pixel();
    if (mode == "top" || mode == "all") test_isp_bpc_top();
    if (mode != "pixel" && mode != "top" && mode != "all") {
        std::cout << "Unknown mode: " << mode << "\n";
        return 1;
    }
    return finish_tests();
}
