<!--
Project: Adaptive Directional BPC and BLC
Module: Week 5.2 ISP Progress Report
Description: Summarize the RAW10 migration, verification rerun, and selected adaptive BPC operating point.
Author: Viet Nguyen To Quoc
-->

# Báo cáo tiến độ Week 5.2 ISP

## Tổng quan

| Công việc | Kết quả chính |
|---|---|
| RAW10 | Đổi toàn bộ flow BLC/BPC từ RAW12 sang RAW10. |
| Verification | BLC và BPC đều pass Reference, CSim, C Synthesis và RTL CoSim. |
| Tuning | Chốt Adaptive config `(4, 8, 4, 3, 0)`, đạt BRG 93,16% và BCR 3,12%. |
| Benchmark | Đang so sánh với AMD Vitis Vision và OpenISP trên cùng held-out split RAW10. |

## Chuyển đổi RAW10

Em chuyển toàn bộ flow từ RAW12 sang RAW10. Pixel Bayer hiện nằm trong khoảng `[0, 1023]`. Phần reference, HLS BLC/BPC, testbench, injector, evaluator và flow tuning đều dùng cùng contract này.

Trong HLS, các kiểu dữ liệu và phép tính được thu hẹp theo miền RAW10. AXI4-Stream vẫn dùng word 16-bit nhưng payload pixel chuyển sang bit `[9:0]`; testbench cũng kiểm tra lại payload và sideband theo format này. Injector và evaluator chuẩn hóa frame về RAW10 trước khi chạy BLC, BPC và tuning.

Interface giữa BLC và BPC nằm trong file `blc_bpc_interface.md` nộp kèm.

## Verification RAW10

Reference là golden model cho HLS CSim và RTL CoSim. Kết quả chạy lại với source RAW10 như sau:

| IP | Reference | CSim | C Synthesis | RTL CoSim |
|---|---|---|---|---|
| BLC | 64 passed, 0 failed | 2.074.622 passed, 0 failed | Pass | 2.074.622 passed, 0 failed |
| Adaptive Directional BPC | 54 passed, 0 failed | 2.083.600 passed, 0 failed | Pass | 2.073.600 passed, 0 failed |

Với BLC, CSim gồm 1.000 pixel random, seed `20260914`, và một frame 1920 × 1080. Với BPC, CSim gồm 8.000 random case, 2.000 constrained-random case với seed `20260915`, sau đó chạy thêm một frame 1920 × 1080 với seed `20260916`. CoSim của cả hai khối đều đối chiếu generated RTL với reference, gồm payload RAW10 và AXI sideband.

Kết quả timing, latency, II và resource utilization nằm trong file `blc_bpc_resource_report.md` nộp kèm.

## Tuning và config được chọn

Tuning dùng 883 ảnh, sau đó test trên 221 ảnh held-out của split RAW10 cố định. Em chọn `candidate_3 = (4, 8, 4, 3, 0)`. So với các điểm ưu tiên BRG hơn, config này giảm BRG không nhiều nhưng giữ BCR quanh 3%, phù hợp hơn với mục tiêu hạn chế sửa nền.

Thông tin FiveK, điều kiện lọc và split nằm trong file `fivek_dataset.md` nộp kèm.

| Config / Metric | Kết quả |
|---|---:|
| Config `(T0_R, T0_G, T0_B, k_s, k_a)` | `(4, 8, 4, 3, 0)` |
| RG_hot | 97,77% |
| RG_dead | 88,55% |
| BRG | 93,16% |
| BCR | 3,12% |
| IOTCR | 0,00178% |

## Benchmark với AMD và OpenISP

Em đang benchmark Adaptive BPC với AMD Vitis Vision Bad Pixel Correction và OpenISP gradient DPC. Cả ba chạy trên cùng held-out split RAW10, cùng input sau BLC và cùng bộ metric `RG_hot`, `RG_dead`, `BRG`, `BCR`, `IOTCR`.

Kết quả benchmark chưa đưa vào báo cáo này. Khi chạy xong, em sẽ bổ sung bảng metric vào đúng phần này.

## Minh chứng nộp kèm

- `blc_bpc_resource_report.md`
- `blc_bpc_interface.md`
- `fivek_dataset.md`
- `csim_blc.png`, `csim_bpc.png`
- `cosim_blc.png`, `cosim_bpc.png`
- `hls_blc.png`, `hls_bpc.png`
- `candidate_3_selection.json`: config và kết quả tuning được chọn
- `candidate_3_test_per_image.csv`: metric Adaptive trên 221 ảnh held-out
