/*
Project: Adaptive Directional BPC and BLC
Module: Defect Injector
Description: Load, validate, normalize, and inject defects into RAW Bayer data.
Author: Viet Nguyen To Quoc
*/

#include "blc.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <vector>

#include <libraw/libraw.h>

//CONFIGURATION
constexpr std::size_t OUTPUT_WIDTH = 1920;
constexpr std::size_t OUTPUT_HEIGHT = 1080;
constexpr std::size_t HOT_DEFECT_COUNT = 150;
constexpr std::size_t DEAD_DEFECT_COUNT = 150;
constexpr std::size_t TOTAL_DEFECT_COUNT = HOT_DEFECT_COUNT + DEAD_DEFECT_COUNT;
constexpr std::size_t BORDER_WIDTH = 2;
constexpr std::size_t MIN_DEFECT_SPACING = 10;
constexpr std::size_t MAX_PLACEMENT_ATTEMPTS = 1000000;
constexpr unsigned int RAW12_MAX = 4095;
constexpr unsigned int WHITE_LEVEL_MAX = 65535;
constexpr unsigned int RNG_SEED = 0;
constexpr int HOT_OFFSET_MIN = 256;
constexpr int HOT_OFFSET_MAX = 2048;
constexpr double DEAD_GAIN_MIN = 0.0;
constexpr double DEAD_GAIN_MAX = 0.75;
constexpr const char* CLEAN_OUTPUT_PATH = "clean_rggb.pgm";
constexpr const char* CORRUPTED_OUTPUT_PATH = "corrupted_rggb.pgm";
constexpr const char* DEFECT_OUTPUT_PATH = "defects.csv";

enum class DefectType {
    HOT,
    DEAD
};

struct Coordinate {
    std::size_t x;
    std::size_t y;
};

struct Defect {
    std::size_t x;
    std::size_t y;
    DefectType type;
    unsigned short input_value;
    unsigned short output_value;
    double a;
    double b;
};

const char* get_cfa_name(std::size_t row, std::size_t col) {
    if (row % 2 == 0 && col % 2 == 0) {
        return "R";
    }

    if (row % 2 == 0) {
        return "Gr";
    }

    if (col % 2 == 0) {
        return "Gb";
    }

    return "B";
}

const char* get_defect_type_name(DefectType type) {
    if (type == DefectType::HOT) {
        return "hot";
    }

    if (type == DefectType::DEAD) {
        return "dead";
    }

    return "dead";
}

bool write_pgm(const char* path, const std::vector<unsigned short>& image) {
    std::ofstream output(path, std::ios::binary);

    if (!output) {
        return false;
    }

    output << "P5\n" << OUTPUT_WIDTH << ' ' << OUTPUT_HEIGHT << "\n" << RAW12_MAX << "\n";

    std::vector<unsigned char> encoded_image(image.size() * 2);

    for (std::size_t index = 0; index < image.size(); index++) {
        encoded_image[index * 2] = static_cast<unsigned char>(image[index] >> 8);
        encoded_image[index * 2 + 1] = static_cast<unsigned char>(image[index] & 0xff);
    }

    output.write(reinterpret_cast<const char*>(encoded_image.data()), static_cast<std::streamsize>(encoded_image.size()));
    output.close();

    return output.good();
}

bool write_csv(const char* path, const std::vector<Defect>& defects) {
    std::ofstream output(path);

    if (!output) {
        return false;
    }

    output << "x,y,cfa,type,input_value,output_value,a,b\n";
    output << std::setprecision(std::numeric_limits<double>::max_digits10);

    for (const Defect& defect : defects) {
        output << defect.x << ',' << defect.y << ',' << get_cfa_name(defect.y, defect.x) << ',' << get_defect_type_name(defect.type) << ',' << defect.input_value << ',' << defect.output_value << ',' << defect.a << ',' << defect.b << '\n';
    }

    output.close();

    return output.good();
}

