<!--
Project: Adaptive Directional BPC and BLC
Module: HDR Plus RAW10 Dataset
Description: Record the exact clean RAW10 BL64 selection and export contract for HDR Plus DNGs.
Author: Viet Nguyen To Quoc
-->

# HDR+ clean RAW10, Black Level 64

## Input and selection

The source is Google's [HDR+ Burst Photography Dataset curated subset](https://hdrplusdata.org/dataset.html). The initial candidate table at `scripts/hdrplus_bl64_candidates.tsv` lists 55 original `payload_N000.dng` files whose first 64 KiB expose `WhiteLevel=1023` and four `BlackLevel=64` entries. This is a metadata prefilter, not an acceptance claim. The initial run covered one frame per listed burst. Use the dataset under its published CC BY-SA terms when distributing derived images.

Each full DNG is accepted only if the existing LibRaw loader can decode a single Bayer plane, normal orientation, supported CFA and crop, explicit BlackLevel with no delta tags, exact native WhiteLevel 1023, and exact native BlackLevel 64 at all four phases after crop alignment. A decoded crop sample above 1023 is rejected. A file with fractional BlackLevel that rounds to 64 is rejected.

## Output

The clean export is a **1920 × 1080 RGGB Bayer mosaic**. It uses a centered crop whose origin is shifted by at most one pixel per axis to put red at `(0,0)`; there is no resize or demosaic. The `clean_rggb.pgm` file is binary P5 with `Maxval=1023`, using two big-endian bytes per pixel as required by PGM. Its effective sample domain is RAW10 `[0,1023]`. The file is not CSI-2 packed RAW10. Since accepted input has WhiteLevel 1023, export does not rescale or subtract the pedestal. `input_metadata.json` records the crop and four black levels. No defects are injected.

Original DNGs, clean images, and `manifest.json` are placed under ignored `artifacts/hdrplus_bl64/`. The manifest records the URL, source and output SHA-256, acceptance status, and rejection reason. Re-running the script keeps downloaded sources and accepted exports.

The initial run downloaded all 55 candidate DNGs (about 1.2 GiB). Full decoding accepted **41** and rejected **14** for unsupported orientation. The 41 initial clean PGM files occupy about 163 MiB. These counts apply only to that initial candidate set.

## Curated expansion and cleanup

`scripts/hdrplus_curated_scan.py` lists public bucket objects through the paginated Google Cloud Storage JSON API. The listing contains **1,355 original DNG frames across 153 bursts** and is cached in `artifacts/hdrplus_bl64/curated_objects.json`. It visits every `N000` before `N001`, then `N002`, and so on, sorting burst names within each frame index. This prioritizes scene coverage before adding more frames of the same burst.

For each new URL, the scanner requests exactly the first 64 KiB using HTTP Range and reads the metadata with ExifTool. It rejects mismatching native WhiteLevel/BlackLevel, black-level delta tags, and explicitly unsupported orientation before downloading the full file. Missing orientation in the prefix is left to the full LibRaw check. Files passing the prefilter still undergo the full clean-export contract above. The scanner resumes by URL, retains accepted source DNGs and outputs, removes newly downloaded rejected sources, and stops at the requested total accepted count. It keeps a 2 GiB free-space reserve.

`curated_scan.json` records each new inspection, metadata, and rejection/error reason; `manifest.json` combines initial and newly decoded files with hashes. Nonzero frame outputs are named `<burst>__payload_Nxxx`, while source files remain under `source/<burst>/payload_Nxxx.dng`. Multiple frames of one burst are correlated images of the same scene, not additional independent scenes. No visual ranking or WB/CCM grouping is performed by this scan. Earlier WB/CCM CSV files describe only the initial 41 images.

The first expansion stopped at **200 accepted frames** after visiting 723 URLs. The subsequent exhaustive run inspected the remaining **632 URLs**, adding **188 accepted frames**. The entire curated subset has now been visited: **1,355/1,355 original DNGs (100%)**, with **388 accepted frames from 41 bursts** and **967 rejected frames**. The combined evidence consists of 55 initial manifest entries (41 accepted and 14 orientation rejections) and 1,300 scan entries (347 accepted and 953 metadata rejections). There are no unresolved errors. The accepted set includes 35 bursts with ten frames, four with six, and two with seven; frame counts are 41 each at `N000`–`N005`, 37 at `N006`, and 35 each at `N007`–`N009`.

Final verification checked all **388 source/output SHA-256 hashes**, output headers and sample ranges, exact contract metadata, and distinct crop hashes. It also checked that every listed URL has a recorded terminal result. There are exactly 388 retained source DNGs and 388 clean PGM files, with no defect CSVs or partial downloads. Retained sources total **8,328,371,970 bytes**; clean PGMs total **1,609,120,584 bytes**. The machine had approximately **67.9 GiB free** after completion. `artifacts/hdrplus_bl64/expansion_summary.json` contains the measured counts and verification summary. The additional frames come from the same 41 bursts, so they do not increase the number of independent scenes. WB/CCM grouping remains deferred.

The user-authorized cleanup removed only DNGs rejected by the existing manifests: **3,896 FiveK files (38,495,722,062 bytes)** and **14 initial HDR+ files (337,676,976 bytes)**. All 1,104 compatible FiveK source DNGs and the 41 initial accepted HDR+ sources/exports were retained. The deletion audit is `artifacts/dataset_cleanup.json`; historical manifests retain their rejected entries even though those source files are no longer on disk. FiveK retention follows its original compatibility filter, not a new native BL64 rescan.

