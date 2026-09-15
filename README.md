<!--
Project: Adaptive Directional BPC and BLC
Module: Repository Overview
Description: Describe the owned reference workspace and reproducible one-image pipeline.
Author: Viet Nguyen To Quoc
-->

# Adaptive Directional BPC HLS

Personal R&D workspace for:

- Adaptive Directional Bad Pixel Correction (BPC)
- Black Level Correction (BLC)
- FPGA High-Level Synthesis and verification

This repository contains only work owned by Viet Nguyen To Quoc. It is not the full Viettel ISP Project and contains no confidential Viettel source or data.

## Structure

- `reference/`: Algorithm and golden-reference code
- `hls/`: Synthesizable HLS code
- `tests/`: Verification code
- `scripts/`: Experiment utilities
- `docs/`: Engineering contracts and decisions

## One-image RAW/BLC/BPC experiment

For the compatible-image batch workflow (fixed manifest, split, tuning,
selection, and held-out test), see [FiveK experiment scripts](docs/fivek-experiment.md).
The `adaptive_v2` workflow reuses the existing compatibility manifest and writes
all new stage results below `artifacts/adaptive_v2/`.

The loader decodes a supported DNG, crops/normalizes RGGB RAW12, injects only
hot/dead defects, and runs BLC on both reference and corrupted frames. The BPC
evaluator consumes those post-BLC frames. See [input contract](docs/defect-injection.md#11-blc-pipeline-profile)
and [evaluation contract](docs/bpc-evaluation-methodology.md#11-one-image-pipeline-experiment).

Dependencies: C++17 compiler, `pkg-config`, LibRaw development package, Python 3
and NumPy. Build and test from the repository root:

```sh
bash scripts/build_reference.sh
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -Ireference tests/test_reference.cpp reference/blc.cpp reference/bpc_baseline.cpp reference/bpc_adaptive.cpp -o build/test_reference
build/test_reference
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -Ireference tests/bpc_reference_test.cpp reference/bpc_adaptive.cpp -o build/bpc_reference_test
build/bpc_reference_test
python3 tests/test_pipeline.py
python3 tests/test_evaluator.py
python3 tests/test_fivek_experiment.py
```

Run one explicitly selected source (replace `/absolute/path/image.dng`):

```sh
mkdir -p artifacts/my_image
cd artifacts/my_image
../../build/defect_injector /absolute/path/image.dng
cd ../..
python3 scripts/sweep_one_image.py artifacts/my_image --output artifacts/adaptive_v2/one_image
```

Use a fresh output directory for each sweep. The script runs coarse search,
per-CFA separation, refinement and final shift audit; the maximum scheduled
candidate count is 75,942 before clipping and deduplication. The C++ evaluator
builds exact per-phase integer metric tables, and Python combines/ranks them.
The local native-int64 `metrics.bin` is a generated cache, not a portable dataset
format. It occupies about 85 MiB. No full-frame BPC call is made per candidate;
the script checks the cache against the original BPC on 12 configurations and
the selected configuration.

`candidates.csv` records all unique candidates. `summary.json` records the
baseline, selected configuration, timings, input/executable hashes and verification
configurations. Baseline shared threshold defaults to the historical exploratory
value 1063; override explicitly with `--baseline-threshold`. A one-image sweep
does not establish generalization or physical-defect ground truth.

For the batch experiment, the highest-mean-BRG adaptive candidate supplies the
mean BCR/IOTCR comparison budget on tuning data. The baseline threshold is then
trained for maximum mean BRG without exceeding that adaptive operating point.

The checked experiment and limitations are recorded in the
[historical v1 evaluation evidence](docs/bpc-evaluation-methodology.md#12-historical-adaptive-v1-one-image-run-2026-09-07).
