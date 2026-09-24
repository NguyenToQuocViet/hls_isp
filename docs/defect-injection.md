<!--
Project: Adaptive Directional BPC and BLC
Module: Defect Injection Contract
Description: Define historical injection behavior and the current RAW10 BLC/BPC dataset profile.
Author: Viet Nguyen To Quoc
-->

# Synthetic Defect Injection Contract

## 1. Purpose and Authority

This document is the authoritative behavioral contract for the first host-side synthetic bad-pixel injector. The injector creates a controlled corrupted RAW Bayer image for BPC reference-model development and baseline evaluation.

This contract defines intended behavior. It does not claim that an implementation exists, passes tests, matches physical sensor statistics, or is suitable for HLS synthesis.

Section 11 defines the current RAW10 BLC pipeline profile and overrides the historical numeric domain, restrictions and counts below. Sections 3 through 10 preserve the version 0.1 RAW12 profile with 150 hot, 150 dead, and 25 stuck assignments for historical baseline reproducibility. That profile is not valid for new single-frame BPC tuning. Under [ADR 0001](adr/0001-exclude-stuck-from-single-frame-bpc.md), every new training, validation, and test dataset shall contain only hot and dead corruption and shall set the stuck count to zero.

## 2. Scope and Non-Goals

The injector is a C++ software utility, not an HLS block. It shall:

- extract one supported RAW Bayer plane from a DNG file;
- normalize it to an RGGB, 1920 x 1080, 12-bit mosaic by CFA-aligned cropping and, when required, deterministic linear quantization;
- inject reproducible hot, dead, and stuck defects;
- preserve both the clean and corrupted mosaics;
- record every assigned defect and its parameters.

The following are outside this contract:

- BPC or BLC correction;
- demosaicing, resizing, rotation, color processing, or tone processing;
- clustered, row, column, temporal, blinking, or coupled defects;
- per-CFA defect quotas;
- a claim that the synthetic distribution represents all real sensors;
- HLS synthesis, streaming architecture, latency, throughput, or resource optimization.

Synthetic injection provides controlled ground truth. It is not sufficient by itself to prove performance on real sensor defects.

## 3. Input Contract

The input shall be one DNG containing a linear 2 x 2 Bayer CFA raw plane. The injector shall read the raw plane, not an embedded RGB or YCbCr preview.

The raw plane is accepted only when all of the following hold:

- image orientation is already normal; orientation transforms are unsupported;
- the CFA is one of the four standard Bayer phase arrangements and contains one red, two green, and one blue site per 2 x 2 tile;
- decoded samples are unsigned integers;
- every applicable `WhiteLevel` entry has the same integer value `WL` in `[4095, 65535]`;
- every applicable `BlackLevel` entry is `0`;
- the dimensions contain the required crop after CFA alignment.

The decoded storage width does not determine the effective signal range. For example, samples stored in a 16-bit container with `WL = 4095` already have a 12-bit effective range and shall not be rescaled.

Unsupported or ambiguous metadata shall cause an explicit error. The injector shall not silently use a preview, subtract a black level, or guess a CFA pattern. The DNG decoding library is an implementation choice, but it must expose the unrendered CFA samples and required metadata.

## 4. RGGB Crop and RAW12 Normalization

The output dimensions are fixed:

```text
WIDTH  = 1920
HEIGHT = 1080
```

Let the decoded raw dimensions be `RAW_WIDTH x RAW_HEIGHT`. Define the initial center-crop origin:

```text
x0 = floor((RAW_WIDTH - WIDTH) / 2)
y0 = floor((RAW_HEIGHT - HEIGHT) / 2)
```

Use the DNG CFA metadata to determine the row and column parity of red samples. If `x0` or `y0` has the wrong parity for a red sample at the crop origin, increment that coordinate by one. The adjustment is therefore zero or one pixel per axis. Reject the input if the aligned rectangle exceeds the raw bounds.

The aligned crop becomes `clean_rggb`. Its CFA labels are:

```text
even y, even x : R
even y, odd  x : Gr
odd  y, even x : Gb
odd  y, odd  x : B
```

For each cropped decoded sample `S`, produce the clean RAW12 sample `X` using integer arithmetic:

```text
X = floor((2 * min(S, WL) * 4095 + WL) / (2 * WL))
```

The intermediate arithmetic shall be wide enough to avoid overflow. The equation linearly maps the declared effective range to `[0, 4095]` and rounds to the nearest integer, with halfway cases upward. When `WL = 4095`, it is the identity for every `S` in `[0, 4095]`. Samples above `WL` saturate at `4095` through `min(S, WL)`.

This mapping is the only pixel-value transformation permitted before defect injection. No resize, demosaic, black-level subtraction, rotation, color processing, or tone processing occurs.

