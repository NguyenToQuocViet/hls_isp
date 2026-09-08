/*
Project: Adaptive Directional BPC and BLC
Module: One Image BPC Evaluator
Description: Measure paired post-BLC frames and build exact per-phase threshold metric tables.
Author: Viet Nguyen To Quoc
*/

#include "bpc_adaptive.hpp"
#include "bpc_baseline.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr int WIDTH = 1920;
constexpr int HEIGHT = 1080;
constexpr int LEVELS = 4096;
// E_residual_hot, E_residual_dead, background changes, injection-induced changes.
using Stats = std::array<std::int64_t, 4>;
using Frame = std::vector<std::uint16_t>;

Frame read_pgm(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    std::string magic;
    int width = 0, height = 0, maximum = 0;
    file >> magic >> width >> height >> maximum;
    if (!file || magic != "P5" || width != WIDTH || height != HEIGHT || maximum != 4095 || file.get() != '\n') {
        throw std::runtime_error("Expected injector P5 RAW12 frame: " + path);
    }
    Frame frame(WIDTH * HEIGHT);
    for (auto& value : frame) {
        const int high = file.get(), low = file.get();
        if (high < 0 || low < 0 || high > 15) throw std::runtime_error("Invalid RAW12 PGM payload");
        value = static_cast<std::uint16_t>((high << 8) | low);
    }
    if (file.peek() != EOF) throw std::runtime_error("Unexpected trailing PGM data");
    return frame;
}

struct Dataset {
    Frame reference, corrupted;
    std::vector<int> labels;
    std::array<std::int64_t, 2> initial{}, counts{};
    std::int64_t valid_count = (WIDTH - 4) * (HEIGHT - 4);
    std::int64_t unmodified_count = valid_count;

    explicit Dataset(const std::string& directory)
        : reference(read_pgm(directory + "/reference_blc.pgm")), corrupted(read_pgm(directory + "/corrupted_blc.pgm")), labels(WIDTH * HEIGHT, -1) {
        std::ifstream csv(directory + "/defects.csv");
        std::string line;
        if (!std::getline(csv, line) || line != "x,y,cfa,type,input_value,output_value,a,b") throw std::runtime_error("Invalid defects.csv");
        while (std::getline(csv, line)) {
            std::istringstream row(line);
            std::array<std::string, 8> fields;
            for (auto& field : fields) if (!std::getline(row, field, ',')) throw std::runtime_error("Incomplete defect row");
            const int x = std::stoi(fields[0]), y = std::stoi(fields[1]);
            const int type = fields[3] == "hot" ? 0 : fields[3] == "dead" ? 1 : -1;
            if (x < 2 || x >= WIDTH - 2 || y < 2 || y >= HEIGHT - 2 || type < 0) throw std::runtime_error("Invalid hot/dead coordinate");
            const int index = y * WIDTH + x;
            if (labels[index] >= 0) throw std::runtime_error("Duplicate defect coordinate");
            labels[index] = type;
            initial[type] += std::abs(int(corrupted[index]) - int(reference[index]));
            counts[type]++;
            unmodified_count--;
        }
        if (!initial[0] || !initial[1]) throw std::runtime_error("Zero initial hot/dead error; regenerate injection");
        for (int i = 0; i < WIDTH * HEIGHT; i++) {
            if (labels[i] < 0 && reference[i] != corrupted[i]) throw std::runtime_error("Pair differs outside injection labels");
        }
    }
};

Stats measure(const Dataset& data, const Frame& output_r, const Frame& output_c) {
    Stats sums{};
    for (int row = 2; row < HEIGHT - 2; row++) {
        for (int col = 2; col < WIDTH - 2; col++) {
            const int i = row * WIDTH + col;
            sums[2] += output_r[i] != data.reference[i];
            if (data.labels[i] >= 0) sums[data.labels[i]] += std::abs(int(output_c[i]) - int(data.reference[i]));
            else sums[3] += output_c[i] != output_r[i];
        }
    }
    return sums;
}

Stats oracle(const Dataset& data, const adaptive_bpc::BpcConfig& config) {
    Frame output_r, output_c;
    std::vector<adaptive_bpc::Detection> detections;
    adaptive_bpc::bpc_frame(data.reference, output_r, detections, config);
    adaptive_bpc::bpc_frame(data.corrupted, output_c, detections, config);
    return measure(data, output_r, output_c);
}