To expand or resume, from `Viet/`:

```sh
rtk python3 scripts/hdrplus_curated_scan.py --target 1356
```

This target exceeds the 1,355-frame listing so the scanner exhausts the subset rather than stopping at an accepted-image quota. With the completed ledger, re-running this command skips all visited URLs.

The original candidate-table downloader below can fetch the 14 rejected initial sources again if invoked with `--download`; use the curated scanner for further expansion.

## WB and CCM statistics (all 388 accepted frames)

`scripts/hdrplus_color_stats.py` extracts metadata from every accepted original DNG using ExifTool and downloads one `rgb2rgb.txt` per accepted burst. The dataset defines this row-major 3×3 matrix as sensor RGB to linear sRGB, excluding WB. The script derives green-normalized WB `(nG/nR, 1, nG/nB)` from `AsShotNeutral`; DNG `ColorMatrix1/2` and the other calibration tags remain separate columns. Missing camera identities remain unknown, and nonpositive exposure/ISO values are excluded from measurement summaries.

Artifacts are in `artifacts/hdrplus_bl64/color_stats/`: `frames.csv` (388 rows), `bursts.csv` (41 rows), `ccm_profiles.csv` (22 rows), `summary.json`, `report.md`, the raw ExifTool JSON, downloaded sidecars with URL/hash provenance, and `verification.json`. These do not overwrite the older WB/CCM CSVs for the initial 41-frame set.

All 388 frames have valid WB; there are **38 distinct WB pairs**, with no variation within any of the 41 bursts. Frame-weighted and burst-median-weighted minimum/median/maximum values are the same in this set: R/G **1.107422 / 1.882353 / 2.433962**, B/G **1.333333 / 1.600000 / 3.029293**. G is normalized to 1. The 41 published sidecars contain **22 numerically distinct CCMs**, grouped by exact equality of their nine coefficients with no tolerance or normalization. This differs from the DNG metadata, which has six `ColorMatrix1` values and seven `ColorMatrix2` values. The largest exact CCM profile contains 148 frames from 16 bursts; this is a frequency count, not a WB-compatible selection.

ExifTool 13.50 extraction and independent artifact checks confirmed manifest membership, frame/burst/profile counts, WB recomputation, all 41 sidecar hashes/coefficient arrays, and CCM membership counts. No metadata vector validation issues were found. ExposureTime is positive in 284 frames and ISO in 254; the other values are zero and cannot be used as valid measurements. No approximate grouping, tolerance sweep, common WB, or 32-image selection has been performed.

Run from `Viet/`:

```sh
rtk python3 scripts/hdrplus_color_stats.py --download
```

Omit `--download` for an offline re-extraction using cached sidecars.

## Largest WB-compatible groups (exact CCM)

`scripts/hdrplus_color_groups.py` reports the largest two disjoint groups at WB tolerances ±5% and ±10%, using all 388 metadata records. Within each group, all nine published `rgb2rgb.txt` coefficients must match exactly. For each WB channel R/G and B/G, `abs(gain / common_gain - 1) <= tolerance`; G remains 1. The common gain is the midpoint of the group's minimum and maximum, minimizing maximum relative deviation. Thus ±5% does not mean a 5% endpoint-to-endpoint range: the allowed max/min ratio is `(1+t)/(1-t)`.

Group 1 maximizes frame count; group 2 maximizes frame count after removing group 1. Ties prefer more bursts, then smaller WB deviation, then deterministic profile/frame identifiers. The search enumerates rectangles anchored at observed WB minima within each exact CCM. An independent exhaustive enumeration of every burst subset confirmed the maximum frame count for all four reported groups; this is valid here because WB is identical within each burst. This establishes metadata compatibility, not rendered-color accuracy.

| Tolerance | Rank | Frames | Bursts | CCM | Common WB (R,G,B) | Maximum R/B deviation |
|---|---:|---:|---:|---|---|---|
| ±5% | 1 | 70 | 7 | C15 | 1.983681, 1, 1.663614 | 4.8881% / 4.7865% |
| ±5% | 2 | 22 | 3 | C15 | 2.306641, 1, 1.451042 | 4.9958% / 1.2204% |
| ±10% | 1 | 112 | 12 | C15 | 2.026367, 1, 1.628906 | 8.1446% / 9.8321% |
| ±10% | 2 | 40 | 4 | C08 | 1.714286, 1, 1.645353 | 6.6667% / 5.1282% |

Reports, matrices, burst identifiers and per-frame membership/deviation CSVs are under `artifacts/hdrplus_bl64/color_stats/groups_pm05/` and `groups_pm10/`; independent verification is `groups_verification.json`. No image files are copied or modified and no 32-frame sample is chosen. Reproduce from `Viet/` with `rtk python3 scripts/hdrplus_color_groups.py`.

## Reproduce clean exports

From `Viet/`:

```sh
rtk bash scripts/build_reference.sh
rtk python3 scripts/hdrplus_raw10_filter.py --download
```

The script downloads the listed original DNGs and validates/exports them. To re-run against already downloaded sources, omit `--download`. The executable's clean mode is `build/defect_injector --export-clean-raw10-bl64 image.dng`, run from a fresh output directory. Its historical injection mode is unchanged.
