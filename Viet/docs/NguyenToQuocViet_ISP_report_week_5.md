<!--
Project: Adaptive Directional BPC and BLC
Module: Week 5 ISP Progress Report
Description: Summarize adaptive BPC v2 evaluation and BLC and BPC HLS results.
Author: Viet Nguyen To Quoc
-->

# Báo cáo tiến độ Week 3 (Week 5 ISP)

## Những việc đã làm

| Công việc | Kết quả chính |
|---|---|
| Cập nhật Adaptive BPC v2 | Chuyển sang so pixel trung tâm với prediction `P`; threshold dùng `P` và `G_min` của hướng được chọn. |
| Tuning và so sánh lại với baseline | Tuning trên 883 ảnh, test trên 221 ảnh. BRG của adaptive đạt 93,53%, baseline 88,01%; adaptive tốt hơn trên 216/221 ảnh. |
| Đưa BLC và BPC qua HLS | Cả hai đạt II=1 và target 150 MHz theo ước tính C synthesis. |
| Verify BLC | Reference test pass 64/64; HLS CSim và Verilog CoSim đều pass. |

## Adaptive v2 và kết quả test

Ở bản trước, detector dựa vào khoảng min–max của 8 same-CFA neighbor. Sang v2, em bỏ cách đó. Vẫn là window 5 × 5: chọn hướng có cặp pixel gần nhau nhất, lấy trung bình cặp đó làm prediction `P`, rồi so pixel trung tâm `X` với `P`. Chênh lệch của cặp được chọn là `G_min`.

```text
T = T0_CFA + (P >> k_s) + (G_min >> k_a)
Detect khi abs(X - P) > T; nếu detect thì thay X bằng P.
```

Chọn xong hướng là đã có cả `P` lẫn `G_min` cho datapath. Không cần tìm thêm min/max trong 8 neighbor. Cách này gọn hơn về phép tính, còn mức tiết kiệm tài nguyên so với v1 thì em chưa đo.

Lần này baseline không giữ ngưỡng `T = 0` của báo cáo trước. Em chọn adaptive có BRG cao nhất trong pool ứng viên trên 883 ảnh tuning, rồi dùng giới hạn BCR/IOTCR của cấu hình đó để tìm ngưỡng cho baseline. Cấu hình chốt là adaptive `(T0_R, T0_G, T0_B, k_s, k_a) = (16, 16, 16, 3, 0)` và baseline `T = 160`. Trên 221 ảnh test:

| Metric | Baseline | Adaptive v2 |
|---|---:|---:|
| RG_hot | 98,02% | 97,77% |
| RG_dead | 78,00% | 89,29% |
| BRG | 88,01% | 93,53% |
| BCR | 1,75% | 3,85% |
| IOTCR | 0,00208% | 0,00211% |

BRG của adaptive nhỉnh hơn (**93,53% so với 88,01%**), chủ yếu nhờ khôi phục dead pixel tốt hơn. Đổi lại, nó sửa nhiều pixel nền hơn: BCR là 3,85% so với 1,75% của baseline. Giới hạn BCR/IOTCR là điều kiện lúc chọn cấu hình trên tuning; sang test, BCR của hai bên không bằng nhau. Adaptive cũng nhỉnh hơn giới hạn tuning một chút ở cả BCR và IOTCR.

## HLS BLC và BPC

BPC nhận RAW12 qua AXI4-Stream, giữ bốn hàng trong line buffer để tạo window 5 × 5 và xả các pixel còn lại ở cuối frame. BLC thì trừ Black Level theo từng phase RGGB. Kết quả Vitis HLS 2026.1 trên `xczu7ev-ffvc1156-2-e`, target 150 MHz:

| Khối | II mỗi pixel | Timing slack ước tính | Tài nguyên ước tính |
|---|---:|---:|---|
| BLC | 1 | +1,76 ns | 465 LUT, 169 FF, 0 BRAM, 0 DSP |
| BPC | 1 | +0,07 ns | 2.332 LUT, 1.129 FF, 8 BRAM18K, 0 DSP |

Các con số trên đến từ C synthesis. Timing sau place-and-route thì chưa có, mà BPC chỉ còn dư 0,07 ns ở bước này.

## Verification

BLC reference pass **64/64 kiểm tra**: chọn CFA phase, xử lý các mức sát ngưỡng Black Level và chạy một frame nhỏ có output tính tay. HLS dùng reference này làm golden model. Testbench còn có 1.000 mẫu random cố định seed và một frame 1920 × 1080, so cả pixel lẫn AXI4-Stream sideband. CSim và Verilog CoSim đều pass: **2.074.622 kiểm tra, 0 lỗi** ở mỗi run.

BPC đã pass đối chiếu full-frame giữa HLS C++ và reference, cùng với CSim. RTL CoSim cho BPC thì chưa chạy; backpressure và timing sau place-and-route vẫn chưa có kết quả. Phần đánh giá trên ảnh cũng chỉ dùng FiveK compatible và hot/dead pixel được inject, không phải lỗi cảm biến đo trực tiếp.
