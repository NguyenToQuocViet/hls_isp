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

### Rebuild baseline and target

This rebuild branch retains the HLS `blc_pixel()` and `bpc_pixel()` algorithms and the reference models; it has no streaming HLS worker or top yet. The former one-frame tops and finite `isp_top_frames` experiment are preserved at commit `b55eb38` on branch `archive/bpc-agent-hls-2026-09-26`. They are historical implementations, **not** the target video-stream contract. Standalone block entry points may be rebuilt for verification; the production streaming datapath must not require a future frame count or a restart between frames. The finite-frame experiment and its limits are recorded in [ADR 0002](adr/0002-overlap-bpc-frame-tail.md).

The target consumes and emits one RAW10 pixel per real image pixel, in row-major order, for an indefinite sequence of fixed-size RGGB frames. `TUSER` denotes SOF on the first pixel and `TLAST` denotes EOL on the last pixel of every row; there is no separate EOF. A handshaken EOL on row `FRAME_HEIGHT - 1` ends the input frame. The stream must contain exactly `FRAME_WIDTH` real pixels per row and `FRAME_HEIGHT` rows per frame. How malformed sidebands are handled remains an interface decision before implementation; they must not silently redefine image geometry.

### Storage and three progress domains

The algorithm and arithmetic in Sections 3–7 remain unchanged. The target retains four independent `FRAME_WIDTH` line-buffer banks, a `horizontal[3][5]` window, and the sparse same-CFA 3 × 3 computing window. Each committed internal advance reads old bank values, writes the new token and shifted history, then shifts/loads the horizontal window. The center is evaluated after the shift/load. The center delay in **committed internal advances** is `CENTER_DELAY = 2 * FRAME_WIDTH + 2` (3842 at 1920 pixels); it is not a fixed cycle latency under stalls.

Keep these domains separate:

| Domain | Advances on | Purpose |
|---|---|---|
| Logical input `in_row/in_col` | Real input handshake only | SOF/EOL, BLC CFA phase, input geometry |
| Physical `lb_addr` | Committed internal advance with real or synthetic token | Circular line-buffer access; wraps modulo `FRAME_WIDTH` |
| Logical output `out_row/out_col` | Valid real output handshake only | BPC border/CFA phase, output SOF/EOL, frame retirement |

SOF resets logical input coordinates to `(0,0)`; it does not clear the banks/window, reset `lb_addr`, or force a new full warmup. EOL advances the logical input row. `lb_addr` is never forced to the logical input column: after a gap of `g` synthetic advances, the next frame's column zero starts at the current physical address. Every full real row has `FRAME_WIDTH` pixels, so the same logical column in successive rows of that frame reaches the same physical bank address. Freeze all three domains as applicable when their enabling handshake/advance does not occur.

### Between-frame drain and valid centers

After the last real pixel of a frame, up to `CENTER_DELAY` further committed advances are needed to expose its remaining centers. All such centers belong to the bottom two rows or the final two columns and therefore bypass `bpc_pixel`; the delayed center value must still be preserved exactly. While those centers remain and the next real SOF is unavailable, inject synthetic zero tokens **only between frames**. A real next-frame pixel has priority over a synthetic token as soon as it can be accepted, subject to normal downstream flow control. It may enter while the previous frame's border tail is still leaving. With no input gap and no backpressure, there must be no architecture-imposed frame-boundary acceptance bubble.

Within an active input frame, an upstream stall freezes the BPC storage: no synthetic insertion, window shift, or `lb_addr` advance. Once the previous frame's last center has been exposed, synthetic advancement is no longer needed. When that last valid output handshakes, the frame is retired; if no next frame is active, remain idle until its SOF. An output buffer may cause exposure and retirement to occur at different times, so control must track both safely. A pending output must remain stable under backpressure, and storage may advance only if the output can be retained without loss or duplication.

Synthetic tokens never count as input pixels and never produce output beats. A gap can put synthetic centers between the final center of one frame and the first center of the next. Track token validity/ownership or equivalent bounded control so these positions are suppressed while all real centers are emitted exactly once and in order. The output coordinate alone cannot identify a synthetic position; it only identifies the next **valid** output coordinate. In particular, do not advance the output coordinate to skip a synthetic position. A long gap may leave synthetic history in the pipeline; the next frame then takes as many committed advances as its actual history requires before its first valid center appears. Do not reset a fixed warmup at each SOF.

For each valid center, the logical output coordinates determine the outer-two-row/column border bypass, CFA phase, `TUSER` at `(0,0)`, and `TLAST` at column `FRAME_WIDTH - 1`. `bpc_pixel()` runs only for valid interior centers. Synthetic positions emit no payload or sidebands. An implementation with output buffering may need separate generated-center and retired-output state; the externally visible `out_row/out_col` advances only on the AXI output handshake. Input acceptance, storage update, output buffering, and output retirement must remain transactionally consistent under stalls.

### System boundary and acceptance evidence

The production BLC worker, AXI adapters, and composite `DATAFLOW` top must also run for an unknown number of frames and preserve SOF/EOL. The HLS block-control style and safe configuration-update rule must be selected and documented before implementing those modules; the archived AXI4-Lite `ap_ctrl_hs` invocation and `frame_count` cannot by themselves provide this behavior. Configuration must be stable for every frame being processed, including its delayed output tail. The required observable behavior does not mandate a particular FSM, nonblocking-read primitive, or per-pixel valid-bit implementation.

Verification must compare output data and sidebands with independent, per-frame BLC→BPC reference results and check `real input beats = valid output beats` after drain. Cover zero gap, short gap, arrival just before tail completion, gap longer than drain, an in-frame input stall, output backpressure, synthetic centers reaching the physical center, and many frames with mixed gaps. Check both CSim and generated-RTL CoSim handshakes; inspect synthesis for II, memory-port scheduling, and control compatibility. `II=1` is a target for a progressing pixel loop, not proof of zero inter-frame bubble, backpressure correctness, or indefinite operation.
