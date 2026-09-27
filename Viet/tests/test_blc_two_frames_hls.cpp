/*
Project: Adaptive Directional BPC and BLC
Module: Two-Frame BLC HLS Test
Description: Compare two complete RAW10 HLS frames with the independent BLC reference model.
Author: Viet Nguyen To Quoc
*/

#include "blc_top.hpp"
#include "isp_frame.hpp"
#include "blc.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>

static std::uint16_t random_pixel(std::size_t index) {
    //Fixed seed: CSim and CoSim replay the same two images.
    std::uint32_t value = std::uint32_t(index) + 0x6d2b79f5u;
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    value ^= value >> 16;
    return std::uint16_t(value & 0x3ffu);
}

int main() {
    constexpr int FRAME_COUNT = 2;
    const std::size_t frame_pixels = std::size_t(FRAME_WIDTH) * FRAME_HEIGHT;
    const std::size_t total_pixels = FRAME_COUNT * frame_pixels;
    const blc::BlcConfig config{64, 66, 65, 68};

    hls::stream<ap_axiu<16, 1, 0, 0>> input;
    hls::stream<ap_axiu<16, 1, 0, 0>> output;
    hls::ap_none<ap_uint<10>> bl_r;
    hls::ap_none<ap_uint<10>> bl_gr;
    hls::ap_none<ap_uint<10>> bl_gb;
    hls::ap_none<ap_uint<10>> bl_b;

    bl_r.write(config.black_level_r);
    bl_gr.write(config.black_level_gr);
    bl_gb.write(config.black_level_gb);
    bl_b.write(config.black_level_b);

    //Allow AXI-Lite configuration writes to complete before the first SOF.
    constexpr int PREAMBLE_PIXELS = 64;
    for (int index = 0; index < PREAMBLE_PIXELS; index++) {
        ap_axiu<16, 1, 0, 0> pixel{};
        pixel.keep = 0b11;
        pixel.strb = 0b11;
        input.write(pixel);
    }

    for (std::size_t index = 0; index < total_pixels; index++) {
        const std::size_t position = index % frame_pixels;
        ap_axiu<16, 1, 0, 0> pixel{};
        pixel.data = random_pixel(index);
        pixel.keep = 0b11;
        pixel.strb = 0b11;
        pixel.user = position == 0;
        pixel.last = position % FRAME_WIDTH == FRAME_WIDTH - 1;
        input.write(pixel);
    }

    blc_top(
        input, output,
        bl_r, bl_gr, bl_gb, bl_b
    );

    int failures = 0;

    for (std::size_t index = 0; index < total_pixels; index++) {
        const std::size_t position = index % frame_pixels;
        const std::size_t row = position / FRAME_WIDTH;
        const std::size_t col = position % FRAME_WIDTH;
        const std::uint16_t expected = blc::blc_pixel(
            random_pixel(index), row, col, config
        );
        const ap_axiu<16, 1, 0, 0> actual = output.read();

        if (actual.data.to_uint() != expected ||
            actual.keep != 0b11 ||
            actual.strb != 0b11 ||
            actual.user != (position == 0) ||
            actual.last != (col == FRAME_WIDTH - 1)) {
            failures++;

            if (failures <= 10) {
                std::cout << "Pixel " << index
                          << ": expected=" << expected
                          << " actual=" << actual.data.to_uint()
                          << " user=" << actual.user
                          << " last=" << actual.last << '\n';
            }
        }
    }

    if (!output.empty()) {
        failures++;
        std::cout << "Unexpected number of input/output pixels\n";
    }

    std::cout << "BLC two " << FRAME_WIDTH << 'x' << FRAME_HEIGHT
              << " frames: " << total_pixels
              << " pixels checked, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
