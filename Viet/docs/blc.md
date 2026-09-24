<!--
Project: Adaptive Directional BPC and BLC
Module: Black Level Correction
Description: Define the host-side Black Level Correction reference behavior and pipeline boundary.
Author: Viet Nguyen To Quoc
-->

# Black Level Correction

## 1. Purpose and Authority

This document is the authoritative behavioral contract for the host-side C++ Black Level Correction reference model.

It defines intended behavior. It does not claim that the current implementation is complete, optimized for HLS, or integrated with AXI interfaces.

## 2. Input and Configuration

The input is one row-major RAW Bayer frame:

```text
Format            : RGGB RAW10
Storage           : unsigned 16-bit container
Valid sample range: [0, 1023]
System resolution : 1920 x 1080
CFA origin        : coordinate (row=0, col=0) is R
```

The host reference interface accepts any non-zero `width` and `height` so small deterministic frames can be tested. System-level use remains 1920 x 1080.

The caller provides one Black Level for each RGGB phase:

```cpp
struct BlcConfig {
    std::uint16_t black_level_r;
    std::uint16_t black_level_gr;
    std::uint16_t black_level_gb;
    std::uint16_t black_level_b;
};
```

Every configured Black Level shall be in `[0, 1023]` and expressed in the same RAW10 numeric domain as the input samples. BLC does not estimate these values or determine whether they came from metadata, factory calibration, a dark frame, or another calibration process.

## 3. CFA-Phase Selection

Coordinates are zero-based. Select the Black Level from row and column parity:

```text
even row, even column : black_level_r
even row, odd column  : black_level_gr
odd row,  even column : black_level_gb
odd row,  odd column  : black_level_b
```

`Gr` and `Gb` remain independent. The reference model shall not combine them into one green coefficient.

## 4. Pixel Correction

For input sample `X` and the Black Level selected for its CFA phase:

```text
Y = max(X - BL_CFA, 0)
```

The implementation shall compare `X` and `BL_CFA` before unsigned subtraction:

```text
if X > BL_CFA: Y = X - BL_CFA
otherwise    : Y = 0
```

This rule covers all saturation boundaries:

```text
X = BL_CFA - 1 : Y = 0
X = BL_CFA     : Y = 0
X = BL_CFA + 1 : Y = 1
```

Because correction only subtracts from a valid RAW10 input, `Y` cannot exceed `1023`; no upper clamp is required. `BL_CFA=0` follows the same equation and is an exact pass-through. It shall not use a different algorithm.

Pixel correction uses integer comparison and subtraction only. It requires no floating-point arithmetic, multiplication, division, neighborhood window, or line buffer.

## 5. Full-Frame Processing

For `blc_frame(input, output, width, height, config)`:

- `width` and `height` shall both be non-zero;
- calculating `width * height` shall not overflow `std::size_t`;
- `input.size()` shall equal `width * height`;
- every input sample and configured Black Level shall satisfy the RAW10 range;
- every coordinate shall be corrected with the pixel rule in Section 4;
- `output` shall contain exactly `width * height` row-major samples.

These are caller preconditions. The algorithm trusts valid input and does not perform runtime rejection checks. Tests and the system pipeline use distinct input and output vectors; in-place behavior is outside the current contract.

BLC has no neighborhood and therefore no border exception. Every pixel, including the outer rows and columns, is processed.

## 6. Output Contract

The output has the same dimensions and CFA alignment as the input:

```text
Format     : RGGB RAW10
Range      : [0, 1023]
CFA origin : coordinate (row=0, col=0) is R
```

The systematic Black Level pedestal is shifted toward zero. Residual sensor or read noise around black is allowed; BLC does not require every physically dark pixel to become exactly zero.

Bad pixels remain present after BLC, although their numeric values may change. BLC performs neither bad-pixel detection nor denoising.

No white-level normalization or multiplicative rescaling is applied. For example:

```text
X = 1023, BL_CFA = 53 : Y = 970
```

Mapping `970` back to `1023` would be a separate RAW gain or white-level normalization contract.

## 7. Defect-Injector Boundary

The system order is:

```text
RAW with original/configured Black Level
    -> RAW10 normalization when required by the input contract
    -> defect injection
    -> BLC
    -> BPC
```

The injector does not subtract Black Level. Defects are injected after the documented RAW10 normalization and therefore remain in the pre-BLC RAW10 domain.

BLC processes an injected sample exactly like any other input sample. With `BL_CFA=53`:

```text
injected sample 0    : BLC output 0
injected sample 1023 : BLC output 970
```

BLC shall not restore the second value to `1023` after subtraction.

Consequently, BPC receives the post-BLC values of injected defects; it shall not assume that pre-BLC extremes still equal exactly `0` or `1023`.

## 8. BPC Boundary

Both the fixed-threshold baseline and adaptive directional BPC consume post-BLC RAW10. Neither receives nor compensates for the original Black Level.

The fixed-threshold detector is invariant to a common additive pedestal where subtraction does not saturate because it uses relative differences. The adaptive detector is not fully pedestal-invariant because its threshold contains the absolute directional prediction term `P`. This distinction does not change the pipeline order and does not authorize a BPC algorithm change.

## 9. Acceptance Criteria

The C++ reference implementation is conforming when directed tests demonstrate:

1. Four zero Black Levels preserve arbitrary valid RAW10 samples exactly.
2. With a scalar Black Level of `53`, `53 -> 0`, `75 -> 22`, and `1023 -> 970`.
3. `BL_CFA-1`, `BL_CFA`, and `BL_CFA+1` produce `0`, `0`, and `1` for each of `R`, `Gr`, `Gb`, and `B`.
4. Distinct values such as `R=64`, `Gr=66`, `Gb=65`, and `B=68` are selected at the correct RGGB coordinates.
5. A small full frame produces the manually predicted row-major result at every pixel.
6. Pre-BLC defect extremes follow the normal correction rule: `0 -> 0` and `1023 -> 970` when `BL_CFA=53`.
7. Tests supply valid RAW10 samples, Black Levels, dimensions, and frame sizes as required by Section 5.
8. The resulting frame is accepted by both existing BPC reference interfaces as post-BLC RAW10.

When a compatible dataset path is available, run one DNG whose metadata reports zero Black Level and one reporting non-zero Black Level if such a file is actually available. Missing metadata or unavailable files shall be reported rather than inferred.

## 10. Current Limitations

The loader supports the bounded scalar/four-phase BlackLevel profile in [defect-injection.md](defect-injection.md#11-current-raw10-blc-pipeline-profile). Metadata extraction, crop mapping and RAW10 coefficient scaling belong to that loader.

Optical-black estimation, dark-frame calibration, factory calibration, row/column correction, gain- or temperature-dependent estimation, white-level normalization, HLS optimization, AXI4-Stream, AXI4-Lite register mapping, and FPGA integration are outside this reference-model contract.
