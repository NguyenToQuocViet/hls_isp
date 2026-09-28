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

`blc_top` là IP BLC độc lập và là top HLS để đánh giá riêng BLC. Hai cổng pixel dùng AXI4-Stream `ap_axiu<16,1,0,0>`; wrapper đổi sang packet RAW10 nội bộ trước `blc_engine` và đổi lại tại output. Input hợp lệ có `keep = strb = 0b11` và `TDATA[15:10] = 0`; wrapper chỉ lấy `TDATA[9:0]`, `user`, `last`. Output đặt các bit dữ liệu cao bằng 0, `keep = strb = 0b11`, còn `user` và `last` lấy từ engine. Không quy định phục hồi input có `keep/strb` hoặc bit dữ liệu cao sai.

Bốn black level và `config_valid` là thanh ghi AXI4-Lite của `blc_top`; `config_valid` reset về 0. Sau reset, phần mềm ghi đủ bốn black level, đợi các giao dịch ghi hoàn tất và bảo đảm thứ tự MMIO, rồi ghi `config_valid = 1` sau cùng. Phần mềm giữ cả năm giá trị ổn định đến reset kế tiếp. Không hỗ trợ đổi cấu hình giữa các frame trong cùng lần chạy.

Wrapper đọc các thanh ghi sau khi thấy `config_valid = 1`, phát đúng một snapshot `BlcConfig` cho runner và một token mở ingress. Runner nhận snapshot trước khi gọi `blc_engine` và giữ nó cho mọi frame đến reset. Engine không đọc thanh ghi AXI-Lite và không chốt lại cấu hình tại SOF. Pixel đến sớm có thể được nhận vào buffer AXI do HLS tạo; mọi beat đã được nhận phải giữ đúng thứ tự hoặc bị backpressure khi buffer đầy, và không được xử lý bằng cấu hình chưa công bố. Sau reset giữa frame, frame dở bị hủy; phần mềm phải cấu hình lại và nguồn phải bắt đầu frame mới với SOF. `config_valid` chỉ công bố cấu hình, không phải lệnh xử lý một frame hay tín hiệu hoàn tất frame.

## Adaptive Directional BPC

BPC nhận RAW10 sau BLC trên cùng stream format. Các tham số thuật toán là `T0_R`, `T0_G`, `T0_B`, `k_s`, `k_a`; hai phase green dùng chung `T0_G`.

```text
T = T0_CFA + (P >> k_s) + (G_min >> k_a)
detect khi abs(X - P) > T
```

Operating point hiện chọn là `(T0_R, T0_G, T0_B, k_s, k_a) = (4, 8, 4, 3, 0)`. Streaming BPC cần trả về một pixel RAW10 cho mỗi pixel input và tạo sideband theo tọa độ output.

## Streaming HLS target

Branch rebuild giữ hàm pixel BLC/BPC trong HLS và đang xây dựng worker cùng `blc_top`; worker streaming, adapter và top cũ đã được lưu ở branch `archive/bpc-agent-hls-2026-09-26` (commit `b55eb38`). Top tích hợp BLC→BPC chưa được xây dựng trên branch này.

Top tích hợp đích cần xử lý luồng frame không biết trước số lượng, giữ định dạng AXI4-Stream ở bảng trên và nối BLC → BPC. Quy tắc nhận frame, drain, backpressure và phát sideband nằm trong [contract streaming](bpc-adaptive-directional.md#11-hls-streaming-architecture). Block control và cấu hình của top tích hợp, cùng register map cụ thể, chưa được chốt.
