<!--
Project: Adaptive Directional BPC and BLC
Module: Adaptive Directional BPC
Description: Define the adaptive directional BPC behavior and streaming HLS architecture.
Author: Viet Nguyen To Quoc
-->

# Adaptive Directional BPC

## 1. Purpose

This document defines the experimental `adaptive_v2` directional BPC reference model for detecting isolated hot and dead pixels. Compared with the historical range-based detector, v2 measures the center directly against the selected directional prediction.

The tie-break follows the existing implementation: H, then V, then D1, then D2. Initial configuration values remain experimental.

## 2. Input and Configuration

The input format remains unchanged from the fixed-threshold baseline:

```text
Format     : RGGB RAW10
Resolution : 1920 x 1080
Range      : [0, 1023]
Window     : 5 x 5
```

The configurable parameters are:

```text
T0_R, T0_G, T0_B : base threshold for each CFA phase
k_s              : signal-level shift
k_a              : local-activity shift
```

Both green phases use `T0_G`.

The evaluation ranges are `T0_CFA` in `[0, 1023]` and `k_s`, `k_a` in
`[0, 10]`. The shift controls remain 4-bit hardware fields. Shift `10` is the
canonical zero-term endpoint for RAW10 because every valid sample shifted by
10 is zero; values 11 through 15 are representable but redundant and are not
searched.

The accepted RAW10 development operating point is Candidate 3:

```text
T0_R = 4, T0_G = 8, T0_B = 4, k_s = 3, k_a = 0
```

This is a tuning decision for the current Adaptive BPC implementation, not a
universal sensor calibration. Its selection rationale and held-out evidence
are recorded in the [RAW10 evaluation methodology](bpc-evaluation-methodology.md#current-raw10-operating-point).

## 3. Same-CFA Neighborhood

Interior pixels use the same eight neighbors as the baseline:

```text
UL          U          UR
      5 x 5 window
L         center        R
DL          D          DR
```

The four directional pairs are:

```text
H  : L  and R
V  : U  and D
D1 : UL and DR
D2 : UR and DL
```

The center pixel is excluded from all neighborhood statistics.

## 4. Directional Analysis

For each direction `d`:

```text
A_d = abs(N_d_minus - N_d_plus)
```

The smoothest direction is the direction with the smallest activity:

```text
d_selected = argmin(A_H, A_V, A_D1, A_D2)
```

The directional prediction is calculated from the selected pair:

```text
P = (N_d_minus + N_d_plus) >> 1
```

The addition uses a wide enough intermediate type to avoid overflow. The right shift gives a floor average.

If multiple directions have the same minimum activity, prefer H, then V, then D1, then D2. This records the existing strict-less-than selection order; it does not change the algorithm.

## 5. Adaptive Threshold

The minimum directional activity is the activity of the selected direction:

```text
G_min = min(A_H, A_V, A_D1, A_D2) = A_d_selected
```

The adaptive threshold is:

```text
T = T0_CFA + (P >> k_s) + (G_min >> k_a)
```

`T0_CFA` is selected from `T0_R`, `T0_G`, or `T0_B` using the RGGB phase.

Threshold calculation uses wide intermediate arithmetic.

## 6. Detection and Correction

For center pixel `X`, the pixel is detected as defective when:

```text
abs(X - P) > T
```

The comparison is strict: equality with `T` is not a detection.

All additions and comparisons use wide enough intermediate types to avoid overflow.

The output is:

```text
detected pixel     : P
non-detected pixel : X
```

## 7. Border Handling

The outer two rows and columns are copied unchanged because they do not have a complete 5 x 5 neighborhood.

Border pixels are not reported as detections.

## 8. Output

Full-frame processing returns:

- a corrected frame with the same dimensions as the input;
- detection records containing row, column, original value, and replacement value.

Input and output buffers must be different objects.

## 9. Difference from Baseline

| Processing point | Fixed-threshold baseline | Adaptive directional BPC |
|---|---|---|
| Input | RGGB RAW10 | unchanged |
| Window | Eight same-CFA neighbors | unchanged |
| Local analysis | Sorted median | directional activity and prediction |
| Threshold | Fixed for each CFA phase | signal- and activity-dependent |
| Detection | Distance from median | distance from directional prediction |
| Replacement | Median | directional prediction |
| Border and output | Pass-through and detection records | unchanged |

## 10. Limitations

The algorithm targets isolated spatial high and low outliers represented by synthetic hot and dead corruption. These names describe behavior observable in one Bayer frame; they do not establish the physical defect type of a sensor coordinate.

A physical stuck pixel may be corrected when it happens to appear as a spatial high or low outlier. The algorithm does not identify or classify it as stuck. Reliable stuck-pixel identification requires multiple RAW frames from the same physical sensor at the same sensor coordinates and is outside the current scope.

Synthetic stuck corruption shall not be used for training, validation, testing, or performance claims under this single-frame contract. Defect clusters, temporal detection, calibration state, and persistent defect maps are also outside the current scope. This scope decision is recorded in [ADR 0001](adr/0001-exclude-stuck-from-single-frame-bpc.md).

## 11. HLS Streaming Architecture

The [BPC streaming contract](bpc_streaming_contract.md), accepted on 2026-09-28, owns the free-running variant. It carries forward the indefinite-frame, four-bank/window, real/synthetic advance, overlap, drain and backpressure requirements previously recorded here, and defines the BLC-derived wrapper/configuration boundary and verification gates. The finite-frame variant is governed separately by the [BPC handshake contract](bpc_handshake_contract.md), accepted on 2026-09-30. Sections 1–10 of this document continue to own the shared algorithm.

See [ADR 0003](adr/0003-reuse-blc-boundaries-for-bpc.md) for the boundary decision and [verification-status.md](verification-status.md) for observed results. The archived finite-frame implementation at `b55eb38` remains historical evidence; [ADR 0002](adr/0002-overlap-bpc-frame-tail.md) records its rationale and limits. BPC streaming implementation and acceptance evidence remain pending.
