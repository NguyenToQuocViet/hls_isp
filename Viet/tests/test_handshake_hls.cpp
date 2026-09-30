/*
Project: Adaptive Directional BPC and BLC
Module: Handshake HLS Equivalence Test
Description: Replay saved RAW10 frames through a handshake top and check every AXIS packet against Golden.
Author: Viet Nguyen To Quoc
*/

#include "isp_frame.hpp"

#ifndef HS_TEST_BPC
#error "Select the block with -DHS_TEST_BPC=0 (BLC) or 1 (BPC)"
#endif

#if HS_TEST_BPC
#include "bpc_top.hpp"
#include "bpc_adaptive.hpp"
static constexpr const char* BLOCK = "bpc";
#else
#include "blc_top.hpp"
#include "blc.hpp"
static constexpr const char* BLOCK = "blc";
#endif

#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

static std::vector<std::uint16_t> read_pixels(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("cannot open input: " + path);
    }
    std::vector<std::uint16_t> pixels(FRAME_WIDTH * FRAME_HEIGHT);
    for (auto& pixel : pixels) {
        const int low = file.get();
        const int high = file.get();
        if (low < 0 || high < 0) {
            throw std::runtime_error("truncated input: " + path);
        }
        pixel = static_cast<std::uint16_t>(low | (high << 8));
        if (pixel > 1023) {
            throw std::runtime_error("input outside RAW10: " + path);
        }
    }
    if (file.get() != std::char_traits<char>::eof()) {
        throw std::runtime_error("extra input bytes: " + path);
    }
    return pixels;
}

static bool check_frame(unsigned index, std::uint64_t seed,
                        const std::array<unsigned, 5>& config,
                        const std::string& path) {
    const auto pixels = read_pixels(path);
    std::vector<std::uint16_t> expected;
    std::size_t branch_count = 0;

#if HS_TEST_BPC
    if (config[0] > 1023 || config[1] > 1023 || config[2] > 1023 ||
        config[3] > 15 || config[4] > 15) {
        throw std::runtime_error("invalid BPC config");
    }
    const adaptive_bpc::BpcConfig golden_config{
        static_cast<std::uint16_t>(config[0]), static_cast<std::uint16_t>(config[1]),
        static_cast<std::uint16_t>(config[2]), static_cast<std::uint8_t>(config[3]),
        static_cast<std::uint8_t>(config[4])
    };
    {
        std::vector<adaptive_bpc::Detection> detections;
        adaptive_bpc::bpc_frame(pixels, expected, detections, golden_config,
                                FRAME_WIDTH, FRAME_HEIGHT);
        branch_count = detections.size();
    }
#else
    if (config[0] > 1023 || config[1] > 1023 || config[2] > 1023 ||
        config[3] > 1023 || config[4] != 0) {
        throw std::runtime_error("invalid BLC config");
    }
    const blc::BlcConfig golden_config{
        static_cast<std::uint16_t>(config[0]), static_cast<std::uint16_t>(config[1]),
        static_cast<std::uint16_t>(config[2]), static_cast<std::uint16_t>(config[3])
    };
    blc::blc_frame(pixels, expected, FRAME_WIDTH, FRAME_HEIGHT, golden_config);
    for (const auto pixel : expected) {
        branch_count += pixel == 0;
    }
#endif

    std::cout << "HS_START block=" << BLOCK << " index=" << index
              << " seed=" << seed << " config=";
    for (const auto value : config) {
        std::cout << value << ',';
    }
    std::cout << std::endl;

    //Only this frame is supplied. BPC must generate its own drain advances.
    hls::stream<ap_axiu<16, 1, 0, 0>> input;
    hls::stream<ap_axiu<16, 1, 0, 0>> output;
    for (std::size_t position = 0; position < pixels.size(); position++) {
        ap_axiu<16, 1, 0, 0> packet{};
        packet.data = pixels[position];
        packet.keep = 0b11;
        packet.strb = 0b11;
        packet.user = position == 0;
        packet.last = position % FRAME_WIDTH == FRAME_WIDTH - 1;
        input.write(packet);
    }
#if HS_TEST_BPC
    bpc_top(input, output, config[0], config[1], config[2], config[3], config[4]);
#else
    blc_top(input, output, config[0], config[1], config[2], config[3]);
#endif

    std::size_t checked = 0;
    std::size_t mismatches = 0;
    while (checked < expected.size() && !output.empty()) {
        const auto packet = output.read();
        const auto row = checked / FRAME_WIDTH;
        const auto col = checked % FRAME_WIDTH;
        const bool user = checked == 0;
        const bool last = col == FRAME_WIDTH - 1;
        //Compare full 16-bit TDATA as well as every defined sideband.
        if (packet.data.to_uint() != expected[checked] ||
            packet.user != user || packet.last != last ||
            packet.keep != 0b11 || packet.strb != 0b11) {
            mismatches++;
            if (mismatches <= 8) {
                std::cout << "HS_MISMATCH index=" << index << " row=" << row
                          << " col=" << col << " expected(data,user,last,keep,strb)="
                          << expected[checked] << ',' << user << ',' << last << ",3,3"
                          << " actual=" << packet.data << ',' << packet.user << ','
                          << packet.last << ',' << packet.keep << ',' << packet.strb << '\n';
            }
        }
        checked++;
    }
    std::size_t extras = 0;
    while (!output.empty()) {
        output.read();
        extras++;
    }
    const bool remaining_input = !input.empty();
    std::cout << "HS_FRAME block=" << BLOCK << " index=" << index << " seed=" << seed
              << " checked=" << checked << " missing=" << expected.size() - checked
              << " mismatches=" << mismatches << " extras=" << extras
              << " remaining_input=" << remaining_input;
#if HS_TEST_BPC
    const std::size_t interior = (FRAME_WIDTH - 4) * (FRAME_HEIGHT - 4);
    std::cout << " corrected=" << branch_count << " kept=" << pixels.size() - branch_count
              << " interior_kept=" << interior - branch_count
              << " border=" << pixels.size() - interior;
#else
    std::cout << " clamped_zero=" << branch_count
              << " positive=" << pixels.size() - branch_count;
#endif
    std::cout << std::endl;
    return checked == expected.size() && mismatches == 0 && extras == 0 && !remaining_input;
}

