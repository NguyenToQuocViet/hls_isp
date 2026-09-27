#include "isp_3blocks_dataflow_top.h"
#include "isp_golden_fixed.h"

#include <cstdint>
#include <iostream>
#include <vector>

namespace {

const int FRAME_COUNT = 2;
const int TOTAL_PIXELS = FRAME_COUNT * ISP_FRAME_PIXELS;
const int EDGE_THRESHOLD = 270;
const int EDGE_MAG = 23;
const int MAX_PRINTED_ERRORS = 20;

std::uint16_t make_raw(int frame, int row, int col) {
    const unsigned value =
        static_cast<unsigned>(frame * 113)
      + static_cast<unsigned>(row * 29)
      + static_cast<unsigned>(col * 17)
      + static_cast<unsigned>((row * col * 3) ^ (col * 11));
    return static_cast<std::uint16_t>(value & 0x03ffu);
}

axis_raw10_t make_input(int frame, int row, int col) {
    axis_raw10_t p;
    p.data = make_raw(frame, row, col);
    p.keep = -1;
    p.strb = -1;
    p.user = (row == 0 && col == 0) ? 1 : 0;
    p.last = (col == ISP_FRAME_WIDTH - 1) ? 1 : 0;
    p.id = 0;
    p.dest = 0;
    return p;
}

} // namespace

int main() {
    const wb_gain_t gain_r  = (wb_gain_t)1.125;
    const wb_gain_t gain_gr = (wb_gain_t)0.9375;
    const wb_gain_t gain_gb = (wb_gain_t)1.0625;
    const wb_gain_t gain_b  = (wb_gain_t)1.25;

    const ccm_coeff_t a00 = (ccm_coeff_t)1.125;
    const ccm_coeff_t a01 = (ccm_coeff_t)-0.0625;
    const ccm_coeff_t a02 = (ccm_coeff_t)-0.0625;
    const ccm_coeff_t a10 = (ccm_coeff_t)-0.03125;
    const ccm_coeff_t a11 = (ccm_coeff_t)1.0625;
    const ccm_coeff_t a12 = (ccm_coeff_t)-0.03125;
    const ccm_coeff_t a20 = (ccm_coeff_t)-0.0625;
    const ccm_coeff_t a21 = (ccm_coeff_t)-0.0625;
    const ccm_coeff_t a22 = (ccm_coeff_t)1.125;

    const isp_golden::WbConfigQ12 wb_cfg = {
        isp_golden::encode_ufixed_q12(1.125),
        isp_golden::encode_ufixed_q12(0.9375),
        isp_golden::encode_ufixed_q12(1.0625),
        isp_golden::encode_ufixed_q12(1.25)
    };
    const isp_golden::CcmConfigQ12 ccm_cfg = {
        isp_golden::encode_fixed_q12(1.125),
        isp_golden::encode_fixed_q12(-0.0625),
        isp_golden::encode_fixed_q12(-0.0625),
        isp_golden::encode_fixed_q12(-0.03125),
        isp_golden::encode_fixed_q12(1.0625),
        isp_golden::encode_fixed_q12(-0.03125),
        isp_golden::encode_fixed_q12(-0.0625),
        isp_golden::encode_fixed_q12(-0.0625),
        isp_golden::encode_fixed_q12(1.125)
    };

    hls::stream<axis_raw10_t> stream_in;
    hls::stream<axis_rgb36_t> stream_out;
    std::vector<isp_golden::Rgb12> expected;
    expected.reserve(TOTAL_PIXELS);

    // Hai frame duoc nap lien tiep
    for (int frame = 0; frame < FRAME_COUNT; ++frame) {
        std::vector<std::uint16_t> raw(ISP_FRAME_PIXELS);
        for (int row = 0; row < ISP_FRAME_HEIGHT; ++row) {
            for (int col = 0; col < ISP_FRAME_WIDTH; ++col) {
                const int index = row * ISP_FRAME_WIDTH + col;
                raw[index] = make_raw(frame, row, col);
                stream_in.write(make_input(frame, row, col));
            }
        }

        const std::vector<isp_golden::Rgb12> frame_expected =
            isp_golden::pipeline_frame(
                raw,
                ISP_FRAME_WIDTH,
                ISP_FRAME_HEIGHT,
                wb_cfg,
                EDGE_THRESHOLD,
                EDGE_MAG,
                ccm_cfg
            );
        expected.insert(
            expected.end(), frame_expected.begin(), frame_expected.end());
    }

    
    isp_3blocks_dataflow_top(
        stream_in,
        stream_out,
        gain_r, gain_gr, gain_gb, gain_b,
        EDGE_THRESHOLD, EDGE_MAG,
        a00, a01, a02,
        a10, a11, a12,
        a20, a21, a22,
        (frame_count_t)FRAME_COUNT
    );

    int errors = 0;
    int checked = 0;
    for (int index = 0; index < TOTAL_PIXELS; ++index) {
        if (stream_out.empty()) {
            std::cerr << "[FAIL] Missing output at index=" << index << '\n';
            ++errors;
            break;
        }

        const axis_rgb36_t got = stream_out.read();
        const unsigned got_r = (unsigned)got.data.range(11, 0);
        const unsigned got_g = (unsigned)got.data.range(23, 12);
        const unsigned got_b = (unsigned)got.data.range(35, 24);
        const isp_golden::Rgb12 &ref = expected[index];

        const int in_frame = index % ISP_FRAME_PIXELS;
        const int row = in_frame / ISP_FRAME_WIDTH;
        const int col = in_frame % ISP_FRAME_WIDTH;
        const unsigned expected_user =
            (row == 0 && col == 0) ? 1u : 0u;
        const unsigned expected_last =
            (col == ISP_FRAME_WIDTH - 1) ? 1u : 0u;

        const bool wrong_data =
            got_r != ref.r || got_g != ref.g || got_b != ref.b;
        const bool wrong_sideband =
            (unsigned)got.user != expected_user
         || (unsigned)got.last != expected_last
         || (unsigned)got.keep != 0x1fu
         || (unsigned)got.strb != 0x1fu;

        if (wrong_data || wrong_sideband) {
            if (errors < MAX_PRINTED_ERRORS) {
                std::cerr << "[FAIL] index=" << index
                          << " DUT=(" << got_r << ',' << got_g << ',' << got_b
                          << ") GOLD=(" << ref.r << ',' << ref.g << ',' << ref.b
                          << ") user=" << (unsigned)got.user
                          << " last=" << (unsigned)got.last << '\n';
            }
            ++errors;
        }
        ++checked;
    }

    if (!stream_out.empty()) {
        std::cerr << "[FAIL] Unexpected extra output token\n";
        ++errors;
    }
    if (!stream_in.empty()) {
        std::cerr << "[FAIL] DUT did not consume all real input pixels\n";
        ++errors;
    }

    if (errors != 0) {
        std::cerr << "[FAIL] 3-block DATAFLOW test: "
                  << errors << " errors\n";
        return 1;
    }

    std::cout << "[PASS] WB -> Demosaic -> CCM bit-exact\n"
              << "       frames=" << FRAME_COUNT
              << ", size=" << ISP_FRAME_WIDTH << 'x' << ISP_FRAME_HEIGHT
              << ", checked=" << checked << " pixels\n"
              << "       Check CoSim wave: after first output handshake, "
              << "TVALID must stay high through the frame boundary when "
              << "TREADY=1.\n";
    return 0;
}
