#!/usr/bin/env bash
# Project: Adaptive Directional BPC and BLC
# Module: Reference Build
# Description: Build the C++ injector and one-image evaluator with the installed LibRaw.
# Author: Viet Nguyen To Quoc
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build
cxx="${CXX:-g++}"
read -r -a libraw_flags <<< "$(pkg-config --cflags --libs libraw)"
"$cxx" -std=c++17 -O3 -Wall -Wextra -Wpedantic -Werror -Ireference reference/defect_injector.cpp reference/blc.cpp "${libraw_flags[@]}" -o build/defect_injector
"$cxx" -std=c++17 -O3 -Wall -Wextra -Wpedantic -Werror -Ireference scripts/bpc_evaluate.cpp reference/bpc_baseline.cpp reference/bpc_adaptive.cpp -o build/bpc_evaluate