int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            throw std::runtime_error("usage: test_handshake_hls <manifest.txt>");
        }
        std::ifstream manifest(argv[1]);
        std::string magic;
        std::string block;
        int width = 0;
        int height = 0;
        unsigned frames = 0;
        if (!(manifest >> magic >> block >> width >> height >> frames) ||
            magic != "HS_V1" || block != BLOCK || width != FRAME_WIDTH ||
            height != FRAME_HEIGHT || (frames != 1 && frames != 3)) {
            throw std::runtime_error("manifest does not match compiled block/geometry/count");
        }
        for (unsigned frame = 0; frame < frames; frame++) {
            unsigned index = 0;
            std::uint64_t seed = 0;
            std::array<unsigned, 5> config{};
            std::string path;
            if (!(manifest >> index >> seed >> config[0] >> config[1] >> config[2]
                           >> config[3] >> config[4] >> path)) {
                throw std::runtime_error("truncated manifest");
            }
            //Repeated calls in this process preserve DUT static RAM/window.
            if (!check_frame(index, seed, config, path)) {
                std::cout << "HS_CHECKER_FAIL block=" << BLOCK << std::endl;
                return 1;
            }
        }
        std::string trailing;
        if (manifest >> trailing) {
            throw std::runtime_error("extra manifest records");
        }
        //This is a Golden checker verdict, never by itself a RTL verdict.
        std::cout << "HS_CHECKER_PASS block=" << BLOCK << " frames=" << frames
                  << " pixels=" << std::size_t(frames) * FRAME_WIDTH * FRAME_HEIGHT
                  << std::endl;
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "HS_TEST_ERROR " << error.what() << '\n';
        return 2;
    }
}
