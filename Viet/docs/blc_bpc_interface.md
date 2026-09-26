<!--
Project: Adaptive Directional BPC and BLC
Module: BLC and BPC RAW10 Interface
Description: Summarize the RAW10 stream interface and BLC-to-BPC processing boundary.
Author: Viet Nguyen To Quoc
-->

# BLC và BPC RAW10 Interface

## Data stream

| Hạng mục | Quy ước |
|---|---|
| Frame | 1920 × 1080, row-major RGGB |
| CFA origin | Pixel `(0,0)` là R |
| Pixel | RAW10 unsigned, `[0, 1023]` |
| AXI4-Stream word | `ap_axiu<16,1,0,0>` |
| Payload | `TDATA[9:0]` là pixel RAW10; bit `[15:10]` bằng 0 |
| Sideband | `TUSER` đánh dấu SOF tại `(0,0)`; `TLAST` đánh dấu pixel cuối mỗi dòng |

## BLC

Input BLC là RAW10 trước Black Level Correction. Bốn hệ số `bl_r`, `bl_gr`, `bl_gb`, `bl_b` cùng miền RAW10.

```text
Y = max(X - BL_CFA, 0)
```

Output BLC giữ nguyên kích thước và CFA alignment; SOF/EOL được tạo từ tọa độ nội bộ. Với input hợp lệ, sideband output giống input. Output này là input của BPC.

`blc_engine` khởi tạo tọa độ `(0,0)` và chỉ bắt đầu mỗi frame khi packet tại vị trí chờ này có `user = 1`. Packet có `user = 0` trong lúc chờ bị đọc bỏ, không tạo output và không làm tiến tọa độ. Packet SOF được xử lý ngay tại `(0,0)`. Trong frame, `user` đến thêm bị bỏ qua và `last` không điều khiển bộ đếm. Sau mỗi pixel được xử lý và ghi output, bộ đếm tăng cột, wrap cuối dòng và wrap về `(0,0)` sau đúng `FRAME_WIDTH * FRAME_HEIGHT` pixel để chờ SOF tiếp theo. Không lưu cờ lỗi sideband. Cơ chế này giữ hình học cố định; không phục hồi frame bị mất hoặc chèn pixel giữa frame. Cấu hình phải ổn định trong suốt frame đang xử lý.

## Adaptive Directional BPC

BPC nhận RAW10 sau BLC trên cùng stream format. Các tham số thuật toán là `T0_R`, `T0_G`, `T0_B`, `k_s`, `k_a`; hai phase green dùng chung `T0_G`.

```text
T = T0_CFA + (P >> k_s) + (G_min >> k_a)
detect khi abs(X - P) > T
```

Operating point hiện chọn là `(T0_R, T0_G, T0_B, k_s, k_a) = (4, 8, 4, 3, 0)`. Streaming BPC cần trả về một pixel RAW10 cho mỗi pixel input và tạo sideband theo tọa độ output.

## Streaming HLS target

Branch rebuild chỉ giữ hàm pixel BLC/BPC trong HLS; worker streaming, adapter và top cũ đã được lưu ở branch `archive/bpc-agent-hls-2026-09-26` (commit `b55eb38`). Hiện chưa có top HLS để tổng hợp hoặc kiểm tra CoSim trên branch này.

Top đích cần xử lý luồng frame không biết trước số lượng, giữ định dạng AXI4-Stream ở bảng trên và nối BLC → BPC. Quy tắc nhận frame, drain, backpressure và phát sideband nằm trong [contract streaming](bpc-adaptive-directional.md#11-hls-streaming-architecture). Kiểu block control HLS, cách cập nhật cấu hình và register map chưa được chốt.