Stats baseline(const Dataset& data, int threshold) {
    const auto value = static_cast<std::uint16_t>(threshold);
    Frame output_r, output_c;
    std::vector<Detection> detections;
    bpc_frame(data.reference, output_r, detections, {value, value, value});
    bpc_frame(data.corrupted, output_c, detections, {value, value, value});
    return measure(data, output_r, output_c);
}

struct Feature {
    int center, prediction, activity, margin;
    int cutoff(int signal_shift, int activity_shift) const {
        return std::clamp(margin - (prediction >> signal_shift) - (activity >> activity_shift), 0, LEVELS);
    }
};

Feature feature(const Frame& frame, int i, bool median_mode = false) {
    const std::array<int, 8> neighbors = {frame[i - 2], frame[i + 2], frame[i - 2 * WIDTH], frame[i + 2 * WIDTH], frame[i - 2 * WIDTH - 2], frame[i + 2 * WIDTH + 2], frame[i - 2 * WIDTH + 2], frame[i + 2 * WIDTH - 2]};
    int selected = 0;
    for (int pair = 2; pair < 8; pair += 2) {
        if (std::abs(neighbors[pair] - neighbors[pair + 1]) < std::abs(neighbors[selected] - neighbors[selected + 1])) selected = pair;
    }
    const int center = frame[i];
    if (median_mode) {
        auto sorted = neighbors;
        std::sort(sorted.begin(), sorted.end());
        const int prediction = (sorted[3] + sorted[4]) / 2;
        return {center, prediction, 0, std::abs(center - prediction)};
    }
    const int prediction = (neighbors[selected] + neighbors[selected + 1]) / 2;
    const int activity = std::abs(neighbors[selected] - neighbors[selected + 1]);
    return {center, prediction, activity, std::abs(center - prediction)};
}

struct PairedFeature {
    Feature reference, corrupted;
    int phase, label;
};

std::vector<PairedFeature> prepare(const Dataset& data, bool median_mode = false) {
    std::vector<PairedFeature> result;
    result.reserve(data.valid_count);
    for (int row = 2; row < HEIGHT - 2; row++) {
        for (int col = 2; col < WIDTH - 2; col++) {
            const int i = row * WIDTH + col;
            result.push_back({feature(data.reference, i, median_mode), feature(data.corrupted, i, median_mode), (row % 2) * 2 + col % 2, data.labels[i]});
        }
    }
    return result;
}

std::vector<Stats> make_table(const std::vector<PairedFeature>& pixels, int signal_shift, int activity_shift, bool median_mode = false) {
    std::vector<Stats> delta(4 * (LEVELS + 1));
    for (const auto& pixel : pixels) {
        Stats* phase = delta.data() + pixel.phase * (LEVELS + 1);
        const auto add = [phase](int metric, int begin, int end, std::int64_t value) {
            phase[begin][metric] += value;
            phase[end][metric] -= value;
        };
        const auto& r = pixel.reference;
        const auto& c = pixel.corrupted;
        const int cut_r = median_mode ? r.margin : r.cutoff(signal_shift, activity_shift);
        const int cut_c = median_mode ? c.margin : c.cutoff(signal_shift, activity_shift);
        add(2, 0, cut_r, 1);
        if (pixel.label >= 0) {
            add(pixel.label, 0, cut_c, std::abs(c.prediction - r.center));
            add(pixel.label, cut_c, LEVELS, std::abs(c.center - r.center));
        } else {
            add(3, 0, std::min(cut_r, cut_c), r.prediction != c.prediction);
            if (cut_r < cut_c) add(3, cut_r, cut_c, r.center != c.prediction);
            else add(3, cut_c, cut_r, r.prediction != c.center);
        }
    }
    std::vector<Stats> table(4 * LEVELS);
    for (int phase = 0; phase < 4; phase++) {
        Stats sum{};
        for (int threshold = 0; threshold < LEVELS; threshold++) {
            for (int metric = 0; metric < 4; metric++) sum[metric] += delta[phase * (LEVELS + 1) + threshold][metric];
            table[phase * LEVELS + threshold] = sum;
        }
    }
    return table;
}

void print_stats(const Stats& values) {
    std::cout << '[' << values[0] << ',' << values[1] << ',' << values[2] << ',' << values[3] << ']';
}

