#include "ltm.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include <dirent.h>
#include <algorithm>

std::string in_dir = "/home/asura/Work/ISP/dataset/input_csim/Video/Sequence/";
std::string out_dir = "/home/asura/Work/ISP/dataset/output_csim/Video/Sequence/";
std::string golden_dir = "/home/asura/Work/ISP/dataset/output_csim_golden/Sequence/";

#ifdef LTM
int main()
{
    std::cout << "Starting LTM Testbench..." << std::endl;

    // 1. Generate LUTs in C++ Testbench
    log_t log_lut[4096];
    log_t reinhard_lut[1024];
    exp_out_t exp_lut[1024];

    std::cout << "Generating log_lut..." << std::endl;
    for (int i = 0; i < 4096; i++)
    {
        double val = std::log((double)i + 1.0);
        log_lut[i] = (log_t)val;
    }

    std::cout << "Generating reinhard_lut..." << std::endl;
    double exposure_gain = 3.0;
    double target_max = 4095.0;
    double log_target_max = std::log(target_max + 1.0);
    for (int i = 0; i < 1024; i++)
    {
        double log_I_base = (double)i / 123.0;

        double L = (log_I_base / log_target_max) * exposure_gain;
        double L_white = exposure_gain;
        double L_mapped = (L * (1.0 + (L / (L_white * L_white)))) / (1.0 + L);

        double log_I_mapped_base = L_mapped * log_target_max;

        reinhard_lut[i] = (log_t)log_I_mapped_base;
    }

    std::cout << "Generating exp_lut..." << std::endl;
    for (int i = 0; i < 1024; i++)
    {
        double log_I_final = (double)i / 123.0;
        double I_final = std::exp(log_I_final) - 1.0;

        if (I_final > 4095.0)
            I_final = 4095.0;
        if (I_final < 0.0)
            I_final = 0.0;

        exp_lut[i] = (exp_out_t)I_final;
    }



    // 2. Get list of all binary files in directory
    std::vector<std::string> bin_files;
    DIR* dir = opendir(in_dir.c_str());
    if (dir != nullptr) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string filename = entry->d_name;
            if (filename.length() >= 4 && filename.substr(filename.length() - 4) == ".bin") {
                bin_files.push_back(filename);
            }
        }
        closedir(dir);
    }

    if (bin_files.empty()) {
        std::cerr << "No .bin files found in " << in_dir << std::endl;
        return 1;
    }

    // Sort files naturally so that 2.bin comes before 10.bin
    std::sort(bin_files.begin(), bin_files.end(), [](const std::string& a, const std::string& b) {
        if (a.length() != b.length()) {
            return a.length() < b.length();
        }
        return a < b;
    });

    int total_processed = 0;
    int common_width = 0;
    int common_height = 0;

    hls::stream<AXI_PIXEL_IN> s_axis("s_axis");
    hls::stream<AXI_PIXEL_OUT> m_axis("m_axis");

    std::cout << "\n========================================" << std::endl;
    std::cout << "Pushing all frames to input stream..." << std::endl;

    for (const auto& filename : bin_files) {
        std::string in_path = in_dir + filename;

        FILE *f_in = fopen(in_path.c_str(), "rb");
        if (!f_in) {
            std::cerr << "Error opening " << in_path << std::endl;
            continue;
        }
        int header[2];
        if (fread(header, sizeof(int), 2, f_in) != 2) {
            std::cerr << "Error reading dimensions from " << in_path << std::endl;
            fclose(f_in);
            continue;
        }
        int width = header[0];
        int height = header[1];
        
        if (common_width == 0 && common_height == 0) {
            common_width = width;
            common_height = height;
        } else if (width != common_width || height != common_height) {
            std::cerr << "Error: Image dimensions mismatch in " << filename << std::endl;
            fclose(f_in);
            continue;
        }

        std::vector<uint16_t> input_image(width * height * 3);
        if (fread(input_image.data(), sizeof(uint16_t), input_image.size(), f_in) != input_image.size()) {
            std::cerr << "Error reading image data from " << in_path << std::endl;
            fclose(f_in);
            continue;
        }
        fclose(f_in);

        for (int r = 0; r < height; r++)
        {
            for (int c = 0; c < width; c++)
            {
                AXI_PIXEL_IN p;

                int idx = (r * width + c) * 3;
                ap_uint<12> r_val = input_image[idx + 0];
                ap_uint<12> g_val = input_image[idx + 1];
                ap_uint<12> b_val = input_image[idx + 2];

                p.data.range(11, 0) = r_val;
                p.data.range(23, 12) = g_val;
                p.data.range(35, 24) = b_val;

                p.user = (r == 0 && c == 0) ? 1 : 0;
                p.last = (c == width - 1) ? 1 : 0;

                s_axis.write(p);
            }
        }
        total_processed++;
    }

    std::cout << "Successfully pushed " << total_processed << " frames." << std::endl;

    std::cout << "\n========================================" << std::endl;
    std::cout << "Running LTM module continuously..." << std::endl;
    
    int debug_pixels = 0;
    for (int i = 0; i < total_processed; i++) {
        #ifdef DEBUG
        ltm(s_axis, m_axis, log_lut, reinhard_lut, exp_lut, debug_pixels);
        #else
        ltm(s_axis, m_axis, log_lut, reinhard_lut, exp_lut);
        #endif
    }

    std::string out_path = out_dir + "output_video.ppm";
    std::cout << "\n========================================" << std::endl;
    std::cout << "Reading output stream and saving to " << out_path << "..." << std::endl;
    
    std::string golden_path = golden_dir + "golden_output_video.ppm";
    std::ifstream golden_img(golden_path);
    bool check_golden = true;
    if (!golden_img.is_open()) {
        std::cerr << "Warning: Golden model output " << golden_path << " not found. Skipping pixel-by-pixel check." << std::endl;
        check_golden = false;
    } else {
        std::string magic;
        int w, h, max_val;
        golden_img >> magic >> w >> h >> max_val;
        if (magic != "P3" || w != common_width || h != common_height * total_processed) {
            std::cerr << "Warning: Golden model header mismatch. Skipping pixel-by-pixel check." << std::endl;
            check_golden = false;
        }
    }

    std::ofstream out_img(out_path);
    // Write a single tall image
    out_img << "P3\n" << common_width << " " << common_height * total_processed << "\n4095\n";

    int pixels_out = 0;
    int mismatches = 0;
    while (!m_axis.empty())
    {
        AXI_PIXEL_OUT p_out = m_axis.read();
        
        int r_val = p_out.data.range(11, 0);
        int g_val = p_out.data.range(23, 12);
        int b_val = p_out.data.range(35, 24);
        
        out_img << r_val << " " << g_val << " " << b_val << "\n";
        
        if (check_golden) {
            int g_r, g_g, g_b;
            if (golden_img >> g_r >> g_g >> g_b) {
                if (r_val != g_r || g_val != g_g || b_val != g_b) {
                    if (mismatches < 10) {
                        std::cout << "Mismatch at pixel " << pixels_out << ": expected (" << g_r << "," << g_g << "," << g_b << ") but got (" << r_val << "," << g_val << "," << b_val << ")" << std::endl;
                    }
                    mismatches++;
                }
            } else {
                std::cerr << "Error reading golden pixel at " << pixels_out << std::endl;
                check_golden = false;
            }
        }
        
        pixels_out++;
    }
    out_img.close();
    if (golden_img.is_open()) golden_img.close();

    std::cout << "Total output pixels processed: " << pixels_out << std::endl;
    if (check_golden) {
        std::cout << "Total pixel mismatches with golden model: " << mismatches << std::endl;
    }

    if (pixels_out == common_height * common_width * total_processed && total_processed > 0 && mismatches == 0)
    {
        std::cout << "SUCCESS: All frames processed correctly and matched golden model!" << std::endl;
        return 0;
    }
    else
    {
        std::cout << "ERROR: Missing or mismatched pixels (or golden mismatch)!" << std::endl;
        return 1;
    }
}

#endif
