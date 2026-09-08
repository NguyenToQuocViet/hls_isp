<!--
Project: Adaptive Directional BPC and BLC
Module: FiveK Batch Experiment
Description: Define compatible-image tuning, held-out evaluation, and resumable script usage.
Author: Viet Nguyen To Quoc
-->

# Compatible-image experiment

The compatibility scan is a one-time dataset qualification artifact. The
`adaptive_v2` experiment reuses the existing `artifacts/fivek_scan/manifest.json`;
it does not decode and filter all DNG files again. A fresh split is derived from
that manifest so deterministic per-image injection seeds are present.

The injector and BLC remain unchanged; adaptive BPC follows the `adaptive_v2`
contract. Only DNG files accepted by the recorded compatibility scan are included.
Compatibility does not guarantee that injection/evaluation succeeds: for example,
zero initial hot/dead error after BLC remains an evaluator error. Such failures are
recorded and block aggregate selection/reporting; the runner does not silently
change the split or drop images.

## Experiment decision

- Split compatible images approximately 80% tuning / 20% test with seed 20260907.
  Optional explicit scene groups stay together; group sizes can shift the ratio.
  Without a group file, this is a per-image random split and near-duplicate leakage
  has not been checked. Do not infer independence from filenames.
- Sweep each tuning image using the existing maximum 75,942-candidate schedule
  before clipping/deduplication. Per-image eligibility still uses the exploratory
  baseline operating point (default 1063, configurable). A winner is best only
  within that image's evaluated eligible candidates.
- The common adaptive candidate pool is the union of those per-image winners.
  Evaluate every candidate on every tuning image. This bounded pool is not the
  union of every swept config, nor an exhaustive adaptive parameter search. It
  may omit a strong common config that never wins individually; results depend
  on the exploratory baseline used for candidate generation.
- Select the adaptive candidate with highest mean BRG on tuning images. Break
  ties by lower mean IOTCR, lower mean BCR, then lexicographic config order. Its
  mean BCR and IOTCR define the matched comparison budget.
- Tune the baseline shared threshold over all integers 0..4095 on tuning images.
  Reject thresholds exceeding either selected-adaptive budget, then choose
  highest mean BRG. Break ties by lower mean IOTCR, lower mean BCR, then lower
  threshold. This optimizes one shared threshold, not three independent per-CFA
  thresholds.
- Every image has equal weight; compute each image's normalized metrics before
  averaging, not pooled pixel sums. The budget matches the selected adaptive
  operating point; it is not an independent production safety requirement.
- If no common config is eligible, record that outcome; do not relax constraints
  or run test. Eligibility does not additionally require BRG above baseline.
- Freeze both selected configurations for test. Test uses the original full-frame
  BPC implementations and performs no search. Report mean/median metrics and
  fractions of images whose BRG improves, worsens, or ties. Mean constraints on
  tuning do not guarantee per-image constraints or constraints on test.

## Adaptive v2 commands (for the user to run)

Requires the existing C++17/LibRaw build dependencies and Python 3.9+ with NumPy.
Run from the repository root. Rebuild the evaluator for its new modes first:

```sh
bash scripts/build_reference.sh
python3 scripts/fivek_experiment.py split artifacts/fivek_scan/manifest.json
python3 scripts/fivek_experiment.py sweep artifacts/adaptive_v2/split/split.json --jobs 6
python3 scripts/fivek_experiment.py select artifacts/adaptive_v2/split/split.json --sweeps artifacts/adaptive_v2/sweeps --jobs 6
python3 scripts/fivek_experiment.py test artifacts/adaptive_v2/split/split.json --selection artifacts/adaptive_v2/selection/selection.json --jobs 6
```

The omitted `--output` arguments default to stage directories under
`artifacts/adaptive_v2/`. Do not run `scan` for this v2 experiment. The existing
manifest fixes the compatible-image population; `split` only assigns the
train/test membership and per-image injection seeds.

For scene-aware splitting, add `--groups /absolute/path/groups.csv` to `split`.
The CSV must cover every compatible image, use absolute paths, and have columns:

```csv
path,group
/data/photos/image1.dng,scene_a
/data/photos/image2.dng,scene_a
/data/photos/image3.dng,scene_b
```

`--injector` and `--evaluator` overrides are global arguments before the subcommand.
`sweep --baseline-threshold T` controls only per-image adaptive candidate
eligibility during the bounded search. The final comparison budget comes from
the selected highest-BRG adaptive operating point; `select` trains the baseline
under that budget.

## Outputs and resuming

| Stage | Main artifacts |
|---|---|
| scan (reused) | `artifacts/fivek_scan/manifest.json` |
| split | `artifacts/adaptive_v2/split/split.json` |
| sweep | `artifacts/adaptive_v2/sweeps/per_image_best.csv`; per-image `summary.json`, `candidates.csv`, `baseline.npy` |
| select | `artifacts/adaptive_v2/selection/baseline_candidates.csv`, `common_candidates.csv`, `selection.json`; per-image common-candidate metrics |
| test | `artifacts/adaptive_v2/test/per_image.csv`, `report.json`; per-image raw counts and normalized metrics |

Rerun the same command/output directory to resume. Completed image checkpoints
are skipped; failed images are retried. `run.log` retains subprocess output;
`error.json` records failures. A stage with image failures exits nonzero and does
not publish its final aggregate. All tuning images must complete before selection.

Each v2 run records the algorithm identifier, executable/script hashes and its
input manifest or split hash. Changed run inputs require a new output directory.
Source identity uses absolute path, file size, and nanosecond modification time,
not full DNG content hashes; do not modify sources in place. The scan-time
injector hash remains in the historical manifest, while sweep/select/test record
the current injector and evaluator hashes independently. A changed runtime binary
therefore requires fresh v2 stage outputs, not another compatibility scan.

`sweep`, `select`, and `test` accept `--jobs N` and process up to `N` independent
images concurrently. The default is one worker; `--jobs 6` matches the six physical
cores of the measured development machine. Worker count affects throughput, not
seeds, candidate ranking, aggregation order, or run provenance, so a resumed run
may use a different value.

Each active worker owns a separate image directory, log, PGM pair and approximately
85 MiB adaptive bank. These temporary files are removed on ordinary completion or
failure. The select pass regenerates each tuning image and bank to avoid retaining
thousands of banks. An uncatchable process termination can leave a `work-*`
directory; these directories are not reused. Candidate CSV files are retained and
can still consume substantial disk space across thousands of tuning images.

The evaluator's new `--baseline-bank FILE` mode writes native int64 counts shaped
`(4096,4)` and compares five directed thresholds against the original baseline
when invoked. `--fixed BASELINE_T R G B ks ka` returns paired original-model counts
and denominators without generating a bank. These are script support interfaces;
they do not change the reference algorithm interfaces.

## Per-image injection seeds

Batch splits now assign injection seeds 1..N to compatible images sorted by
absolute path, before the tuning/test shuffle. Each row in `split.json` stores
`injection_seed`; sweep, select, test, and resume reuse that stored value.
The split `--seed` controls membership shuffling only. A new manifest may change
the mapping; retain the split to reproduce an experiment.

The injector accepts `--seed N <image.dng>` and records the seed in
`input_metadata.json`. Direct calls without this option retain seed 0.
This seed controls both placement and hot/dead corruption parameters.

Rebuild, derive the v2 split from the existing manifest, then run sweep, select,
and test under `artifacts/adaptive_v2`. Previous range-detector and fixed-seed
artifacts remain historical results; they do not validate `adaptive_v2`.