bool inject_defects(const std::vector<unsigned short>& clean_rggb, std::vector<unsigned short>& corrupted_rggb, std::vector<Defect>& defects, unsigned int seed) {
    //PLACE DEFECTS
    std::mt19937 rng(seed);
    std::uniform_int_distribution<std::size_t> x_distribution(BORDER_WIDTH, OUTPUT_WIDTH - BORDER_WIDTH - 1);
    std::uniform_int_distribution<std::size_t> y_distribution(BORDER_WIDTH, OUTPUT_HEIGHT - BORDER_WIDTH - 1);
    std::vector<Coordinate> coordinates;
    coordinates.reserve(TOTAL_DEFECT_COUNT);

    std::size_t placement_attempts = 0;

    while (coordinates.size() < TOTAL_DEFECT_COUNT && placement_attempts < MAX_PLACEMENT_ATTEMPTS) {
        placement_attempts++;

        const Coordinate candidate = {x_distribution(rng), y_distribution(rng)};
        bool valid_position = true;

        for (const Coordinate& coordinate : coordinates) {
            const std::size_t x_distance = candidate.x > coordinate.x ? candidate.x - coordinate.x : coordinate.x - candidate.x;
            const std::size_t y_distance = candidate.y > coordinate.y ? candidate.y - coordinate.y : coordinate.y - candidate.y;

            if (std::max(x_distance, y_distance) < MIN_DEFECT_SPACING) {
                valid_position = false;

                break;
            }
        }

        if (valid_position) {
            coordinates.push_back(candidate);
        }
    }

    if (coordinates.size() != TOTAL_DEFECT_COUNT) {
        std::cerr << "Cannot place all defects with the required spacing\n";

        return false;
    }

    //INJECT DEFECTS
    std::uniform_int_distribution<int> hot_offset_distribution(HOT_OFFSET_MIN, HOT_OFFSET_MAX);
    std::uniform_real_distribution<double> dead_gain_distribution(DEAD_GAIN_MIN, DEAD_GAIN_MAX);
    corrupted_rggb = clean_rggb;
    defects.clear();
    defects.reserve(TOTAL_DEFECT_COUNT);

    for (std::size_t index = 0; index < coordinates.size(); index++) {
        const Coordinate& coordinate = coordinates[index];
        const std::size_t pixel_index = coordinate.y * OUTPUT_WIDTH + coordinate.x;
        Defect defect = {coordinate.x, coordinate.y, DefectType::DEAD, clean_rggb[pixel_index], 0, 0.0, 0.0};

        if (index < HOT_DEFECT_COUNT) {
            defect.type = DefectType::HOT;
            defect.a = 1.0;
            defect.b = static_cast<double>(hot_offset_distribution(rng));
        } else {
            defect.type = DefectType::DEAD;
            defect.a = dead_gain_distribution(rng);
        }

        const long rounded_value = std::lround(defect.a * defect.input_value + defect.b);
        const long clipped_value = std::clamp(rounded_value, 0L, static_cast<long>(RAW12_MAX));

        defect.output_value = static_cast<unsigned short>(clipped_value);
        corrupted_rggb[pixel_index] = defect.output_value;
        defects.push_back(defect);
    }

    return true;
}

// LibRaw averages DeltaH/V into a scalar; reject their presence rather than
// silently replacing a spatial black-level model with that average.
struct InputTags {
    bool black_delta = false;
    bool black_level = false;
};

void inspect_tag(void* context, int tag, int, int, unsigned int, void*, INT64) {
    InputTags& tags = *static_cast<InputTags*>(context);
    const int id = tag & 0xffff;
    tags.black_level = tags.black_level || id == 50714;
    tags.black_delta = tags.black_delta || id == 50715 || id == 50716;
}

