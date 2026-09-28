#!/usr/bin/env bash
# Project: Adaptive Directional BPC and BLC
# Module: Standalone BLC Streaming Checks
# Description: Run bounded 16x16 CSim, synthesis, ordered CoSim, and generated-RTL timing checks.
# Author: Viet Nguyen To Quoc

set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
work_dir="$repo_root/Viet/build/blc_streaming_gate"
rtl_dir="$work_dir/rtl_test"
cfg="$work_dir/blc_streaming_16x16.cfg"
mkdir -p "$rtl_dir"

cat > "$cfg" <<EOF
part=xczu7ev-ffvc1156-2-e

[hls]
syn.top=blc_top
clock=150MHz
clock_uncertainty=10%
syn.cflags=-I$repo_root -I$repo_root/Viet/hls -I$repo_root/Viet/hls/blc -DISP_FRAME_WIDTH=16 -DISP_FRAME_HEIGHT=16
syn.file=$repo_root/Viet/hls/blc/blc_top.cpp
syn.file=$repo_root/Viet/hls/blc/isp_blc.cpp
tb.cflags=-I$repo_root -I$repo_root/Viet/hls -I$repo_root/Viet/hls/blc -I$repo_root/Viet/reference -DISP_FRAME_WIDTH=16 -DISP_FRAME_HEIGHT=16
tb.file=$repo_root/Viet/tests/test_blc_two_frames_hls.cpp
tb.file=$repo_root/Viet/reference/blc.cpp
cosim.setup=true
EOF

timeout 60s vitis-run --mode hls --csim --config "$cfg" --work_dir "$work_dir"
timeout 180s v++ -c --mode hls --config "$cfg" --work_dir "$work_dir"

cat > "$rtl_dir/run.tcl" <<'EOF'
run all
quit
EOF

(
    cd "$rtl_dir"
    timeout 30s xvlog -sv "$work_dir"/hls/syn/verilog/*.v \
        "$repo_root/Viet/tests/test_blc_streaming_rtl.sv"
    timeout 30s xelab test_blc_streaming_rtl --snapshot blc_streaming_test
    timeout 30s xsim blc_streaming_test -tclbatch run.tcl | tee rtl_test.log
    grep -q '^PASS: ordered AXI-Lite config' rtl_test.log
)

# Vitis generates five independent AXI-Lite writer threads from the five
# DirectIO ports. Set up CoSim, then order only its generated configuration
# driver; the synthesized DUT and C test vectors remain unchanged.
timeout 180s vitis-run --mode hls --cosim --config "$cfg" --work_dir "$work_dir"
sequence="$work_dir/hls/sim/verilog/svtb/blc_top_subsys_test_sequence_lib.sv"
python3 "$repo_root/Viet/scripts/patch_blc_cosim_order.py" "$sequence"
(
    cd "$work_dir/hls/sim/verilog"
    timeout 180s sh sim.sh | tee ordered_cosim.log
    test "$(grep -c '^BLC two 16x16 frames: 512 pixels checked, 0 failures$' ordered_cosim.log)" -eq 2
    ! grep -q '^ERROR:' ordered_cosim.log
)
