#include "../ltm_gamma.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <dirent.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>

std::string in_dir_top = "/home/asura/Work/ISP/hls_isp/Anh/dataset/input_csim/Sequence/";
std::string out_dir_top = "/home/asura/Work/ISP/hls_isp/Anh/dataset/output_csim/LTM_Gamma/Sequence/";

#ifdef LTM_GAMMA
int main()
{
    std::cout << "Starting ISP Top Testbench..." << std::endl;

    // Generate LUTs for LTM
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

    // Generate LUTs for Gamma
    ap_uint<8> gamma_lut[4096];
    std::cout << "Generating gamma_lut..." << std::endl;
    double gamma_val = 0.45;
    for (int i = 0; i < 4096; i++)
    {
        double normalized = (double)i / 4095.0;
        double corrected = std::pow(normalized, gamma_val);
        double val = corrected * 255.0;
        if (val > 255.0) val = 255.0;
        if (val < 0.0) val = 0.0;
        gamma_lut[i] = (ap_uint<8>)(std::round(val));
    }

    // Get list of all binary files in directory
    std::vector<std::string> bin_files;
    DIR* dir = opendir(in_dir_top.c_str());
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
        std::cerr << "No .bin files found in " << in_dir_top << std::endl;
        return 1;
    }

    // Sort files naturally
    std::sort(bin_files.begin(), bin_files.end(), [](const std::string& a, const std::string& b) {
        if (a.length() != b.length()) {
            return a.length() < b.length();
        }
        return a < b;
    });

    int total_processed = 0;
    int common_width = 0;
    int common_height = 0;

    hls::stream<axis_pixel_36b> s_axis("s_axis");
    hls::stream<axis_pixel_24b> m_axis("m_axis");

    std::cout << "\n========================================" << std::endl;
    std::cout << "Pushing all frames to input stream..." << std::endl;

    for (const auto& filename : bin_files) {
        std::string in_path = in_dir_top + filename;

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
                axis_pixel_36b p;

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
    std::cout << "Running ISP Top module continuously..." << std::endl;
    
    for (int i = 0; i < total_processed; i++) {
        isp_ltm_gamma_top(s_axis, m_axis, log_lut, reinhard_lut, exp_lut, gamma_lut, gamma_lut, gamma_lut);
    }

    std::string out_path = out_dir_top + "output_video.ppm";
    std::cout << "\n========================================" << std::endl;
    std::cout << "Reading output stream and saving to " << out_path << "..." << std::endl;
    
    // Create output directory if it doesn't exist
    std::string mkdir_cmd = "mkdir -p " + out_dir_top;
    int ret = system(mkdir_cmd.c_str());
    (void)ret; // Ignore return value

    std::ofstream out_img(out_path);
    if (!out_img.is_open()) {
        std::cerr << "Error opening " << out_path << " for writing." << std::endl;
        return 1;
    }
    
    // Write a single tall image
    out_img << "P3\n" << common_width << " " << common_height * total_processed << "\n255\n";

    int pixels_out = 0;
    while (!m_axis.empty())
    {
        axis_pixel_24b p_out = m_axis.read();
        
        int r_val = p_out.data.range(7, 0);
        int g_val = p_out.data.range(15, 8);
        int b_val = p_out.data.range(23, 16);
        
        out_img << r_val << " " << g_val << " " << b_val << "\n";
        
        pixels_out++;
    }
    out_img.close();

    std::cout << "Total output pixels processed: " << pixels_out << std::endl;

    if (pixels_out == common_height * common_width * total_processed && total_processed > 0)
    {
        std::cout << "SUCCESS: All frames processed correctly!" << std::endl;
        return 0;
    }
    else
    {
        std::cout << "ERROR: Missing pixels in the output!" << std::endl;
        return 1;
    }
}

#endif
