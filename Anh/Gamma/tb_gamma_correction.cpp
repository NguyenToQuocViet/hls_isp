#include "../header.hpp"
#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <cmath>
#include <hls_stream.h>
#include <iostream>
using namespace std;

// Define AXI-Stream structs based on module signature
typedef ap_axiu<36, 1, 0, 0> axis_pixel_12b;
typedef ap_axiu<24, 1, 0, 0> axis_pixel_8b;

#ifdef GAMMA
int main()
{
    std::cout << "GAMMA CORRECTION TESTBENCH" << endl;
    hls::stream<axis_pixel_12b> src("stream_in");
    hls::stream<axis_pixel_8b> dst("stream_out");
    ap_uint<8> gamma_lut[4096];

    // 1. Calculate Gamma LUT using double precision math
    std::cout << "Generating Gamma LUT (Double Precision)..." << std::endl;
    double gamma = 0.45; // Standard gamma curve (approx 1/2.2)
    for (int i = 0; i < 4096; i++)
    {
        // Normalize 12-bit index to [0.0, 1.0]
        double normalized = (double)i / 4095.0;
        // Apply power function and scale to 8-bit [0.0, 255.0]
        double val = std::pow(normalized, gamma) * 255.0;
        // Round to nearest integer and cast to uint8
        gamma_lut[i] = (ap_uint<8>)(std::round(val));
    }

    int wait = 1000;
    while (wait)
        wait--;
    // 2. Generate input test data
    // NOTE: We must generate exactly MAX_HEIGHT * MAX_WIDTH pixels because
    // the hardware module uses hardcoded for-loops for the frame resolution.
    std::cout << "Generating " << MAX_WIDTH << "x" << MAX_HEIGHT
              << " Input Stream..." << std::endl;

    for (int i = 0; i < MAX_HEIGHT; i++)
    {
        for (int j = 0; j < MAX_WIDTH; j++)
        {
            axis_pixel_12b pixel_in;

            // Generate a deterministic gradient pattern to test all channels
            ap_uint<12> r_val = (i + j) % 4096;
            ap_uint<12> g_val = (i * j) % 4096;
            ap_uint<12> b_val = (4095 - j) % 4096;

            // Pack into 36-bit data bus (R at LSB, B at MSB)
            pixel_in.data.range(11, 0) = r_val;
            pixel_in.data.range(23, 12) = g_val;
            pixel_in.data.range(35, 24) = b_val;

            // Set AXI sideband signals
            pixel_in.user =
                (i == 0 && j == 0) ? 1 : 0; // TUSER = Start of Frame
            pixel_in.last = (j == MAX_WIDTH - 1) ? 1 : 0; // TLAST = End of Line

            src.write(pixel_in);
        }
    }

    // 3. Call the hardware module
    std::cout << "Running Hardware Module (C-Simulation)..." << std::endl;
    gamma_correction(src, dst, gamma_lut, gamma_lut, gamma_lut, MAX_HEIGHT,
                     MAX_WIDTH);

    // 4. Verify output data against Software Golden Model
    std::cout << "Verifying Output Stream..." << std::endl;
    int error_count = 0;

    for (int i = 0; i < MAX_HEIGHT; i++)
    {
        for (int j = 0; j < MAX_WIDTH; j++)
        {
            axis_pixel_8b pixel_out = dst.read();

            // Re-calculate the expected input values
            ap_uint<12> r_val = (i + j) % 4096;
            ap_uint<12> g_val = (i * j) % 4096;
            ap_uint<12> b_val = (4095 - j) % 4096;

            // The golden expected output from the software LUT
            ap_uint<8> exp_r = gamma_lut[r_val];
            ap_uint<8> exp_g = gamma_lut[g_val];
            ap_uint<8> exp_b = gamma_lut[b_val];

            // The actual output from the hardware module
            ap_uint<8> act_r = pixel_out.data.range(7, 0);
            ap_uint<8> act_g = pixel_out.data.range(15, 8);
            ap_uint<8> act_b = pixel_out.data.range(23, 16);

            // Verify Pixel Data
            if (act_r != exp_r || act_g != exp_g || act_b != exp_b)
            {
                error_count++;
                if (error_count <= 10)
                { // Limit error prints to avoid spam
                    std::cout << "Data Error at (" << i << "," << j
                              << "): " << "Expected [R=" << exp_r
                              << " G=" << exp_g << " B=" << exp_b << "] "
                              << "Got [R=" << act_r << " G=" << act_g
                              << " B=" << act_b << "]" << std::endl;
                }
            }

            // Verify AXI Sideband Signals
            if (pixel_out.user != (i == 0 && j == 0))
            {
                std::cout << "TUSER (SOF) flag incorrect at (" << i << "," << j
                          << ")" << std::endl;
                error_count++;
            }
            if (pixel_out.last != (j == MAX_WIDTH - 1))
            {
                std::cout << "TLAST (EOL) flag incorrect at (" << i << "," << j
                          << ")" << std::endl;
                error_count++;
            }
        }
    }

    // Vitis HLS Testbenches MUST return 0 to pass!
    if (error_count == 0)
    {
        std::cout << "=========================================" << std::endl;
        std::cout << "Test Passed Successfully! (0 Errors)" << std::endl;
        std::cout << "=========================================" << std::endl;
        return 0;
    }
    else
    {
        std::cout << "=========================================" << std::endl;
        std::cout << "Test Failed with " << error_count << " errors!"
                  << std::endl;
        std::cout << "=========================================" << std::endl;
        return 1;
    }
}
#endif