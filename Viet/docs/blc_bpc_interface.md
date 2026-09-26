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

Input BLC là RAW10 trước Black Level Correction. Bốn hệ số `bl_r`, `bl_gr`, `bl_gb`, `bl_b` đi qua AXI4-Lite và cùng miền RAW10.

```text
Y = max(X - BL_CFA, 0)
```

Output BLC giữ nguyên kích thước, CFA alignment và AXI sideband. Output này là input của BPC.

## Adaptive Directional BPC

BPC nhận RAW10 sau BLC trên cùng stream format. Các tham số AXI4-Lite là `T0_R`, `T0_G`, `T0_B`, `k_s`, `k_a`; hai phase green dùng chung `T0_G`.

```text
T = T0_CFA + (P >> k_s) + (G_min >> k_a)
detect khi abs(X - P) > T
```

Config đang dùng là `(T0_R, T0_G, T0_B, k_s, k_a) = (4, 8, 4, 3, 0)`. BPC trả về một pixel RAW10 cho mỗi pixel input và giữ sideband theo tọa độ output.

## Composite HLS top

`isp_top` xử lý đúng một frame 1920 × 1080 mỗi lần gọi. Top nhận stream RAW10 trước BLC, chạy BLC rồi BPC, và xuất đúng 2.073.600 pixel theo cùng format AXI4-Stream. Bốn hệ số BLC và năm tham số BPC là các scalar AXI4-Lite của top ghép; cấu hình phải ổn định trong suốt frame.

`isp_top_frames` hiện là entry point thử nghiệm để xử lý `frame_count` frame liên tiếp trong một lần gọi, dùng cùng định dạng AXI4-Stream và cùng các hệ số. Tham số `frame_count` đặt tại offset AXI4-Lite `0x58`; cấu hình ổn định trong toàn bộ lần gọi. Ranh giới giữa hai frame trong cùng lần gọi không có bước drain, còn sau frame cuối có drain nội bộ để xuất đủ pixel. Đây chưa phải giao diện video streaming đích: kiến trúc đích không cần biết trước số frame, nhận SOF/EOL theo handshake và có thể drain phần đuôi frame trong khoảng trống giữa hai frame. Xem [contract HLS streaming](bpc-adaptive-directional.md#11-hls-streaming-architecture).

| Tham số AXI4-Lite | Offset |
|---|---:|
| `bl_r`, `bl_gr`, `bl_gb`, `bl_b` | `0x10`, `0x18`, `0x20`, `0x28` |
| `thresh_r`, `thresh_g`, `thresh_b` | `0x30`, `0x38`, `0x40` |
| `shift_signal`, `shift_gradient` | `0x48`, `0x50` |

Top ghép hiện dùng hai hàm xử lý stream chung với `isp_blc_top` và `isp_bpc_top`. Hai top độc lập vẫn là entry point để kiểm tra riêng từng IP. AXI4-Stream chỉ xuất hiện ở cổng ngoài; các adapter đổi sang packet nội bộ giữ `data`, `keep`, `strb`, `user` và `last`. Trong top ghép, hai hàm xử lý chạy đồng thời qua stream nội bộ bằng HLS `DATAFLOW`; stream này không phải frame buffer. BPC tự tạo `TUSER` và `TLAST` của output từ tọa độ. Giao diện điều khiển HLS cho streaming vô hạn và quy tắc cập nhật cấu hình chưa được chốt; các offset AXI4-Lite ở bảng trên mô tả top hiện tại.