bool black_levels_for_crop(const libraw_dng_levels_t& levels, std::size_t row_offset, std::size_t col_offset, unsigned int white_level, std::array<double, 4>& native, blc::BlcConfig& config) {
    const unsigned int rows = levels.dng_cblack[4];
    const unsigned int cols = levels.dng_cblack[5];
    if (rows > 2 || cols > 2 || (rows == 0) != (cols == 0)) {
        std::cerr << "Unsupported or missing BlackLevel; expected scalar or at most 2x2 pattern\n";
        return false;
    }
    for (int channel = 0; channel < 4; channel++) {
        if (levels.dng_fcblack[channel] != 0.0f) {
            std::cerr << "Unsupported per-channel BlackLevel on Bayer input\n";
            return false;
        }
    }
    std::array<std::uint16_t, 4> scaled{};
    for (std::size_t phase = 0; phase < 4; phase++) {
        native[phase] = levels.dng_fblack;
        if (rows && cols) {
            const std::size_t index = ((row_offset + phase / 2) % rows) * cols + (col_offset + phase % 2) % cols;
            native[phase] += levels.dng_fcblack[6 + index];
        }
        if (!std::isfinite(native[phase]) || native[phase] < 0 || native[phase] >= white_level) {
            std::cerr << "BlackLevel must be finite and in [0, WhiteLevel)\n";
            return false;
        }
        scaled[phase] = static_cast<std::uint16_t>(std::floor(native[phase] * RAW12_MAX / white_level + 0.5));
    }
    config = {scaled[0], scaled[1], scaled[2], scaled[3]};
    return true;
}