Coordinates in all later sections refer to `clean_rggb`, not the original DNG. Coordinate `(0, 0)` is the top-left red sample.

## 5. Common Defect Arithmetic

For a clean integer RAW12 sample `X`, each assigned defect produces:

```text
Y = clamp(lround(a * X + b), 0, 4095)
```

where:

- `lround` rounds to the nearest integer, with halfway cases away from zero;
- `clamp` saturates below `0` and above `4095`;
- modulo arithmetic is forbidden;
- `a` and `b` are assigned once according to the defect type; each random parameter is drawn once and remains fixed for that coordinate.

Both `X` and `Y` are integer samples in `[0, 4095]`.

## 6. Defect Types and Counts

The output image contains exactly 325 assigned defect coordinates:

| Type | Count | Parameters |
|---|---:|---|
| Hot | 150 | `a = 1`; integer `b` sampled uniformly from `[256, 2048]` |
| Dead | 150 | real `a` sampled uniformly from `[0, 0.75)`; `b = 0` |
| Stuck | 25 | `a = 0`; integer `b = C` sampled uniformly from `[0, 4095]` |

The implementation shall collect these counts, parameter bounds, output dimensions, spacing, border width, and seed as named constants in one visible configuration block rather than scattering numeric literals through the code. The exact values in this contract define version 0.1.

A stuck value `C` is one constant assigned to one coordinate. It is not recomputed from `X`.

These counts describe assigned defect locations. They do not guarantee 325 numerically changed samples. For example, a hot defect on `X = 4095`, a dead defect on `X = 0`, or a stuck value equal to `X` can produce `Y = X`. Such cases remain valid assigned defects and shall remain in the CSV record.

## 7. Randomness and Spatial Placement

The injector shall use one `std::mt19937` engine initialized with the fixed constant:

```text
RNG_SEED = 0
```

Generate 325 accepted coordinates before drawing defect parameters. Candidate coordinates are independent uniform integer draws from:

```text
x in [2, 1917]
y in [2, 1077]
```

The two-pixel border exclusion keeps every assigned center inside a valid 5 x 5 neighborhood.

For a candidate `p` and every previously accepted coordinate `q`, define Chebyshev distance:

```text
d_inf(p, q) = max(abs(p.x - q.x), abs(p.y - q.y))
```

Accept the candidate only when:

```text
d_inf(p, q) >= 10
```

Otherwise reject it and draw another candidate. This rule prevents overlap and enforces the accepted minimum spacing without imposing a grid or CFA quota. Failure to obtain all 325 valid coordinates is an error; partial output is invalid.

After placement, assign the accepted coordinates in order: first 150 hot, next 150 dead, and final 25 stuck. Draw each defect parameter in that same row order using the same engine. Integer parameters use an inclusive uniform integer distribution; dead-pixel `a` uses a uniform real distribution on `[0, 0.75)`.

The same decoded input samples, constants, executable, and standard-library implementation shall produce identical outputs across repeated runs. Cross-toolchain byte identity is not a version 0.1 requirement because C++ distribution mappings are not fixed by this contract.

## 8. Output Contract

A successful run shall produce exactly three files:

### `clean_rggb.pgm`

The normalized crop before injection.

### `corrupted_rggb.pgm`

A copy of `clean_rggb.pgm` in which only the 325 assigned coordinates have been evaluated with the defect equation.

Both PGM files shall use binary Netpbm `P5` encoding with width `1920`, height `1080`, and maximum value `4095`. Because the maximum exceeds 255, each sample is stored in two bytes, most-significant byte first.

### `defects.csv`

The CSV shall contain this exact header:

```csv
x,y,cfa,type,input_value,output_value,a,b
```

It shall contain one row per assigned coordinate in injection order, for exactly 325 data rows. Field requirements are:

- `x`, `y`: zero-based output coordinates;
- `cfa`: exactly `R`, `Gr`, `Gb`, or `B`;
- `type`: exactly `hot`, `dead`, or `stuck`;
- `input_value`: `X` from the clean crop;
- `output_value`: `Y` after rounding and clipping;
- `a`, `b`: the actual parameters used by the common equation.

For dead defects, serialize `a` with enough decimal precision to recover the generated `double` value, such as `std::numeric_limits<double>::max_digits10`.

## 9. Acceptance Criteria

An implementation conforms to version 0.1 only when tests demonstrate all of the following:

