<!--
Project: Adaptive Directional BPC and BLC
Module: Adaptive Directional BPC
Description: Define the adaptive directional BPC reference behavior.
Author: Viet Nguyen To Quoc
-->

# Adaptive Directional BPC

## 1. Purpose

This document defines the experimental `adaptive_v2` directional BPC reference model for detecting isolated hot and dead pixels. Compared with the historical range-based detector, v2 measures the center directly against the selected directional prediction.

The tie-break follows the existing implementation: H, then V, then D1, then D2. Initial configuration values remain experimental.

## 2. Input and Configuration

The input format remains unchanged from the fixed-threshold baseline:

```text
Format     : RGGB RAW12
Resolution : 1920 x 1080
Range      : [0, 4095]
Window     : 5 x 5
```

The configurable parameters are:

```text
T0_R, T0_G, T0_B : base threshold for each CFA phase
k_s              : signal-level shift
k_a              : local-activity shift
```

Both green phases use `T0_G`.

The evaluation ranges are T0_CFA in [0, 4095] and k_s, k_a in [0, 12]. Initial values are not calibrated.

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
| Input | RGGB RAW12 | unchanged |
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

This document defines the C++ reference behavior. AXI4-Stream, line buffers, pipeline stages, `II=1`, and AXI4-Lite belong to the later HLS architecture contract.