int integer(const char* text, int maximum) {
    std::size_t used = 0;
    const int value = std::stoi(text, &used);
    if (text[used] || value < 0 || value > maximum) throw std::runtime_error("Argument out of range");
    return value;
}

void print_metadata(const Dataset& data) {
    std::cout << "{\"initial\":[" << data.initial[0] << ',' << data.initial[1]
              << "],\"valid_count\":" << data.valid_count
              << ",\"unmodified_count\":" << data.unmodified_count;
}
} // namespace

int main(int argc, char** argv) {
    try {
        if (argc < 4) throw std::runtime_error("Usage: bpc_evaluate INPUT_DIR --bank BANK_FILE BASELINE_T | --oracle R G B ks ka | --baseline-bank FILE | --fixed BASELINE_T R G B ks ka");
        const Dataset data(argv[1]);
        const std::string mode(argv[2]);
        if (mode == "--oracle" && argc == 8) {
            const adaptive_bpc::BpcConfig config{static_cast<std::uint16_t>(integer(argv[3], 4095)), static_cast<std::uint16_t>(integer(argv[4], 4095)), static_cast<std::uint16_t>(integer(argv[5], 4095)), static_cast<std::uint8_t>(integer(argv[6], 12)), static_cast<std::uint8_t>(integer(argv[7], 12))};
            print_stats(oracle(data, config));
            std::cout << '\n';
        } else if (mode == "--baseline-bank" && argc == 4) {
            const auto table = make_table(prepare(data, true), 0, 0, true);
            std::vector<Stats> combined(LEVELS);
            for (int t = 0; t < LEVELS; t++) {
                for (int phase = 0; phase < 4; phase++) {
                    for (int m = 0; m < 4; m++) combined[t][m] += table[phase * LEVELS + t][m];
                }
            }
            // Check the accelerated baseline against the unchanged reference implementation.
            for (int t : {0, 1, 1063, 2048, 4095}) {
                if (combined[t] != baseline(data, t)) throw std::runtime_error("Baseline bank differs from reference");
            }
            std::ofstream file(argv[3], std::ios::binary);
            file.write(reinterpret_cast<const char*>(combined.data()), combined.size() * sizeof(Stats));
            file.close();
            if (!file) throw std::runtime_error("Cannot write baseline bank");
            print_metadata(data);
            std::cout << "}\n";
        } else if (mode == "--fixed" && argc == 9) {
            const int threshold = integer(argv[3], 4095);
            const adaptive_bpc::BpcConfig config{static_cast<std::uint16_t>(integer(argv[4], 4095)), static_cast<std::uint16_t>(integer(argv[5], 4095)), static_cast<std::uint16_t>(integer(argv[6], 4095)), static_cast<std::uint8_t>(integer(argv[7], 12)), static_cast<std::uint8_t>(integer(argv[8], 12))};
            print_metadata(data);
            std::cout << ",\"baseline\":";
            print_stats(baseline(data, threshold));
            std::cout << ",\"adaptive\":";
            print_stats(oracle(data, config));
            std::cout << "}\n";
        } else if (mode == "--bank" && argc == 5) {
            const int threshold = integer(argv[4], 4095);
            const auto started = std::chrono::steady_clock::now();
            const auto pixels = prepare(data);
            std::ofstream file(argv[3], std::ios::binary);
            if (!file) throw std::runtime_error("Cannot create metric bank");
            for (int signal = 0; signal <= 12; signal++) {
                for (int activity = 0; activity <= 12; activity++) {
                    const auto table = make_table(pixels, signal, activity);
                    file.write(reinterpret_cast<const char*>(table.data()), table.size() * sizeof(Stats));
                }
                std::cerr << "metric bank k_s=" << signal << "/12\n";
            }
            file.close();
            if (!file) throw std::runtime_error("Failed writing metric bank");
            std::cout << "{\"initial\":[" << data.initial[0] << ',' << data.initial[1] << "],\"counts\":[" << data.counts[0] << ',' << data.counts[1] << "],\"valid_count\":" << data.valid_count << ",\"unmodified_count\":" << data.unmodified_count << ",\"baseline_threshold\":" << threshold << ",\"baseline\":";
            print_stats(baseline(data, threshold));
            const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
            std::cout << ",\"bank_seconds\":" << seconds << "}\n";
        } else throw std::runtime_error("Invalid evaluator arguments");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
