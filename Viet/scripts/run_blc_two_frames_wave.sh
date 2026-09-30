#!/usr/bin/env bash
# Project: Adaptive Directional BPC and BLC
# Module: Two-Frame BLC Waveform
# Description: Synthesize 16x16 BLC and capture two consecutive frames as a VCD.
# Author: Viet Nguyen To Quoc

set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
work_dir="$repo_root/Viet/build/blc_two_frames_wave"
rtl_dir="$work_dir/rtl"
cfg="$work_dir/blc_16x16.cfg"
mkdir -p "$rtl_dir"

cat > "$cfg" <<EOF
part=xczu7ev-ffvc1156-2-e

[hls]
syn.top=blc_top
clock=150MHz
clock_uncertainty=10%
syn.cflags=-I$repo_root -I$repo_root/Viet/hls -I$repo_root/Viet/hls/blc/none -DISP_FRAME_WIDTH=16 -DISP_FRAME_HEIGHT=16
syn.file=$repo_root/Viet/hls/blc/none/blc_top.cpp
syn.file=$repo_root/Viet/hls/blc/none/isp_blc.cpp
EOF

timeout 180s v++ -c --mode hls --config "$cfg" --work_dir "$work_dir"

cat > "$rtl_dir/run.tcl" <<'EOF'
open_vcd blc_two_frames.vcd
log_vcd /test_blc_streaming_rtl/dut/*
run all
close_vcd
quit
EOF

(
    cd "$rtl_dir"
    timeout 30s xvlog -sv "$work_dir"/hls/syn/verilog/*.v \
        "$repo_root/Viet/tests/test_blc_streaming_rtl.sv"
    timeout 30s xelab test_blc_streaming_rtl --debug typical --snapshot blc_two_frames
    timeout 30s xsim blc_two_frames -testplusarg two_frames_only -tclbatch run.tcl \
        | tee sim.log
    grep -q '^PASS: two 16x16 BLC frames, 512 beats, continuous frame boundary$' sim.log
    test -s blc_two_frames.vcd
)

printf 'VCD: %s/blc_two_frames.vcd\n' "$rtl_dir"
