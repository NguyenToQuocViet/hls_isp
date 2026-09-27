#include "../ltm_gamma.hpp"
#include <algorithm>
#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <cmath>
#include <dirent.h>
#include <fstream>
#include <hls_stream.h>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

    std::string in_dir_gamma =
        "/home/asura/Work/ISP/hls_isp/Anh/dataset/input_csim/Sequence_small/";
    std::string out_dir_gamma =
        "/home/asura/Work/ISP/hls_isp/Anh/dataset/output_csim/Gamma/Sequence_small/";
    std::string out_dir_golden_gamma = "/home/asura/Work/ISP/hls_isp/Anh/dataset/output_csim_golden/Gamma/Sequence_small/golden_gamma_output.ppm";


#ifdef GAMMA
int main()
{
    std::cout << "GAMMA CORRECTION TESTBENCH" << endl;
    hls::stream<gamma_in_t> src("stream_in");
    hls::stream<gamma_out_t> dst("stream_out");


    std::vector<std::string> bin_files;
    DIR* dir = opendir(in_dir_gamma.c_str());
    if (dir != nullptr)
    {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr)
        {
            std::string filename = entry->d_name;
            if (filename.length() >= 4 &&
                filename.substr(filename.length() - 4) == ".bin")
            {
                bin_files.push_back(filename);
            }
        }
        closedir(dir);
    }

    if (bin_files.empty())
    {
        std::cerr << "No .bin files found in " << in_dir_gamma << std::endl;
        return 1;
    }

    // Sort naturally
    std::sort(bin_files.begin(), bin_files.end(),
              [](const std::string& a, const std::string& b)
              {
                  if (a.length() != b.length())
                  {
                      return a.length() < b.length();
                  }
                  return a < b;
              });

    int frames_to_process = bin_files.size();
    std::cout << "Streaming " << frames_to_process << " frames continuously..."
              << std::endl;

    for (int f = 0; f < frames_to_process; f++)
    {
        std::string filename = bin_files[f];
        std::string in_path = in_dir_gamma + filename;

        FILE* f_in = fopen(in_path.c_str(), "rb");
        if (!f_in)
        {
            std::cerr << "Error opening " << in_path << std::endl;
            return 1;
        }

        int header[2];
        if (fread(header, sizeof(int), 2, f_in) != 2)
        {
            std::cerr << "Error reading header from " << in_path << std::endl;
            fclose(f_in);
            return 1;
        }
        int width = header[0];
        int height = header[1];

        if (width != WIDTH || height != HEIGHT)
        {
            std::cerr << "Error: Binary file dimensions (" << width << "x"
                      << height << ") differ from HEIGHT/WIDTH (" << HEIGHT
                      << "x" << WIDTH << ")." << std::endl;
            fclose(f_in);
            return 1;
        }

        std::vector<uint16_t> input_image(WIDTH * HEIGHT * 3);
        if (fread(input_image.data(), sizeof(uint16_t), input_image.size(),
                  f_in) != input_image.size())
        {
            std::cerr << "Error reading image data from " << in_path
                      << std::endl;
            fclose(f_in);
            return 1;
        }
        fclose(f_in);

        for (int i = 0; i < HEIGHT; i++)
        {
            for (int j = 0; j < WIDTH; j++)
            {
                gamma_in_t pixel_in;

                int idx = (i * WIDTH + j) * 3;
                pixel_in.data.range(11, 0) = std::min((uint16_t)4095, input_image[idx + 0]);
                pixel_in.data.range(23, 12) = std::min((uint16_t)4095, input_image[idx + 1]);
                pixel_in.data.range(35, 24) = std::min((uint16_t)4095, input_image[idx + 2]);

                pixel_in.user = (i == 0 && j == 0) ? 1 : 0;
                pixel_in.last = (j == WIDTH - 1) ? 1 : 0;
#ifdef USE_AP_AXIU
                pixel_in.keep = -1;
                pixel_in.strb = -1;
#endif

                src.write(pixel_in);
            }
        }
    }

    // 3. Call the hardware module for the entire stream
    std::cout << "Running Hardware Module (C-Simulation)..." << std::endl;
#ifdef DEBUG
    volatile int debug_pixels = 0;
#endif

#ifdef VER2
    isp_gamma_top_ver2(src, dst);
#else
    for (int f = 0; f < frames_to_process; f++)
    {
        isp_gamma_top(src, dst);
    }
#endif

    // 4. Extract outputs and save to PPM
    std::cout << "Saving Output Streams and Checking against Golden..." << std::endl;
    std::string out_path = out_dir_gamma + "gamma_output.ppm";
    std::ofstream out_img(out_path);
    if (!out_img.is_open())
    {
        std::cerr << "Error opening " << out_path << " for writing." << std::endl;
        return 1;
    }

    std::ifstream golden_img(out_dir_golden_gamma);
    if (!golden_img.is_open())
    {
        std::cerr << "Error opening golden file " << out_dir_golden_gamma << " for reading." << std::endl;
        return 1;
    }

    std::string magic;
    int g_width, g_height, max_val;
    golden_img >> magic >> g_width >> g_height >> max_val;

    if (magic != "P3" || g_width != WIDTH || g_height != HEIGHT * frames_to_process)
    {
        std::cerr << "Error: Golden file header mismatch!" << std::endl;
        return 1;
    }

    out_img << "P3\n" << WIDTH << " " << HEIGHT * frames_to_process << "\n255\n";

    int errors = 0;

    for (int f = 0; f < frames_to_process; f++)
    {
        for (int i = 0; i < HEIGHT; i++)
        {
            for (int j = 0; j < WIDTH; j++)
            {
                gamma_out_t pixel_out = dst.read();

                ap_uint<8> act_r = pixel_out.data.range(7, 0);
                ap_uint<8> act_g = pixel_out.data.range(15, 8);
                ap_uint<8> act_b = pixel_out.data.range(23, 16);

                out_img << (int)act_r << " " << (int)act_g << " " << (int)act_b << " ";

                int g_r, g_g, g_b;
                golden_img >> g_r >> g_g >> g_b;

                if (g_r != (int)act_r || g_g != (int)act_g || g_b != (int)act_b)
                {
                    if (errors < 10)
                    {
                        std::cerr << "Mismatch at frame " << f << " pixel (" << i << "," << j << "): "
                                  << "Expected (" << g_r << "," << g_g << "," << g_b << ") "
                                  << "Got (" << (int)act_r << "," << (int)act_g << "," << (int)act_b << ")" << std::endl;
                    }
                    errors++;
                }
            }
            out_img << "\n";
        }
    }
    out_img.close();
    golden_img.close();
    
    std::cout << "Saved frames to " << out_path << std::endl;

    std::cout << "=========================================" << std::endl;
    if (errors == 0)
    {
        std::cout << "Test Passed Successfully!" << std::endl;
    }
    else
    {
        std::cout << "Test Failed with " << errors << " errors!" << std::endl;
        return 1;
    }
    std::cout << "=========================================" << std::endl;
    return 0;
}
#endif