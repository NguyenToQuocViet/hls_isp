<!--
Project: Adaptive Directional BPC and BLC
Module: Single-Frame Defect Scope Decision
Description: Record why temporal stuck pixels are excluded from single-frame BPC tuning and claims.
Author: Viet Nguyen To Quoc
Version: 0.1
-->

# Exclude stuck pixels from single-frame BPC scope

Status: accepted

The project targets isolated spatial high and low outliers that are observable from one RAW Bayer frame. A physical stuck pixel cannot be identified as stuck from one frame: when its fixed value is high or low relative to the local same-CFA neighborhood, it is only observable as a high or low spatial outlier; when its value is locally plausible, the available frame contains no evidence that distinguishes it from valid image content.

All new single-frame BPC training, validation, and test datasets shall therefore inject only hot and dead corruption. Stuck injection shall not contribute to tuning objectives, diagnostic metrics, algorithm requirements, or performance claims. The BPC output shall report detection and replacement, not a physical defect type.

Reliable stuck-pixel identification requires multiple RAW frames from the same physical sensor, aligned by sensor coordinate and captured under changing scene or illumination conditions. Images from unrelated cameras do not provide this evidence. Temporal detection, calibration state, and persistent defect maps are outside the current adaptive BPC and HLS scope.

The version 0.1 injector and Week-3 report remain historical evidence and retain their 25 stuck samples for reproducibility. They shall not be treated as conforming datasets for new single-frame tuning. A future injector revision used for tuning shall set the stuck count to zero while retaining the legacy implementation until the old experiment no longer needs to be reproduced.
