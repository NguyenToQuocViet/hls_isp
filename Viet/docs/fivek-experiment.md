<!--
Project: Adaptive Directional BPC and BLC
Module: FiveK Batch Experiment
Description: Define compatible-image tuning, held-out evaluation, and resumable script usage.
Author: Viet Nguyen To Quoc
-->

# Compatible-image experiment

The compatibility scan is a one-time dataset qualification artifact. The RAW10
`adaptive_v2` experiment reuses the existing `artifacts/fivek_scan/manifest.json`;
it does not decode and filter all DNG files again. A fresh split is derived from
that manifest so deterministic per-image injection seeds are present.

The injector normalizes to RAW10 and BLC operates in that same numeric domain;
adaptive BPC follows the `adaptive_v2` contract. Only DNG files accepted by the
recorded compatibility scan are included.
Compatibility does not guarantee that injection/evaluation succeeds: for example,
zero initial hot/dead error after BLC remains an evaluator error. Such failures are
recorded and block aggregate selection/reporting; the runner does not silently
change the split or drop images.

## Experiment decision

- Split compatible images approximately 80% tuning / 20% test with seed 20260907.
  Optional explicit scene groups stay together; group sizes can shift the ratio.
  Without a group file, this is a per-image random split and near-duplicate leakage
  has not been checked. Do not infer independence from filenames.
- Sweep each tuning image using the RAW10 maximum 75,078-candidate schedule
  before clipping/deduplication. Per-image eligibility still uses the exploratory
  baseline anchor (default 266, configurable). The per-image shortlist is
  ranked only within that image's evaluated eligible candidates.
- Retain the top 10 eligible candidates from each tuning image, ordered by
  BRG, then IOTCR, BCR, and config order. The common adaptive candidate pool is
  the union of those shortlists. Evaluate every pooled candidate on every tuning
  image. This bounded pool is broader than the former top-1 union, but it is
  still not the union of every swept config or an exhaustive adaptive search.
- Select the adaptive candidate within 1% relative BRG regret of the pool's
  BRG maximum. Among those candidates, minimize the worst relative BCR/IOTCR
  safety ratio against the BRG-max reference, then the sum of those ratios,
  then prefer higher BRG. Its mean BCR and IOTCR define the matched comparison
  budget.
- Tune the baseline shared threshold over all integers 0..1023 on tuning images.
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
Run from `Viet/`. Rebuild the evaluator for its new modes first:

```sh
bash scripts/build_reference.sh
python3 scripts/fivek_experiment.py split artifacts/fivek_scan/manifest.json
python3 scripts/fivek_experiment.py sweep artifacts/adaptive_v2_raw10/split/split.json --jobs 6
python3 scripts/fivek_experiment.py select artifacts/adaptive_v2_raw10/split/split.json --sweeps artifacts/adaptive_v2_raw10/sweeps --top-k 10 --brg-regret 0.01 --jobs 6
python3 scripts/fivek_experiment.py test artifacts/adaptive_v2_raw10/split/split.json --selection artifacts/adaptive_v2_raw10/selection/selection.json --jobs 6
```

The omitted `--output` arguments default to stage directories under
`artifacts/adaptive_v2_raw10/`. Do not run `scan` for this experiment. The existing
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
eligibility during the bounded search. `select --top-k K` retains the top `K`
eligible per-image candidates, while `--brg-regret R` sets the relative BRG
regret floor. The final comparison budget comes from the selected adaptive
operating point; `select` trains the baseline under that budget.

For a post-selection held-out diagnostic of a different candidate already
present in the tuning table, use `freeze`. It derives the matched baseline
threshold from the supplied tuning tables and writes a new selection artifact;
it does not search or alter the original selection:

```sh
python3 scripts/fivek_experiment.py freeze \
  artifacts/adaptive_v2_raw10/selection_top10/selection.json \
  --common-candidates artifacts/adaptive_v2_raw10/selection_top10/common_candidates.csv \
  --baseline-candidates artifacts/adaptive_v2_raw10/selection_top10/baseline_candidates.csv \
  --adaptive-config 4 8 4 3 0 \
  --output artifacts/adaptive_v2_raw10/candidate_3/selection.json
```

## Outputs and resuming

| Stage | Main artifacts |
|---|---|
| scan (reused) | `artifacts/fivek_scan/manifest.json` |
| split | `artifacts/adaptive_v2_raw10/split/split.json` |
| sweep | `artifacts/adaptive_v2_raw10/sweeps/per_image_best.csv`; per-image `summary.json`, `candidates.csv`, `baseline.npy` |
| select | `artifacts/adaptive_v2_raw10/selection/baseline_candidates.csv`, `common_candidates.csv`, `per_image_topk.csv`, `selection.json`; per-image common-candidate metrics |
| test | `artifacts/adaptive_v2_raw10/test/per_image.csv`, `report.json`; per-image raw counts and normalized metrics |

Rerun the same command/output directory to resume. Completed image checkpoints
are skipped; failed images are retried. `run.log` retains subprocess output;
`error.json` records failures. A stage with image failures exits nonzero and does
not publish its final aggregate. All tuning images must complete before selection.

Each v2 run records `algorithm=adaptive_v2`, `pixel_domain=raw10`,
`experiment=adaptive_v2_raw10`, executable/script hashes and its
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
15.125 MiB adaptive bank. These temporary files are removed on ordinary completion or
failure. The select pass regenerates each tuning image and bank to avoid retaining
thousands of banks. An uncatchable process termination can leave a `work-*`
directory; these directories are not reused. Candidate CSV files are retained and
can still consume substantial disk space across thousands of tuning images.

The evaluator's new `--baseline-bank FILE` mode writes native int64 counts shaped
`(1024,4)` and compares five directed thresholds against the original baseline
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

Rebuild, derive a RAW10 split from the existing manifest, then run sweep, select,
and test under `artifacts/adaptive_v2_raw10`. Existing `artifacts/adaptive_v2`
results remain immutable RAW12 history; they do not validate the RAW10 operating point.
