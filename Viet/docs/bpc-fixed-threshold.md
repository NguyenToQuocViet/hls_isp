<!--
Project: Adaptive Directional BPC and BLC
Module: Fixed-Threshold BPC Baseline
Description: Define the fixed-threshold BPC baseline behavior.
Author: Viet Nguyen To Quoc
Version: 0.1
-->

# Fixed-Threshold BPC Baseline

## 1. Purpose

This document defines the fixed-threshold BPC reference model used as the baseline for later evaluation.

## 2. Input and Configuration

The input is one row-major RAW Bayer frame:

```text
Format     : RGGB RAW10
Resolution : 1920 x 1080
Range      : [0, 1023]
```

The caller provides one fixed threshold for each color phase:

```cpp
struct BpcConfig {
    std::uint16_t threshold_r;
    std::uint16_t threshold_g;
    std::uint16_t threshold_b;
};
```

Both green phases use `threshold_g`.

## 3. Same-CFA Neighborhood

For each center pixel, the baseline reads eight same-CFA neighbors from a 5 x 5 window:

```text
(-2, -2)  (-2,  0)  (-2, +2)
( 0, -2)  center    ( 0, +2)
(+2, -2)  (+2,  0)  (+2, +2)
```

The center pixel is excluded from the neighborhood.

## 4. Median

The eight neighbors are sorted in ascending order.

The median is the floor average of the two middle samples:

```text
median = (sorted[3] + sorted[4]) >> 1
```

The addition uses a wide enough intermediate type to avoid overflow.

## 5. Threshold Selection

For the RGGB pattern:

```text
even row, even column : threshold_r
even row, odd column  : threshold_g
odd row,  even column : threshold_g
odd row,  odd column  : threshold_b
```

## 6. Detection and Correction

For center sample `X`, median `M`, and selected threshold `T`:

```text
difference = abs(X - M)
```

The pixel is detected and corrected only when:

```text
difference > T
```

The output is:

```text
detected pixel     : median
non-detected pixel : original center value
```

## 7. Border Handling

The outer two rows and columns are copied unchanged because they do not have a complete 5 x 5 neighborhood.

Border pixels are not reported as detections.

## 8. Output

Full-frame processing returns:

- a corrected frame with the same dimensions as the input;
- detection records containing row, column, original value, and replacement value.

Input and output buffers must be different objects.

## 9. Limitations

This baseline uses fixed thresholds and a non-directional median replacement. It does not account for local activity, edge direction, texture, or signal-dependent noise.

It is a host-side C++ reference model and does not define an HLS architecture.
