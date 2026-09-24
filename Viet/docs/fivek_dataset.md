<!--
Project: Adaptive Directional BPC and BLC
Module: FiveK RAW10 Dataset
Description: Summarize FiveK qualification, split, and synthetic defect inputs used for tuning and test.
Author: Viet Nguyen To Quoc
-->

# FiveK RAW10 Dataset

## Dataset và split

| Hạng mục | Số lượng |
|---|---:|
| MIT-Adobe FiveK DNG được scan | 5.000 |
| Ảnh compatible | 1.104 |
| Tuning (train) | 883 |
| Held-out test | 221 |

Split dùng seed `20260907`, xấp xỉ 80% tuning và 20% test. Mỗi ảnh compatible có injection seed cố định trong `split.json`; tuning và test dùng lại đúng seed này để kết quả lặp lại được.

## Điều kiện lọc ảnh

Ảnh được giữ khi có Bayer CFA 2 × 2 chuẩn, orientation không cần transform, visible RAW area đủ để crop 1920 × 1080 và crop có thể căn về RGGB. DNG phải có WhiteLevel hợp lệ, đồng nhất giữa các entry và nằm trong `[1023, 65535]`; BlackLevel phải là scalar hoặc pattern tối đa 2 × 2. Các file có `BlackLevelDeltaH/V`, CFA không hỗ trợ, orientation không hỗ trợ hoặc metadata không hợp lệ bị loại.

Ảnh compatible được crop RGGB 1920 × 1080 và chuẩn hóa về RAW10 `[0, 1023]`. Flow dữ liệu là:

```text
FiveK DNG → RGGB RAW10 → inject hot/dead → BLC → BPC → evaluate
```

Mỗi ảnh inject 150 hot pixel và 150 dead pixel; không dùng stuck pixel. Manifest scan nằm tại `artifacts/fivek_scan/manifest.json`, còn split RAW10 nằm tại `artifacts/adaptive_v2_raw10/split/split.json`.
