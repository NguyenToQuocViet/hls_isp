#include "blc.hpp"
#include "bpc_adaptive.hpp"
#include "golden_wb_model.h"
#include "golden_demosaic_fixed.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

static std::vector<std::uint16_t> read_u16(const char* path, size_t n) {
    std::vector<std::uint16_t> v(n);
    std::ifstream f(path, std::ios::binary);
    f.read(reinterpret_cast<char*>(v.data()), n * 2);
    if (!f) { std::fprintf(stderr, "short read %s\n", path); std::exit(2); }
    return v;
}

static void write_u16(const char* path, const std::vector<std::uint16_t>& v) {
    std::ofstream f(path, std::ios::binary);
    f.write(reinterpret_cast<const char*>(v.data()), v.size() * 2);
}

int main(int argc, char** argv) {
    std::string mode = argv[1];
    const char* in = argv[2];
    const char* out = argv[3];
    int w = std::atoi(argv[4]), h = std::atoi(argv[5]);
    auto raw = read_u16(in, size_t(w) * h);
    std::vector<std::uint16_t> res;

    if (mode == "blc") {
        blc::BlcConfig c{(std::uint16_t)std::atoi(argv[6]), (std::uint16_t)std::atoi(argv[7]),
                         (std::uint16_t)std::atoi(argv[8]), (std::uint16_t)std::atoi(argv[9])};
        blc::blc_frame(raw, res, w, h, c);
    } else if (mode == "bpc") {
        adaptive_bpc::BpcConfig c{(std::uint16_t)std::atoi(argv[6]), (std::uint16_t)std::atoi(argv[7]),
                                  (std::uint16_t)std::atoi(argv[8]), (std::uint8_t)std::atoi(argv[9]),
                                  (std::uint8_t)std::atoi(argv[10])};
        std::vector<adaptive_bpc::Detection> det;
        adaptive_bpc::bpc_frame(raw, res, det, c);
        std::printf("bpc detections=%zu\n", det.size());
    } else if (mode == "wb") {
        using namespace golden_wb_model;
        FixedGainsQ12 g{quantize_gain_q4_12(std::atof(argv[6])), quantize_gain_q4_12(std::atof(argv[7])),
                        quantize_gain_q4_12(std::atof(argv[8])), quantize_gain_q4_12(std::atof(argv[9]))};
        res = wb_fixed(raw, w, h, g);
    } else if (mode == "demosaic") {
        auto rgb = golden_demosaic::frame(raw, w, h, std::atoi(argv[6]), std::atoi(argv[7]));
        for (auto& p : rgb) { res.push_back(p.r); res.push_back(p.g); res.push_back(p.b); }
    } else {
        std::fprintf(stderr, "bad mode\n");
        return 1;
    }
    write_u16(out, res);
    return 0;
}
