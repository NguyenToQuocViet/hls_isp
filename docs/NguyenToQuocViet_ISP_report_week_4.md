<!--
Project: Adaptive Directional BPC and BLC
Module: Week 4 ISP Progress Report
Description: Summarize BLC integration, adaptive BPC, and FiveK tuning and test results.
Author: Viet Nguyen To Quoc
-->

# Báo cáo tiến độ Week 2 (Week 4 ISP)

## Mục lục

1. [Tổng quan](#1-tổng-quan)
2. [BLC](#2-blc)
3. [Adaptive Directional BPC](#3-adaptive-directional-bpc)
4. [Metric đánh giá](#4-metric-đánh-giá)
5. [Flow](#5-flow)
6. [Kết quả sau tuning và test](#6-kết-quả-sau-tuning-và-test)

## 1. Tổng quan

| Hạng mục | Status |
|---|---|
| BLC | Hoàn thiện C++ reference model, tích hợp trước BPC |
| Adaptive Directional BPC | Hoàn thiện C++ reference model |
| Defect injector | Hỗ trợ Black Level metadata và flow BLC → BPC; chỉ inject hot/dead pixel |
| Script tuning và đánh giá | Hoàn thiện scan, split, sweep, chọn config chung và test |
| So sánh baseline và adaptive | Đã tuning trên 883 ảnh và đánh giá trên 221 ảnh test |
| HLS | Chưa có kết quả synthesis về resource utilization, latency hoặc II |

## 2. BLC

BLC trừ Black Level theo từng CFA phase R/Gr/Gb/B và clamp output về 0:

```text
output = max(input − BL_phase, 0)
```

C++ reference model đã hoàn thiện và được tích hợp trước BPC. Injector đọc Black Level từ metadata, map theo crop offset thành 4 hệ số tương ứng.

## 3. Adaptive Directional BPC

Thuật toán dùng window 5 × 5, lấy 8 same-CFA neighbor theo 4 hướng: horizontal, vertical và 2 diagonal.

Với mỗi hướng, tính absolute difference giữa 2 neighbor đối diện. Chọn hướng có difference nhỏ nhất và lấy average của cặp neighbor đó làm prediction:

```text
d* = argmin |pixel_d1 − pixel_d2|
P  = floor((pixel_d*1 + pixel_d*2) / 2)
```

Adaptive threshold phụ thuộc vào signal level và neighbor range:

```text
R_N = N_max − N_min
T   = T0_CFA + (P >> k_s) + (R_N >> k_a)
```

Trong đó:

- `N_max`, `N_min`: maximum và minimum của 8 neighbor.
- `T0_CFA`: base threshold cho R/G/B.
- `k_s`, `k_a`: điều chỉnh signal term và activity term bằng phép dịch bit.

Điều kiện detection và correction:

```text
Nếu center > N_max + T hoặc center + T < N_min:
    output = P
Ngược lại:
    output = center
```

So với baseline dùng median và fixed threshold, adaptive điều chỉnh threshold theo local content và chọn replacement theo hướng có activity nhỏ nhất. Border 2 pixel được giữ nguyên.

## 4. Metric đánh giá

Quy ước:

- `R`, `C`: reference frame và corrupted frame, đều đã qua BLC.
- `O_R`, `O_C`: BPC output tương ứng.
- `D_q`: các vị trí được inject defect loại `q` — hot hoặc dead.
- `V`: valid region, loại border 2 pixel.
- `U`: các vị trí trong `V` không được inject defect.

### Restoration Gain — RG và BRG

```text
E_initial_q  = Σ |C[i] − R[i]|       với i ∈ D_q
E_residual_q = Σ |O_C[i] − R[i]|     với i ∈ D_q

RG_q = 1 − E_residual_q / E_initial_q
BRG  = (RG_hot + RG_dead) / 2
```

RG đo mức giảm error tại các vị trí được inject defect: 1 là khôi phục hoàn toàn, 0 là không cải thiện, âm là error tăng. BRG cho hot và dead trọng số bằng nhau; cao hơn tốt hơn.

### Background Correction Rate — BCR

```text
BCR = count(O_R[i] ≠ R[i], i ∈ V) / |V|
```

BCR đo tỷ lệ pixel bị thay đổi trên reference frame. Metric này thể hiện mức background correction, không mặc định là false correction rate vì reference frame có thể chứa defect tự nhiên.

### Injection-induced Off-target Correction Rate — IOTCR

```text
IOTCR = count(O_C[i] ≠ O_R[i], i ∈ U) / |U|
```

IOTCR đo tỷ lệ output thay đổi ngoài các vị trí được inject defect do ảnh hưởng của defect injection. Thấp hơn tốt hơn.

Các metric được tính riêng từng ảnh rồi lấy mean, mỗi ảnh có trọng số bằng nhau.

## 5. Flow

```text
FiveK DNG → LibRaw → Crop RGGB 1920 × 1080 → RAW12
                                              |
                       +----------------------+------------------+
                       |                                         |
                 Reference frame                          Inject hot/dead
                       |                                         |
                      BLC                                       BLC
                       |                                         |
                       R                                         C
                       |                                         |
                      BPC                                       BPC
                       |                                         |
                      O_R                                       O_C
                       +--------------------+--------------------+
                                            |
                                Evaluate với R và defects.csv
```

Mỗi ảnh được inject 150 hot và 150 dead pixel với fixed seed. Stuck pixel được loại khỏi tuning vì đặc tính cố định giá trị theo thời gian không thể xác định từ 1 frame.

Flow tuning và test:

```text
Compatibility scan → Tuning/test split
    → Sweep từng ảnh tuning, tối đa 75.942 config trước clipping/dedup
    → Chọn config chung trên tuning set
    → Freeze config, chạy test set
    → Xuất thống kê
```

## 6. Kết quả sau tuning và test

### Dataset và tuning

| Hạng mục | Kết quả |
|---|---:|
| Ảnh FiveK được scan | 5.000 |
| Ảnh compatible với injector | 1.104 |
| Tuning set | 883 |
| Test set | 221 |
| Common adaptive candidate pool | 835 config |

Baseline được sweep shared threshold R/G/B từ 0 đến 4095, chọn mean BRG cao nhất. Adaptive được chọn từ candidate pool gồm các config tốt nhất từng ảnh, tối đa hóa mean BRG với mean BCR và IOTCR không vượt baseline.

| Config được chọn | Giá trị |
|---|---|
| Baseline | `T = 0` |
| Adaptive | `T0_R = T0_G = T0_B = 16`, `k_s = 6`, `k_a = 7` |

Baseline chọn threshold 0 vì objective là tối đa hóa mean BRG; tại threshold này, mọi pixel khác median trong valid region đều được thay bằng median.

### Kết quả test

Mean trên 221 ảnh, biểu diễn theo phần trăm:

| Metric | Baseline | Adaptive |
|---|---:|---:|
| RG_hot | 97,9545% | 97,4985% |
| RG_dead | 91,1178% | 87,7025% |
| BRG | 94,5362% | 92,6005% |
| BCR | 93,6647% | 1,4311% |
| IOTCR | 0,064815% | 0,001336% |

Adaptive thấp hơn baseline **1,94 điểm phần trăm BRG**, nhưng giảm **98,47% BCR** và **97,94% IOTCR**. Lợi ích chính trong thí nghiệm này là giảm mạnh background correction và injection-induced off-target correction, trong khi vẫn đạt BRG 92,60%.

Kết quả giới hạn ở compatible subset và synthetic defect. Split theo ảnh chưa kiểm tra near-duplicate; adaptive candidate pool chưa bảo đảm global optimum. Chưa có kết quả HLS về resource utilization, latency hoặc II.