int main(int argc, char* argv[]) {
    //CHECK INPUT ARGUMENTS
    bool check_only = false;
    const char* input_path = nullptr;
    unsigned int seed = RNG_SEED;

    if (argc == 2) {
        input_path = argv[1];
    } else if (argc == 3 && std::string(argv[1]) == "--check-only") {
        check_only = true;
        input_path = argv[2];
    } else if (argc == 4 && std::string(argv[1]) == "--seed") {
        const std::string value = argv[2];
        const auto parsed = std::from_chars(value.data(), value.data() + value.size(), seed);
        if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size()) {
            std::cerr << "Seed must be an unsigned integer\n";
            return EXIT_FAILURE;
        }
        input_path = argv[3];
    } else {
        std::cerr << "Usage: " << argv[0] << " [--check-only | --seed N] <image.dng>\n";

        return EXIT_FAILURE;
    }

    //READ RAW IMAGE

    //initiate libraw obj
    LibRaw raw;
    InputTags tags;
    raw.set_exifparser_handler(inspect_tag, &tags);

    //open raw img
    const int open_result = raw.open_file(input_path);

    if (open_result != LIBRAW_SUCCESS) {
        std::cerr << "Cannot open DNG: " << libraw_strerror(open_result) << "\n";

        return EXIT_FAILURE;
    }

    //unpack raw img
    const int unpack_result = raw.unpack();

    if (unpack_result != LIBRAW_SUCCESS) {
        std::cerr << "Cannot unpack DNG: " << libraw_strerror(unpack_result) << "\n";

        return EXIT_FAILURE;
    }

    //get in4mation and reference
    const unsigned short* raw_image = raw.imgdata.rawdata.raw_image;

    const libraw_image_sizes_t& sizes = raw.imgdata.sizes;
    const libraw_iparams_t& idata = raw.imgdata.idata;
    const libraw_colordata_t& color = raw.imgdata.color;
    const libraw_dng_levels_t& dng_levels = color.dng_levels;

    //VALIDATE IMAGE

    //validate raw plane
    if (raw_image == nullptr) {
        std::cerr << "LibRaw did not provide a supported single-channel integer RAW plane\n";

        return EXIT_FAILURE;
    }

    //check dng format
    if (idata.dng_version == 0) {
        std::cerr << "Input is not a DNG file\n";

        return EXIT_FAILURE;
    }

    //check cfa presence
    if (idata.filters == 0) {
        std::cerr << "Image doesn't contain CFA mosaic\n";

        return EXIT_FAILURE;
    }

    //check bayer layout
    if (idata.filters < 1000) {
        std::cerr << "Unsupported CFA layout; expected a standard 2 x 2 Bayer mosaic\n";

        return EXIT_FAILURE;
    }

    std::string cfa_pattern;
    for (int row = 0; row < 2; row++) {
        for (int col = 0; col < 2; col++) {
            const int color_index = raw.COLOR(sizes.top_margin + row, sizes.left_margin + col);

            if (color_index < 0 || color_index >= 4) {
                std::cerr << "Unsupported CFA\n";

                return EXIT_FAILURE;
            }

            cfa_pattern += idata.cdesc[color_index];
        }
    }

    const bool is_standard_bayer =  (cfa_pattern == "RGGB") ||
                                    (cfa_pattern == "GRBG") ||
                                    (cfa_pattern == "GBRG") ||
                                    (cfa_pattern == "BGGR");

    if (!is_standard_bayer) {
        std::cerr << "Unsupported CFA pattern: " << cfa_pattern << "\n";

        return EXIT_FAILURE;
    }

    //check orientation
    if (sizes.flip != 0) {
        std::cerr << "Unsupported orientation\n";

        return EXIT_FAILURE;
    }

    //check visible size
    if (sizes.width < OUTPUT_WIDTH || sizes.height < OUTPUT_HEIGHT) {
        std::cerr << "Visible RAW area is too small: " << sizes.width << "x" << sizes.height << "; Required 1920x1080";

        return EXIT_FAILURE;
    }

    if (!tags.black_level) {
        std::cerr << "Missing explicit BlackLevel metadata\n";
        return EXIT_FAILURE;
    }
    if (tags.black_delta) {
        std::cerr << "Unsupported BlackLevelDeltaH/V in four-phase input profile\n";
        return EXIT_FAILURE;
    }

    //check white level
    const unsigned int white_level = dng_levels.dng_whitelevel[0];

    if (white_level < RAW12_MAX || white_level > WHITE_LEVEL_MAX) {
        std::cerr << "Unsupported WhiteLevel: " << white_level << "\n";

        return EXIT_FAILURE;
    }

    for (int channel = 1; channel < 4; channel++) {
        if (dng_levels.dng_whitelevel[channel] != 0 && dng_levels.dng_whitelevel[channel] != white_level) {
            std::cerr << "WhiteLevel entries do not match\n";

            return EXIT_FAILURE;
        }
    }

    //check raw buffer layout
    if (sizes.raw_pitch % sizeof(unsigned short) != 0) {
        std::cerr << "RAW pitch is not aligned to the sample size\n";

        return EXIT_FAILURE;
    }

    const std::size_t row_stride = sizes.raw_pitch / sizeof(unsigned short);

    if (row_stride < sizes.raw_width) {
        std::cerr << "RAW row stride is smaller than RAW width\n";

        return EXIT_FAILURE;
    }

    const std::size_t visible_right = static_cast<std::size_t>(sizes.left_margin) + sizes.width;
    const std::size_t visible_bottom = static_cast<std::size_t>(sizes.top_margin) + sizes.height;

    if (visible_right > sizes.raw_width || visible_bottom > sizes.raw_height) {
        std::cerr << "Visible RAW area exceeds RAW buffer dimensions\n";

        return EXIT_FAILURE;
    }

    //calculate center crop
    std::size_t crop_left = static_cast<std::size_t>(sizes.left_margin) + (static_cast<std::size_t>(sizes.width) - OUTPUT_WIDTH) / 2;
    std::size_t crop_top = static_cast<std::size_t>(sizes.top_margin) + (static_cast<std::size_t>(sizes.height) - OUTPUT_HEIGHT) / 2;

    //align crop to rggb
    std::size_t red_row_offset = 0;
    std::size_t red_col_offset = 0;
    bool red_found = false;

    for (int row_offset = 0; row_offset < 2 && !red_found; row_offset++) {
        for (int col_offset = 0; col_offset < 2; col_offset++) {
            const int color_index = raw.COLOR(static_cast<int>(crop_top) + row_offset, static_cast<int>(crop_left) + col_offset);

            if (idata.cdesc[color_index] == 'R') {
                red_row_offset = static_cast<std::size_t>(row_offset);
                red_col_offset = static_cast<std::size_t>(col_offset);
                red_found = true;

                break;
            }
        }
    }

    if (!red_found) {
        std::cerr << "Cannot align crop to RGGB\n";

        return EXIT_FAILURE;
    }

    crop_top += red_row_offset;
    crop_left += red_col_offset;

    //check aligned crop bounds
    if (crop_left + OUTPUT_WIDTH > visible_right || crop_top + OUTPUT_HEIGHT > visible_bottom) {
        std::cerr << "RGGB-aligned crop exceeds visible RAW area\n";

        return EXIT_FAILURE;
    }

    std::array<double, 4> native_black{};
    blc::BlcConfig blc_config{};
    if (!black_levels_for_crop(dng_levels, crop_top - sizes.top_margin, crop_left - sizes.left_margin, white_level, native_black, blc_config)) {
        return EXIT_FAILURE;
    }

    //copy and normalize crop
    std::vector<unsigned short> clean_rggb(OUTPUT_WIDTH * OUTPUT_HEIGHT);

    for (std::size_t row = 0; row < OUTPUT_HEIGHT; row++) {
        for (std::size_t col = 0; col < OUTPUT_WIDTH; col++) {
            const std::size_t source_index = (crop_top + row) * row_stride + crop_left + col;
            const std::size_t output_index = row * OUTPUT_WIDTH + col;
            const unsigned int sample = raw_image[source_index];
            const unsigned int clipped_sample = sample > white_level ? white_level : sample;
            const std::uint64_t numerator = 2ULL * clipped_sample * RAW12_MAX + white_level;

            clean_rggb[output_index] = static_cast<unsigned short>(numerator / (2ULL * white_level));
        }
    }

    if (check_only) {
        std::cout << "RAW compatibility check passed\n";
        return EXIT_SUCCESS;
    }

    std::vector<unsigned short> corrupted_rggb;
    std::vector<Defect> defects;

    if (!inject_defects(clean_rggb, corrupted_rggb, defects, seed)) {
        return EXIT_FAILURE;
    }

    std::vector<std::uint16_t> reference_blc;
    std::vector<std::uint16_t> corrupted_blc;
    blc::blc_frame(clean_rggb, reference_blc, OUTPUT_WIDTH, OUTPUT_HEIGHT, blc_config);
    blc::blc_frame(corrupted_rggb, corrupted_blc, OUTPUT_WIDTH, OUTPUT_HEIGHT, blc_config);
    if (!write_pgm("reference_blc.pgm", reference_blc) || !write_pgm("corrupted_blc.pgm", corrupted_blc)) {
        std::cerr << "Cannot write post-BLC frames\n";
        return EXIT_FAILURE;
    }
    std::ofstream metadata("input_metadata.json");
    metadata << std::setprecision(17)
             << "{\n  \"source\": " << std::quoted(input_path)
             << ",\n  \"libraw\": " << std::quoted(LibRaw::version())
             << ",\n  \"width\": " << OUTPUT_WIDTH << ", \"height\": " << OUTPUT_HEIGHT
             << ",\n  \"crop_top\": " << crop_top << ", \"crop_left\": " << crop_left
             << ",\n  \"white_level\": " << white_level
             << ",\n  \"native_black_r_gr_gb_b\": [" << native_black[0] << ',' << native_black[1] << ',' << native_black[2] << ',' << native_black[3] << ']'
             << ",\n  \"black_r_gr_gb_b\": [" << blc_config.black_level_r << ',' << blc_config.black_level_gr << ',' << blc_config.black_level_gb << ',' << blc_config.black_level_b << ']'
             << ",\n  \"seed\": " << seed << ", \"hot\": " << HOT_DEFECT_COUNT << ", \"dead\": " << DEAD_DEFECT_COUNT << ", \"stuck\": 0\n}\n";
    metadata.close();
    if (!metadata) {
        std::cerr << "Cannot write input_metadata.json\n";
        return EXIT_FAILURE;
    }

    //WRITE OUTPUTS
    if (!write_pgm(CLEAN_OUTPUT_PATH, clean_rggb)) {
        std::cerr << "Cannot write " << CLEAN_OUTPUT_PATH << "\n";

        return EXIT_FAILURE;
    }

    if (!write_pgm(CORRUPTED_OUTPUT_PATH, corrupted_rggb)) {
        std::cerr << "Cannot write " << CORRUPTED_OUTPUT_PATH << "\n";

        return EXIT_FAILURE;
    }

    if (!write_csv(DEFECT_OUTPUT_PATH, defects)) {
        std::cerr << "Cannot write " << DEFECT_OUTPUT_PATH << "\n";

        return EXIT_FAILURE;
    }

    std::cout << "Created " << CLEAN_OUTPUT_PATH << ", " << CORRUPTED_OUTPUT_PATH << ", and " << DEFECT_OUTPUT_PATH << "\n";

    return EXIT_SUCCESS;
}