1. Unsupported input metadata is rejected instead of guessed or transformed outside the Section 4 mapping.
2. An accepted input with `WL = 4095` preserves every in-range cropped sample exactly.
3. An accepted input with `WL > 4095` maps every cropped sample using the Section 4 equation, including the endpoints and saturation above `WL`.
4. The clean output is a 1920 x 1080 RGGB crop with samples in `[0, 4095]`.
5. The CSV contains 150 hot, 150 dead, and 25 stuck rows at 325 unique coordinates.
6. Every coordinate respects the two-pixel border and pairwise Chebyshev distance of at least 10.
7. Every recorded `output_value` equals the specified rounded and clipped equation result.
8. The corrupted image equals the clean image at every coordinate not listed in the CSV.
9. The three output files repeat identically for repeated runs under the reproducibility boundary in Section 7.

Passing these checks establishes conformance to this synthetic injection contract only. It does not establish BPC correction quality or physical sensor realism.

## 10. Change Control

Version 0.1 separates decoded storage width from effective signal range and defines deterministic linear quantization when `WhiteLevel` exceeds `4095`. The defect model, placement, randomness, and output formats remain unchanged from the initial draft.

The version 0.1 values are frozen for reproducible baseline work, not asserted to be a final physical defect model. Changing the input restrictions, crop rule, normalization, defect arithmetic, distributions, counts, seed, placement rule, output encoding, CSV schema, or acceptance criteria requires an explicit contract revision before implementation results can claim conformance to the new revision. Experimental overrides are allowed, but their outputs shall be labeled with the actual configuration and shall not be reported as version 0.1 results.

Version 0.2 records the accepted single-frame scope decision without rewriting the historical version 0.1 profile. New tuning data containing stuck assignments is non-conforming. The current injector source still implements version 0.1 and must be revised or given an explicit hot/dead-only profile before it generates new tuning datasets.

## 11. Current RAW10 BLC Pipeline Profile

The accepted integration order is decoded Bayer, RGGB crop/RAW10 normalization,
hot/dead injection, BLC, then BPC. The reference branch omits injection but uses
the same BLC coefficients. This supports the actual BLC-before-BPC application
without requiring the source BlackLevel to be zero.

This profile retains 150 hot and 150 dead assignments and the Section 6 spacing,
but scales the hot offset from RAW12 `[256,2048]` to RAW10 `[64,512]`. The dead
gain remains `[0,0.75)`. It uses no stuck assignments. The one-image smoke experiment retains
seed 0 and records it; this is not the image-identity split experiment in the
evaluation methodology. No full-corpus compatibility claim is made.

The loader accepts explicitly parsed scalar or at most 2 x 2 repeating DNG black
levels. It maps the repeating pattern relative to the visible/active-area origin
through the RGGB-aligned crop. Fractional black levels use the DNG floating fields;
integer mirrors must not be added again. Pixels and Black Levels are scaled by
`1023/WL` and
rounded half-up, just like pixels, to obtain R/Gr/Gb/B integer coefficients.
The loader never subtracts black. BLC performs the sole saturating subtraction;
there is no rescale after subtraction.

The previous Bayer, pitch, visible-area and orientation restrictions remain.
The accepted WhiteLevel range is `[1023,65535]`; every applicable entry must
match. BlackLevelDeltaH/V tags are conservatively rejected, including zero-valued
tags, because the current profile does not decode their arrays. DNG linearization is performed by LibRaw unpack (its DNG decoder applies the
curve when copying decoded samples); the loader must not apply it twice. Explicit
opcode processing is outside this profile. These input
limits belong to the loader, not to the valid-input BLC/BPC algorithms.

Implementation evidence for the installed LibRaw 0.22.2: its
[DNG decoder](https://github.com/LibRaw/LibRaw/blob/0.22.2/src/decoders/dng.cpp)
applies the linearization curve, while its
[TIFF parser](https://github.com/LibRaw/LibRaw/blob/0.22.2/src/metadata/tiff.cpp)
averages BlackLevelDeltaH/V into scalar fields. This is why delta-tag presence
is checked before using those fields for a four-phase model.

The CLI remains `defect_injector [--check-only] image.dng`. Run it in a separate
artifact directory. Check-only performs loading and metadata validation without
writing files. A normal run writes the three historical pre-BLC files plus
`reference_blc.pgm`, `corrupted_blc.pgm`, and `input_metadata.json` containing the
source path, LibRaw version, crop origin, WhiteLevel, native and RAW10 four-phase
black levels, `bit_depth=10`, `pixel_max=1023`, and injection profile. All PGM
outputs use `maxval=1023` and two-byte big-endian samples. BPC evaluation reads the two post-BLC files;
the CSV coordinates are unchanged but its values remain explicitly pre-BLC.

Acceptance: nonzero scalar and phase-specific metadata map correctly through
crop and scale; pre-BLC files preserve the pedestal; post-BLC files equal direct
BLC output; repeated runs reproduce artifacts; only hot/dead rows are emitted.
